"""

"""
import erpc
import logging
import time
import random
import ioxp.ioxp_pc2emu as pc2emu
import ioxp.ioxp_emu2pc as emu2pc

#USB_CDC_PORT = '/dev/serial/by-id/usb-ZEPHYR_IOExp_668271B5BCD6B09D-if00'
USB_CDC_PORT = '/dev/serial/by-id/usb-ZEPHYR_IOExp_6276018CAE49AE37-if00'

logging.basicConfig(
    format='%(asctime)s.%(msecs)03d [%(levelname)8s] %(message)s (%(filename)s:%(lineno)s)',
    level=logging.INFO,
    handlers=[
        # logging.FileHandler(f'./{datetime.datetime.now().strftime("%Y-%m-%d_%H.%M.%S")}.log'),
        logging.StreamHandler()
    ]
)

LOGGER = logging.getLogger(__name__)


if __name__ == '__main__':

    LOGGER.info('Starting ...')

    xport = erpc.transport.SerialTransport(USB_CDC_PORT, baudrate=115200)

    arbitrator = erpc.arbitrator.TransportArbitrator(
        sharedTransport=xport, codec=erpc.basic_codec.BasicCodec()
    )

    ############################################################################
    #
    # Server for Emulator -> PC Communication
    #
    ############################################################################


    class Emu2PcHandler(emu2pc.interface.IIoExpanderEmulatorAsync):

        def canMsgRcvd(self, canMsg):
            data_formatted = ["0x%02x" % b for b in canMsg.data]
            LOGGER.info(f'CAN Message received 0x{canMsg.id:08x} - {data_formatted}')


    handler = Emu2PcHandler()
    service = emu2pc.server.IoExpanderEmulatorAsyncService(handler)
    server = erpc.simple_server.ServerThread(arbitrator, erpc.basic_codec.BasicCodec)
    server.add_service(service)

    LOGGER.info('Starting Server ...')
    server.start()

    ##############################################################################
    #
    # Client for PC -> Emulator Communication
    #
    ##############################################################################

    LOGGER.info('Starting up client ...')

    client_mngr = erpc.client.ClientManager(xport, erpc.basic_codec.BasicCodec)
    client_mngr.arbitrator = arbitrator

    client = pc2emu.client.IoExpanderEmulatorClient(client_mngr)

#    LOGGER.info('Sending first message ...')
#    response = client.sendCanMsg(b'hello world')
#    LOGGER.info(f'received: {response}')

    for i in range(10):
        sleep_duration = random.randint(1, 50)/10
        LOGGER.info(f'Sleeping for {sleep_duration} seconds')
        time.sleep(sleep_duration)
        response = client.sendCanMsg(pc2emu.common.rvc_msg_t(0xbeefabcd, [1, 2, 3, 4, 5, 6, 7, i]))
        LOGGER.info(f'server responded with {response}')

    LOGGER.info("Done")