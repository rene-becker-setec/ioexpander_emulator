#include "erpc_zephyr_usb_cdc_transport.hpp"

using namespace erpc;

ZephyrUsbCdcTransport::ZephyrUsbCdcTransport(const struct device *device)
{
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
