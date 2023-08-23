
import logging
import time
import threading
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
        self._data_lock = threading.Lock()
        self._api_lock = threading.Lock()
        self._switch_data = [0x00, 0x00, 0x00, 0x00]
        self._stop_req = threading.Event()
        self._swst_thread_handle = None

    def start(self):
        self._swst_thread_handle = threading.Thread(target=self._thread_func)
        self._swst_thread_handle.start()

    def stop(self):
        if self._swst_thread_handle is None:
            return
        self._stop_req.set()
        self._swst_thread_handle.join()
        self._swst_thread_handle = None

    def _thread_func(self):
        while True:
            if self._stop_req.is_set():
                break
            time.sleep(SWITCH_MSG_DELAY)
            with self._data_lock:
                rvc_payload = self._switch_data + [0xFF, 0x33, self._rolling_counter & 0xFF]
                rvc_payload += [sum(rvc_payload) & 0xFF]
                assert len(rvc_payload) == 8, f'Payload length should be 8 but is {len(rvc_payload)}'
                msg = can.rvc.RvcMsg(
                    msg_id=can.rvc.RvcMsgId(0x18ef5444),
                    msg_data=rvc_payload
                )
                self._bus.send_can_msg(msg)
                self._rolling_counter += 1

    def increment_menu(self) -> None:
        LOGGER.debug('Incrementing Menu ...')
        with self._data_lock, self._api_lock:
            self._switch_data = [0x00, 0x00, 0x00, 0x40]
            time.sleep(0.5)
            self._switch_data = [0x00, 0x00, 0x00, 0x00]
            time.sleep(0.5)
        LOGGER.debug('Incrementing Menu ... Done')

    def press_extend(self) -> None:
        LOGGER.debug('Pressing EXT ...')
        with self._data_lock, self._api_lock:
            self._switch_data = [0x00, 0x10 , 0x00, 0x00]
            time.sleep(0.5)
            self._switch_data = [0x00, 0x10, 0x00, 0x00]
            time.sleep(0.5)
        LOGGER.debug('Pressing EXT ... done')

    def on_msg_rcvd(self, msg: can.rvc.RvcMsg) -> None:
        LOGGER.debug(f'Message received: {msg} ({msg.id.as_int():08x})')
        if msg.id.as_int() == 0x18EF4454:
            if self._display_state != msg.data[1:3]:
                LOGGER.debug(f'New Display State: {self._display_state} -> {msg.data[1:3]}')
            self._display_state = msg.data[1:3]

    def set_pairing_mode(self) -> None:
        LOGGER.debug('Starting to set Node into Pairing Mode')
        while self._display_state != bytearray([0x77, 0x67]):
            LOGGER.debug('Incrementing Menu')
            self.increment_menu()
            time.sleep(1)
        LOGGER.debug('Display should now be showing "PA"')
        self.press_extend()

