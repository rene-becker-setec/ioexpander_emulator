#include "erpc_zephyr_usb_cdc_transport.hpp"
#include <zephyr/kernel.h>
#include <zephyr/drivers/uart.h>
#include <zephyr/logging/log.h>

#define WELCOME_MSG "Welcome ..."

#define LOG_LEVEL LOG_LEVEL_DBG
LOG_MODULE_REGISTER(erpcxprt);

using namespace erpc;

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


ZephyrUsbCdcTransport::ZephyrUsbCdcTransport(const struct device *cdc_dev)
{

    uart_irq_callback_set(cdc_dev, cdc_0_int_handler);
    write_data(cdc_dev, WELCOME_MSG, strlen(WELCOME_MSG));
    uart_irq_rx_enable(cdc_dev);
    return;
}


ZephyrUsbCdcTransport::~ZephyrUsbCdcTransport(void)
{
    return;
}


erpc_status_t ZephyrUsbCdcTransport::underlyingReceive(uint8_t *data, uint32_t size)
{
    return kErpcStatus_Success;
}

erpc_status_t ZephyrUsbCdcTransport::underlyingSend(const uint8_t *data, uint32_t size)
{
    return kErpcStatus_Success;
}
