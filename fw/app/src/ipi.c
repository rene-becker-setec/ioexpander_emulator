#include "ipi.h"
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <string.h>
#include <stdio.h>
#include "ioxp_emu2pc.h"

#define LOG_LEVEL LOG_LEVEL_INF
LOG_MODULE_REGISTER(ipi);

struct k_thread ipi_thread_data;
K_THREAD_STACK_DEFINE(ipi_thread_stack, 2048);


K_TIMER_DEFINE(ipi_timer, NULL, NULL);
K_MUTEX_DEFINE(ipi_mutex);


static void ipi_thread_func(void*, void*, void*) {

	uint32_t num_msg_sent = 0;

	// Start timer
	k_timer_start(&ipi_timer, K_SECONDS(1), K_SECONDS(1));

	while (true) {

		// block until timer expires
		k_timer_status_sync(&ipi_timer);

		LOG_INF("IPI thread running ...");

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

		// run SPI transaction

		// process the newly received data ...
		// for digital and analog inputs we compare the previous with the newly received 
		// buffer. If different send a change notification to the host.

		// for CAN we just have to test newly received buffer for valid CAN message content.
		// If found send a 'can msg received' notification to the host.

		char msg[100] = {0};
		snprintf(msg, sizeof(msg), "CAN Message Received %d", num_msg_sent);
		binary_t b = {(uint8_t*)msg,(uint32_t)strlen(msg)};
		/* RPC call */
		canMsgRcvd(&b);
		LOG_INF("send CAN Notification ...");
		num_msg_sent++;
		LOG_INF("remote not connected ...");
	}
}


int ipi_init(void){

	k_thread_create(
		&ipi_thread_data,
		ipi_thread_stack,
		K_THREAD_STACK_SIZEOF(ipi_thread_stack),
		ipi_thread_func, NULL, NULL, NULL,
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
