/*
 * Copyright (c) 2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/uart.h>
#include <string.h>
#include <zephyr/random/rand32.h>

#include <zephyr/usb/usb_device.h>
#include <zephyr/usb/class/usb_hid.h>
#include <zephyr/usb/class/usb_cdc.h>

#include <erpc_server_setup.h>
#include <erpc_arbitrated_client_setup.h>
#include "erpc_zephyr_usb_cdc_transport.hpp"

#define LOG_LEVEL LOG_LEVEL_DBG
LOG_MODULE_REGISTER(main);

static void status_cb(enum usb_dc_status_code status, const uint8_t *param)
{
	LOG_INF("Status %d", status);
}

int main(void)
{
	uint32_t dtr = 0U;
	int ret;

	const struct device *cdc_dev;
	cdc_dev = DEVICE_DT_GET_ONE(zephyr_cdc_acm_uart);

	if (!device_is_ready(cdc_dev)) {
		LOG_ERR("CDC ACM device not ready");
		return 0;
	}

	ret = usb_enable(status_cb);
	if (ret != 0) {
		LOG_ERR("Failed to enable USB");
		return 0;
	}

	/* Initialize CDC ACM */
	while (true) {
		uart_line_ctrl_get(cdc_dev, UART_LINE_CTRL_DTR, &dtr);
		if (dtr) {
			break;
		} else {
			/* Give CPU resources to low priority threads. */
			k_sleep(K_MSEC(100));
		}
	}

	LOG_INF("Waiting for 1s");
	/* Wait 1 sec for the host to do all settings */
	k_busy_wait(USEC_PER_SEC);


	erpc_transport_t arbitrated_transport;
	erpc_transport_t transport = reinterpret_cast<erpc_transport_t>(
		new erpc::ZephyrUsbCdcTransport(cdc_dev)
	);
	erpc_mbf_t message_buffer_factory = erpc_mbf_dynamic_init();

	erpc_client_t client = erpc_arbitrated_client_init(transport, message_buffer_factory, &arbitrated_transport);

	LOG_INF("Setting up server ...");
//	auto server = erpc_server_init(arbitrated_transport, message_buffer_factory);

	while (true) {
		k_sleep(K_MSEC(100));
	}

}
