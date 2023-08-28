"""

"""
import logging
import time
from ioxp_emu import IoxpEmu
from rvmc import Rvmc

# USB_CDC_PORT = '/dev/serial/by-id/usb-ZEPHYR_IOExp_668271B5BCD6B09D-if00'
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

    ioxp_emu = IoxpEmu(USB_CDC_PORT)
    rvmc = Rvmc(ioxp_emu)
    rvmc.start()

    time.sleep(1)

    LOGGER.info('Setting Node into pairing mode ...')
    rvmc.set_pairing_mode()

    LOGGER.info('Node should now be in pairing mode ...')
    time.sleep(30)

    LOGGER.info('Done')
