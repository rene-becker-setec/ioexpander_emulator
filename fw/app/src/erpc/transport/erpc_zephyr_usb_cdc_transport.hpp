#include <zephyr/device.h>
#include "erpc_framed_transport.hpp"


namespace erpc {

    class ZephyrUsbCdcTransport : public FramedTransport

public:
    /*!
     * @brief Constructor.
     *
     * @param[in] portName Port name.
     * @param[in] baudRate Baudrate.
     */
    ZephyrUsbCdcTransport(const struct device *device);

    /*!
     * @brief Destructor.
     */
    virtual ~ZephyrUsbCdcTransport(void);

private:
    /*!
     * @brief Receive data from UART peripheral.
     *
     * @param[inout] data Preallocated buffer for receiving data.
     * @param[in] size Size of data to read.
     *
     * @retval kErpcStatus_ReceiveFailed failed to receive data.
     * @retval kErpcStatus_Success Successfully received all data.
     */
    virtual erpc_status_t underlyingReceive(uint8_t *data, uint32_t size);

    /*!
     * @brief Write data to UART peripheral.
     *
     * @param[in] data Buffer to send.
     * @param[in] size Size of data to send.
     *
     * @retval kErpcStatus_Success Always returns success status.
     */
    virtual erpc_status_t underlyingSend(const uint8_t *data, uint32_t size);

}
