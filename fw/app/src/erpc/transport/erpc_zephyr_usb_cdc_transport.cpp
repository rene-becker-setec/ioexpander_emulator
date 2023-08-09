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

K_PIPE_DEFINE(rx_data_pipe, 1024, 4);


static void write_data(const struct device *dev, const uint8_t *buf, int len)
{
	uart_irq_tx_enable(dev);

	while (len) {
		int written;

		data_transmitted = false;
		written = uart_fifo_fill(dev, buf, len);
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
    size_t pipe_free = 0;

	uart_irq_update(dev);

	if (uart_irq_tx_ready(dev)) {
		data_transmitted = true;
	}

	if (!uart_irq_rx_ready(dev)) {
		return;
	}
	uint32_t bytes_read;
	uint8_t rx_buffer;

	while (true) {
        int rc;
        size_t bytes_written_to_pipe;

        pipe_free = k_pipe_write_avail(&rx_data_pipe);
        if (pipe_free == 0) {
            LOG_ERR("No space left in pipe");
            break;
        }

        bytes_read = uart_fifo_read(dev, &rx_buffer, 1);
        if (bytes_read == 0)
        {
            break;
        }

		LOG_INF("char received on CDC0: 0x%02x (%c)", rx_buffer, (char) rx_buffer);
        rc = k_pipe_put(&rx_data_pipe, &rx_buffer, 1, &bytes_written_to_pipe, 1, K_NO_WAIT);

	}


}


ZephyrUsbCdcTransport::ZephyrUsbCdcTransport(const struct device *cdc_dev): cdc_dev {cdc_dev}
{

    uart_irq_callback_set(this->cdc_dev, cdc_0_int_handler);
    uart_irq_rx_enable(this->cdc_dev);
    return;
}


ZephyrUsbCdcTransport::~ZephyrUsbCdcTransport(void)
{
    return;
}


erpc_status_t ZephyrUsbCdcTransport::underlyingReceive(uint8_t *data, uint32_t size)
{
    size_t num_bytes_read;
    LOG_INF("underlying receive running ...");

    k_pipe_get(&rx_data_pipe,(void*) data, size, &num_bytes_read, size, K_FOREVER);
    LOG_INF("read %d bytes", num_bytes_read);
    return kErpcStatus_Success;
    //return kErpcStatus_ReceiveFailed;
}

erpc_status_t ZephyrUsbCdcTransport::underlyingSend(const uint8_t *data, uint32_t size)
{
    LOG_INF("underlying send running ...");

    write_data(this->cdc_dev, data, size);
    return kErpcStatus_Success;
}
