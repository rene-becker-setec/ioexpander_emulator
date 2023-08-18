#include "ipi.h"
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/spi.h>
#include <string.h>
#include <stdio.h>
#include "ioxp_emu2pc.h"

#include "rvmn_spi.h"

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

static uint8_t ipi_active_buffer = 0;

static struct spi_config ipi_spi_config;
struct SPIS_tuMOSI spi_rx_buf[2];
struct SPIS_tuMISO spi_tx_buf[2];
static const struct spi_buf spi_tx_buf_pack[SPI_DMA_OP_CNT] = {
        {.buf = spi_tx_buf[0].spi_dma_block, .len = SPI_TRANSFER_SIZE},
};
static const struct spi_buf spi_rx_buf_pack[SPI_DMA_OP_CNT] = {
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



static void ipi_transceive_thread_func(void*, void*, void*) {

	int rc;

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

	memset(spi_tx_buf[0].spi_dma_block, 0xaa, sizeof(spi_tx_buf[0].spi_dma_block));

	// Start timer - this will start toggling the SPI Ready signal
	k_timer_start(&ipi_timer, K_MSEC(10), K_MSEC(10));

	while (true) {

		// Update checksums on the buffer about to be transmitted

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
		LOG_DBG("SPI trcv complete ...");
//		k_sleep(K_MSEC(100));

		// lock mutex to protect IPI buffers
		k_mutex_lock(&ipi_mutex, K_FOREVER);

		// Switch IPI buffers
		switch (ipi_active_buffer) {
			case 0x01:
				ipi_active_buffer = 0x00;
				break;
			case 0x00:
				ipi_active_buffer = 0x01;
				break;
			default:
				LOG_ERR("ipi_active_buffer value not valid");
				return;
		}

		// copy TX data buffer
		// we want to start the 'fresh' buffer with the same state as the one about to be
		// transmitted. At least for all the digital or analog input this is ... CAN messages
		// should be cleared out.

		// unlock mutex - free to write to the 'fresh' buffer while the other one is being
		// transmitted
		k_mutex_unlock(&ipi_mutex);

		// signal the rx processing task that new data is available for processing
		k_sem_give(&ipi_sem);


		// check the interval between this and previous transceive.
		// Issue a warning if interval too large

	}
}

static void ipi_rxproc_thread_func(void*, void*, void*) {


	while(true){
		k_sem_take(&ipi_sem, K_FOREVER);

		// TODO: validate IPI frame checksum

		// Extract CAN Messages ...
		for (int i=0; i<SPIS_nCANMsgTX; i++){
			if (spi_rx_buf[0].aCANMsg[i].u16ID28_16 & 0x8000) {

				// TODO: validate checksum

				rvc_msg_t m;
				memcpy(&m.id,spi_rx_buf[0].aCANMsg[i].u8Data, sizeof(m.id));
				// bits 15:13 are control bits for the IPI trasnmission, don't belong
				// to RV-C.
				// [15] = Message valid bit, set to 1.
                // [14] = Extended ID bit, set to 1
                // [13] = 1 for TX (0 for RX)
				// mask those out ....
				m.id &= 0x1fffffff;
				memcpy(m.data,spi_rx_buf[0].aCANMsg[i].u8Data, sizeof(m.data));
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


int ipi_send_can_msg(void){

	int status = -EBUSY;

	k_mutex_lock(&ipi_mutex, K_FOREVER);

	// Interate  over the CAN message slots in the IPI data structure of the currently
	// writable buffer and see whether there's a free slot available.
	// If so load message data into this slot. If not we'll return -EBUSY
	for (int i = 0; i < 4; i++){
		if(true) {
			status = 0;
		}
	}

	k_mutex_unlock(&ipi_mutex);

    return status;
}
