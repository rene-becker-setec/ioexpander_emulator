#include "erpc_zephyr_usb_cdc_transport.hpp"
#include <zephyr/kernel.h>
#include <zephyr/drivers/uart.h>
#include <zephyr/logging/log.h>

#define WELCOME_MSG "Welcome ..."

#define LOG_LEVEL LOG_LEVEL_INF
LOG_MODULE_REGISTER(erpcxprt);

using namespace erpc;

static volatile bool data_transmitted;
static volatile bool data_arrived;
static uint8_t rx_buffer[128];
static char dbg_buffer[512];


K_PIPE_DEFINE(rx_data_pipe, 1024, 4);

void to_hex_string(uint8_t * in, size_t in_size, char * out, size_t out_size)
{
    uint8_t* pin = in;
    const char* hex = "0123456789ABCDEF";
    char* pout = out;
    for(; pin < in+in_size; pout +=3, pin++){
        pout[0] = hex[(*pin>>4) & 0xF];
        pout[1] = hex[ *pin     & 0xF];
        pout[2] = ':';
        if ((size_t)(pout + 3 - out) > out_size){
            /* Better to truncate output string than overflow buffer */
            /* it would be still better to either return a status */
            /* or ensure the target buffer is large enough and it never happen */
            break;
        }
    }
    pout[-1] = 0;
}


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

	uart_irq_update(dev);

	if (uart_irq_tx_ready(dev)) {
		data_transmitted = true;
	}

	if (!uart_irq_rx_ready(dev)) {
		return;
	}

	while (true) {
        int rc;
        size_t bytes_written_to_pipe;
        uint32_t bytes_read;
        size_t bytes_to_read = k_pipe_write_avail(&rx_data_pipe);

        if (bytes_to_read == 0) {
            LOG_ERR("No space left in pipe");
            break;
        }

        if (bytes_to_read > 128)
        {
            bytes_to_read = 128;
        }


        bytes_read = uart_fifo_read(dev, rx_buffer, bytes_to_read);
        if (bytes_read == 0)
        {
            // No more data available ...
            break;
        }

        memset(dbg_buffer, 0, sizeof(dbg_buffer));
        to_hex_string(rx_buffer, bytes_read, dbg_buffer, sizeof(dbg_buffer));
		LOG_DBG("CDC0 tx: %s ", dbg_buffer);

        rc = k_pipe_put(&rx_data_pipe, rx_buffer, bytes_read, &bytes_written_to_pipe, bytes_read, K_NO_WAIT);
        if (rc != 0)
        {
            LOG_ERR("Something went wrong writing to pipe");
        }
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
    char debug_buffer[256];

    k_pipe_get(&rx_data_pipe,(void*) data, size, &num_bytes_read, size, K_FOREVER);

    memset(debug_buffer, 0, sizeof(debug_buffer));
    to_hex_string(data, size, debug_buffer, sizeof(debug_buffer));
    LOG_DBG("tprt recv: read %d bytes - %s", num_bytes_read, debug_buffer);
    return kErpcStatus_Success;
}

erpc_status_t ZephyrUsbCdcTransport::underlyingSend(const uint8_t *data, uint32_t size)
{
    char debug_buffer[256];

    write_data(this->cdc_dev, data, size);

    memset(debug_buffer, 0, sizeof(debug_buffer));
    to_hex_string((uint8_t*) data, size, debug_buffer, sizeof(debug_buffer));
    LOG_DBG("tprt send: wrote %d bytes - %s", size, debug_buffer);

    return kErpcStatus_Success;
}
