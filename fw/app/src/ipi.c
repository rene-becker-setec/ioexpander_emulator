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



static void ipi_thread_func(void*, void*, void*) {

	uint32_t num_msg_sent = 0;

	while (true) {
		k_sleep(K_SECONDS(1));

		LOG_INF("client thread running ...");

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


int ipi_queue_can_msg(void){

    return 0;

}
