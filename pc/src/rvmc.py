
import logging
import time
import can.listener
import can.rvc
import ioxp_emu

LOGGER = logging.getLogger(__name__)

SWITCH_MSG_DELAY = 0.040


class Rvmc(can.listener.Listener):

    def __init__(self, bus: ioxp_emu.IoxpEmu) -> None:
        super().__init__()
        LOGGER.debug('creating RVMC instance ...')
        self._rolling_counter = 0
        self._display_state = None
        self._bus = bus
        self._bus.add_listener(self)

    def increment_menu(self) -> None:
        LOGGER.debug('Incrementing Menu ...')
        for i in range(0, 20):
            data = [0x00, 0x00, 0x00, 0x40 if i < 10 else 0x00, 0xFF, 0x33, self._rolling_counter & 0xFF]
            data += [sum(data) & 0xFF]

            msg = can.rvc.RvcMsg(
                msg_id=can.rvc.RvcMsgId(0x18ef5444),
                msg_data=data
            )

            self._bus.send_can_msg(msg)
            time.sleep(SWITCH_MSG_DELAY)
            self._rolling_counter += 1
        LOGGER.debug('Incrementing Menu ... Done')

    def press_extend(self) -> None:
        LOGGER.debug('Pressing EXT')
        for i in range(0, 20):
            data = [0x00, 0x10 if i < 10 else 0x00, 0x00, 0x00, 0xFF, 0x33, self._rolling_counter & 0xFF]
            data += [sum(data) & 0xFF]

            msg = can.rvc.RvcMsg(
                msg_id=can.rvc.RvcMsgId(0x18ef5444),
                msg_data=data
            )

            self._bus.send_can_msg(msg)
            time.sleep(SWITCH_MSG_DELAY)
            self._rolling_counter += 1

    def on_msg_rcvd(self, msg: can.rvc.RvcMsg) -> None:
        LOGGER.debug(f'Message received: {msg} ({msg.id.as_int():08x})')
        if msg.id.as_int() == 0x18EF4454:
            if self._display_state != msg.data[1:3]:
                LOGGER.debug(f'New Display State: {self._display_state} -> {msg.data[1:3]}')
            self._display_state = msg.data[1:3]

    def set_pairing_mode(self) -> None:
        LOGGER.debug('Starting to set Node into Pairing Mode')
        time.sleep(5)
        while self._display_state != bytearray([0x77, 0x67]):
            LOGGER.debug('Incrementing Menu')
            self.increment_menu()
            time.sleep(1)
        LOGGER.debug('Display should now be showing "PA"')
        self.press_extend()
        time.sleep(5)

