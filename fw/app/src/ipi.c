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
	gpio_pin_toggle(gpio_dev, 4);
}

K_TIMER_DEFINE(ipi_timer, ipi_timer_func, NULL);
K_MUTEX_DEFINE(ipi_mutex);
K_SEM_DEFINE(ipi_sem, 0, 1);



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
	rc = gpio_pin_configure(gpio_dev, 4, GPIO_OUTPUT_ACTIVE);
	if (rc != 0){
		LOG_ERR("Configuring SPI Ready pin failed");
		return;
	}

	// Start timer
	k_timer_start(&ipi_timer, K_MSEC(500), K_MSEC(500));

	while (true) {

		// Update checksums on the buffer about to be transmitted

		// run SPI transaction
		// We are SPI Slave, the SPI ready signal (a GPIO) is used to signal to the
		// master that we are ready to receive (The master MCU is looking for a falling edge).
		// we should be ok setting the RDY pin before calling spi_transceive(), otherwise we
		// might have to do this in a support thread...

//		spi_transceive(
//			ipi_spi_dev,
//			&ipi_spi_config,       // const struct spi_config *config,
//			&spi_tx_buf_set_pack,  // const struct spi_buf_set *tx_bufs,
//			&spi_rx_buf_set_pack   // const struct spi_buf_set *rx_bufs,
//		);

		k_sleep(K_MSEC(100));

		// lock mutex to protect IPI buffers
		k_mutex_lock(&ipi_mutex, K_FOREVER);

		// Switch IPI buffers

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

	uint32_t num_msg_sent = 0;

	while(true){
		k_sem_take(&ipi_sem, K_FOREVER);

		char msg[100] = {0};
		snprintf(msg, sizeof(msg), "CAN Message Received %d", num_msg_sent);
		binary_t b = {(uint8_t*)msg,(uint32_t)strlen(msg)};
		/* RPC call */
		canMsgRcvd(&b);
		LOG_INF("send CAN Notification ...");
		num_msg_sent++;
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
