"""

"""
import erpc
import logging
import time
import random
import ioxp.ioxp_pc2emu as pc2emu
import ioxp.ioxp_emu2pc as emu2pc

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

    xport = erpc.transport.SerialTransport(
        '/dev/serial/by-id/usb-ZEPHYR_IOExp_668271B5BCD6B09D-if00',
        baudrate=115200
    )

    arbitrator = erpc.arbitrator.TransportArbitrator(
        sharedTransport=xport, codec=erpc.basic_codec.BasicCodec()
    )

    #
    # Client for PC -> Emulator Communication
    #
    LOGGER.info('Starting up client ...')

    client_mngr = erpc.client.ClientManager(xport, erpc.basic_codec.BasicCodec)
    client_mngr.arbitrator = arbitrator

    client = pc2emu.client.IoExpanderEmulatorClient(client_mngr)

    LOGGER.info('Sending first message ...')
    response = client.sendCanMsg(b'hello world')
    LOGGER.info(f'received: {response}')

    for i in range(10):
        sleep_duration = random.randint(1, 50)/10
        LOGGER.info(f'Sleeping for {sleep_duration} seconds')
        time.sleep(sleep_duration)
        response = client.sendCanMsg(f'Injecting {i}'.encode())
        LOGGER.info(f'server responded with {response}')

    LOGGER.info("Done")