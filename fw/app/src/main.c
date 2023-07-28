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

#define LOG_LEVEL LOG_LEVEL_DBG
LOG_MODULE_REGISTER(main);


#define MOUSE_BTN_REPORT_POS	0
#define MOUSE_X_REPORT_POS	1
#define MOUSE_Y_REPORT_POS	2

#define MOUSE_BTN_LEFT		BIT(0)
#define MOUSE_BTN_RIGHT		BIT(1)
#define MOUSE_BTN_MIDDLE	BIT(2)


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
	const struct device *cdc_dev[] = {
		DT_FOREACH_STATUS_OKAY(zephyr_cdc_acm_uart, DEVICE_AND_COMMA)
	};
	BUILD_ASSERT(ARRAY_SIZE(cdc_dev) >= 2, "Not enough CDC ACM instances");
	uint32_t dtr = 0U;
	int ret;

	for (int idx = 0; idx < ARRAY_SIZE(cdc_dev); idx++) {
		if (!device_is_ready(cdc_dev[idx])) {
			LOG_ERR("CDC ACM device %s is not ready",
				cdc_dev[idx]->name);
			return 0;
		}
	}

	ret = usb_enable(status_cb);
	if (ret != 0) {
		LOG_ERR("Failed to enable USB");
		return 0;
	}

	/* Initialize CDC ACM */
	for (int idx = 0; idx < ARRAY_SIZE(cdc_dev); idx++) {
		LOG_INF("Wait for DTR on %s", cdc_dev[idx]->name);
		while (1) {
			uart_line_ctrl_get(cdc_dev[idx],
					   UART_LINE_CTRL_DTR,
					   &dtr);
			if (dtr) {
				break;
			} else {
				/* Give CPU resources to low priority threads. */
				k_sleep(K_MSEC(100));
			}
		}

		LOG_INF("DTR on device %s", cdc_dev[idx]->name);
	}


	LOG_INF("Waiting for 1s");
	/* Wait 1 sec for the host to do all settings */
	k_busy_wait(USEC_PER_SEC);

	uart_irq_callback_set(cdc_dev[0], cdc_0_int_handler);
	uart_irq_callback_set(cdc_dev[1], cdc_1_int_handler);

	write_data(cdc_dev[0], WELCOME_MSG, strlen(WELCOME_MSG));
	write_data(cdc_dev[0], cdc_dev[0]->name, strlen(cdc_dev[0]->name));
	write_data(cdc_dev[1], WELCOME_MSG, strlen(WELCOME_MSG));
	write_data(cdc_dev[1], cdc_dev[1]->name, strlen(cdc_dev[1]->name));

	uart_irq_rx_enable(cdc_dev[0]);
	uart_irq_rx_enable(cdc_dev[1]);

	while (true) {
		k_sleep(K_MSEC(100));
	}

}
