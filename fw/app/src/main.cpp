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

#define WELCOME_MSG "Welcome ..."

/* CDC ACM */

static volatile bool data_transmitted;
static volatile bool data_arrived;

static void write_data(const struct device *dev, const char *buf, int len)
{
	uart_irq_tx_enable(dev);

	while (len) {
		int written;

		data_transmitted = false;
		written = uart_fifo_fill(dev, (const uint8_t *)buf, len);
		while (data_transmitted == false) {
			k_yield();
		}

		len -= written;
		buf += written;
	}

	uart_irq_tx_disable(dev);
}

static void cdc_0_int_handler(const struct device *dev, void *user_data)
{
	ARG_UNUSED(user_data);

	uart_irq_update(dev);

	if (uart_irq_tx_ready(dev)) {
		data_transmitted = true;
	}

	if (!uart_irq_rx_ready(dev)) {
		return;
	}
	uint32_t bytes_read;
	uint8_t rx_buffer;

	while ((bytes_read = uart_fifo_read(dev, &rx_buffer, 1))) {
		LOG_INF("char received on CDC0: 0x%02x (%c)", rx_buffer, (char) rx_buffer);
	}

}

static void cdc_1_int_handler(const struct device *dev, void *user_data)
{
	ARG_UNUSED(user_data);

	uart_irq_update(dev);

	if (uart_irq_tx_ready(dev)) {
		data_transmitted = true;
	}

	if (!uart_irq_rx_ready(dev)) {
		return;
	}
	uint32_t bytes_read;
	uint8_t rx_buffer;

	while ((bytes_read = uart_fifo_read(dev, &rx_buffer,1))) {
		LOG_INF("char received on CDC1: 0x%02x (%c)", rx_buffer, (char) rx_buffer);
	}
}

/* Devices */

static void status_cb(enum usb_dc_status_code status, const uint8_t *param)
{
	LOG_INF("Status %d", status);
}

#define DEVICE_AND_COMMA(node_id) DEVICE_DT_GET(node_id),

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

	uart_irq_callback_set(cdc_dev, cdc_0_int_handler);

	write_data(cdc_dev, WELCOME_MSG, strlen(WELCOME_MSG));
	write_data(cdc_dev, cdc_dev->name, strlen(cdc_dev->name));

	uart_irq_rx_enable(cdc_dev);

	erpc_transport_t arbitrated_transport;
	erpc_transport_t transport = reinterpret_cast<erpc_transport_t>(
		new erpc::ZephyrUsbCdcTransport(cdc_dev)
	);
	erpc_mbf_t message_buffer_factory = erpc_mbf_dynamic_init();

	erpc_client_t client = erpc_arbitrated_client_init(transport, message_buffer_factory, &arbitrated_transport);


	while (true) {
		k_sleep(K_MSEC(100));
	}

}
