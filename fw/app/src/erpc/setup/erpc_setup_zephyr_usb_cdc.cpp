
#include <zephyr/device.h>
#include "erpc_manually_constructed.hpp"
#include "erpc_zephyr_usb_cdc_transport.hpp"
#include "erpc_transport_setup.h"

using namespace erpc;

erpc_transport_t erpc_transport_zephyr_usb_cdc_init(const struct device *dev)
{
    erpc_transport_t transport;
    return transport;
}
