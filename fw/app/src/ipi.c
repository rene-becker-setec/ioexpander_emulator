#include "ipi.h"
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/spi.h>
#include <string.h>
#include <stdio.h>
#include "ioxp_emu2pc.h"

#include "rvmn_spi.h"

#define IPI_VERSION(major, minor, patch) ((0x0FU & major) << 12 | (0x0FU & minor) << 8 | (0xFFU & patch))

#define GPIO_NAME "GPIO_0"
#define SPI_RDY_PIN 11

#define MY_GPIO DT_NODELABEL(gpio0)

#define SPI_DMA_OP_CNT 1U

#define LOG_LEVEL LOG_LEVEL_DBG
LOG_MODULE_REGISTER(ipi);

const struct device *ipi_spi_dev = DEVICE_DT_GET(DT_NODELABEL(spi0));
const struct device *gpio_dev = DEVICE_DT_GET(DT_NODELABEL(gpio0));

struct k_thread ipi_transceive_thread_data;
K_THREAD_STACK_DEFINE(ipi_transceive_thread_stack, 2048);

struct k_thread ipi_rxproc_thread_data;
K_THREAD_STACK_DEFINE(ipi_rxproc_thread_stack, 2048);

static void ipi_timer_func(struct k_timer *timer_id){
	gpio_pin_toggle(gpio_dev, SPI_RDY_PIN);
}

K_TIMER_DEFINE(ipi_timer, ipi_timer_func, NULL);
K_MUTEX_DEFINE(ipi_mutex);
K_SEM_DEFINE(ipi_sem, 0, 1);


static struct spi_config ipi_spi_config;
struct SPIS_tuMOSI spi_rx_buf[2];
struct SPIS_tuMISO spi_tx_buf[2];
static struct spi_buf spi_tx_buf_pack[SPI_DMA_OP_CNT] = {
        {.buf = spi_tx_buf[0].spi_dma_block, .len = SPI_TRANSFER_SIZE},
};
static struct spi_buf spi_rx_buf_pack[SPI_DMA_OP_CNT] = {
        {.buf = spi_rx_buf[0].spi_dma_block, .len = SPI_TRANSFER_SIZE},
};

static const struct spi_buf_set spi_tx_buf_set_pack = {
        .buffers = spi_tx_buf_pack,
        .count = SPI_DMA_OP_CNT,  /* must be set to 1 otherwise the Nordic SPI driver will reject it  */
};
static const struct spi_buf_set spi_rx_buf_set_pack = {
        .buffers = spi_rx_buf_pack,
        .count = SPI_DMA_OP_CNT,  /* must be set to 1 otherwise the Nordic SPI driver will reject it  */
};

// 'active' buffers are those allocated for SPI transceive, the
// 'inactive' those that can be written to from host/data is processed from for
// upload to host
static struct SPIS_tuMOSI* active_rx_buffer = &spi_rx_buf[0];
static struct SPIS_tuMOSI* inactive_rx_buffer = &spi_rx_buf[1];
static struct SPIS_tuMISO* active_tx_buffer = &spi_tx_buf[0];
static struct SPIS_tuMISO* inactive_tx_buffer = &spi_tx_buf[1];

/**
 * @brief Calculates 32-bit checksum
 *
 * @param data pointer to data block checksum shall be calculated over
 * @param len  number of 32bit words checksum shall be calculated over
 * @return uint32_t
 */
static uint32_t u32CalculateCS(const uint32_t *data, uint16_t len)
{
         uint32_t checksum = 0x00000000;

         for (uint16_t i = 0; i < len; i++) {
                 checksum += *data;
                 data++;
         }

         return checksum;
}


static void ipi_transceive_thread_func(void*, void*, void*) {

	int rc;
	struct SPIS_tuMOSI* tmp_rx_buffer; // this one is for when swapping active/passive
	struct SPIS_tuMISO* tmp_tx_buffer; // this one is for when swapping active/passive


	if (!device_is_ready(gpio_dev)) {
        /* Not ready, do not use */
		LOG_ERR("GPIO Port is not ready ...");
        return;
	}
	rc = gpio_pin_configure(gpio_dev, SPI_RDY_PIN, GPIO_OUTPUT_ACTIVE);
	if (rc != 0){
		LOG_ERR("Configuring SPI Ready pin failed");
		return;
	}

	if (!device_is_ready(ipi_spi_dev)) {
		LOG_ERR("SPI device is not ready ...");
		return;
	}

	ipi_spi_config.frequency = SPI_FREQUENCY;
    ipi_spi_config.operation = (SPI_OP_MODE_SLAVE | SPI_WORD_SET(8));
    ipi_spi_config.slave = 1U;

	memset(active_tx_buffer->spi_dma_block, 0x00, sizeof(active_tx_buffer->spi_dma_block));

	// Initilaize the TX IPI structure currently active with sensible defaults ...
	active_tx_buffer->u16PSinfo          = IPI_VERSION(1, 0, 0); 	// IPI version
    active_tx_buffer->u16SwVer           = IPI_VERSION(0, 3, 0);    // Simulate NXP software version
    active_tx_buffer->u16VBATT_MON       = 0x848;
    active_tx_buffer->u16VCC_Switched    = 0x91c;
    active_tx_buffer->u16DETECT_VHBRIDGE = 0;
	active_tx_buffer->u16SysInfoReserved = 0;
    active_tx_buffer->ign_mon_adc        = 0;

	active_tx_buffer->IO.u16Inp_1[ 0] = 0x803e;
    active_tx_buffer->IO.u16Inp_1[ 1] = 0x8068;
    active_tx_buffer->IO.u16Inp_1[ 2] = 0x8068;
    active_tx_buffer->IO.u16Inp_1[ 3] = 0x8110;
    active_tx_buffer->IO.u16Inp_1[ 4] = 0x80bc;
    active_tx_buffer->IO.u16Inp_1[ 5] = 0x80e6;
    active_tx_buffer->IO.u16Inp_1[ 6] = 0x0;
    active_tx_buffer->IO.u16Inp_1[ 7] = 0x00;
    active_tx_buffer->IO.u16Inp_1[ 8] = 0x99c2;
    active_tx_buffer->IO.u16Inp_1[ 9] = 0x99b4;
    active_tx_buffer->IO.u16Inp_1[10] = 0x99c8;
    active_tx_buffer->IO.u16Inp_1[11] = 0x9a0b;
    active_tx_buffer->IO.u16Inp_1[12] = 0x862c;
    active_tx_buffer->IO.u16Inp_1[13] = 0x85ac;


	// Start timer - this will start toggling the SPI Ready signal
	k_timer_start(&ipi_timer, K_MSEC(10), K_MSEC(10));

	while (true) {

		// Update checksums on the buffer about to be transmitted
		#pragma GCC diagnostic push
		#pragma GCC diagnostic ignored "-Waddress-of-packed-member"
		active_tx_buffer->u32CSinfo = u32CalculateCS((uint32_t *)&active_tx_buffer->u16PSinfo, 2);
		#pragma GCC diagnostic pop
		active_tx_buffer->u32CS = u32CalculateCS((uint32_t*)&active_tx_buffer, sizeof(active_tx_buffer)/4 - 1);

		// run SPI transaction
		// We are SPI Slave, the SPI ready signal (a GPIO) is used to signal to the
		// master that we are ready to receive (The master MCU is looking for a falling edge).
		// we should be ok setting the RDY pin before calling spi_transceive(), otherwise we
		// might have to do this in a support thread...

		spi_transceive(
			ipi_spi_dev,
			&ipi_spi_config,       // const struct spi_config *config,
			&spi_tx_buf_set_pack,  // const struct spi_buf_set *tx_bufs,
			&spi_rx_buf_set_pack   // const struct spi_buf_set *rx_bufs,
		);
//		LOG_DBG("SPI trcv complete ...");
//		k_sleep(K_MSEC(100));

		// lock mutex to protect IPI buffers
		k_mutex_lock(&ipi_mutex, K_FOREVER);

		// Switch IPI buffers
		tmp_rx_buffer = active_rx_buffer;
		active_rx_buffer = inactive_rx_buffer;
		inactive_rx_buffer = tmp_rx_buffer;

		tmp_tx_buffer = active_tx_buffer;
		active_tx_buffer = inactive_tx_buffer;
		inactive_tx_buffer = tmp_tx_buffer;

		spi_tx_buf_pack[0].buf = active_tx_buffer->spi_dma_block;
		spi_rx_buf_pack[0].buf = active_rx_buffer->spi_dma_block;

		// copy TX data buffer
		// we want to start the 'fresh' buffer with the same state as the one about to be
		// transmitted. At least for all the digital or analog input this is ... CAN messages
		// should be cleared out.
		memcpy(
			&inactive_tx_buffer->spi_dma_block,
			&active_tx_buffer->spi_dma_block,
			sizeof(inactive_tx_buffer->spi_dma_block)
		);
		memset(&inactive_tx_buffer->aCANMsg, 0x0, sizeof(inactive_tx_buffer->aCANMsg));

		// unlock mutex - free to write to the 'fresh' buffer while the other one is being
		// transmitted
		k_mutex_unlock(&ipi_mutex);

		// signal the rx processing task that new data is available for processing
		k_sem_give(&ipi_sem);

		// check the interval between this and previous transceive.
		// Issue a warning if interval too large
		// TODO:
	}
}

static void ipi_rxproc_thread_func(void*, void*, void*) {


	while(true){
		k_sem_take(&ipi_sem, K_FOREVER);

		// TODO: validate IPI frame checksum

		// Extract CAN Messages ...
		for (int i=0; i<SPIS_nCANMsgTX; i++){
			if (inactive_rx_buffer->aCANMsg[i].u16ID28_16 & 0x8000) {

				LOG_DBG(
					"SA: 0x%02x, DGN: 0x%03x, PRI: 0x%x",
					inactive_rx_buffer->aCANMsg[i].id.sa,
					inactive_rx_buffer->aCANMsg[i].id.dgn,
					inactive_rx_buffer->aCANMsg[i].id.pri
				);

				// TODO: validate checksum

				rvc_msg_t m;
				memcpy(&m.id, inactive_rx_buffer->aCANMsg[i].id.canid, sizeof(m.id));
				// bits 15:13 are control bits for the IPI trasnmission, don't belong
				// to RV-C.
				// [15] = Message valid bit, set to 1.
                // [14] = Extended ID bit, set to 1
                // [13] = 1 for TX (0 for RX)
				// mask those out ....
				m.id &= 0x1fffffff;
				memcpy(m.data, inactive_rx_buffer->aCANMsg[i].u8Data, sizeof(m.data));
				LOG_DBG("tx can msg to host");
				canMsgRcvd(&m);
			}
		}
	}
}

int ipi_init(void){

	k_thread_create(
		&ipi_transceive_thread_data,
		ipi_transceive_thread_stack,
		K_THREAD_STACK_SIZEOF(ipi_transceive_thread_stack),
		ipi_transceive_thread_func, NULL, NULL, NULL,
		2, 0, K_NO_WAIT
	);

	k_thread_create(
		&ipi_rxproc_thread_data,
		ipi_rxproc_thread_stack,
		K_THREAD_STACK_SIZEOF(ipi_rxproc_thread_stack),
		ipi_rxproc_thread_func, NULL, NULL, NULL,
		2, 0, K_NO_WAIT
	);

    return 0;
}


int ipi_send_can_msg(const rvc_msg_t *msg){

	int status = -EBUSY;

	k_mutex_lock(&ipi_mutex, K_FOREVER);

	// Interate  over the CAN message slots in the IPI data structure of the currently
	// writable buffer and see whether there's a free slot available.
	// If so load message data into this slot. If not we'll return -EBUSY
	for (int i = 0; i < SPIS_nCANMsgRX; i++){
		if (inactive_tx_buffer->aCANMsg[i].u16ID28_16 == 0x0) {
			memcpy(
				&inactive_tx_buffer->aCANMsg[i].id.canid, &msg->id,
				sizeof(inactive_tx_buffer->aCANMsg[i].id.canid)
			);
			memcpy(
				&inactive_tx_buffer->aCANMsg[i].u8Data, &msg->data,
				sizeof(inactive_tx_buffer->aCANMsg[i].u8Data)
			);
			status = 0;
		}
	}

	k_mutex_unlock(&ipi_mutex);

    return status;
}
