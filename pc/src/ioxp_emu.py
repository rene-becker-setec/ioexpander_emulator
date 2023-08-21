"""


"""
import erpc
import logging
import time
import random
from threading import Lock
import ioxp.ioxp_pc2emu as pc2emu
import ioxp.ioxp_emu2pc as emu2pc
from can.listener import Listener
from can.rvc import RvcMsgId, RvcMsg


LOGGER = logging.getLogger(__name__)


class IoxpEmu(emu2pc.interface.IIoExpanderEmulatorAsync):

    def __init__(self, port: str) -> None:

        self._listeners = []
        self._lock = Lock()
        self._xport = erpc.transport.SerialTransport(port, baudrate=115200)
        self._arbitrator = erpc.arbitrator.TransportArbitrator(
            sharedTransport=self._xport, codec=erpc.basic_codec.BasicCodec()
        )
        self._service = emu2pc.server.IoExpanderEmulatorAsyncService(self)
        self._server = erpc.simple_server.ServerThread(self._arbitrator, erpc.basic_codec.BasicCodec)
        self._server.add_service(self._service)
        self._server.start()

        self._client_mngr = erpc.client.ClientManager(self._xport, erpc.basic_codec.BasicCodec)
        self._client_mngr.arbitrator = self._arbitrator
        self._client = pc2emu.client.IoExpanderEmulatorClient(self._client_mngr)

    def canMsgRcvd(self, can_msg):
        msg = RvcMsg(RvcMsgId(can_msg.id), can_msg.data)
        LOGGER.debug(f'CAN Message received {msg}')
        for listener in self._listeners:
            listener.on_msg_rcvd(msg)

    def add_listener(self, listener: Listener) -> None:
        self._listeners.append(listener)

    def send_can_msg(self, msg: RvcMsg) -> int:
        with self._lock:
            response = self._client.sendCanMsg(
                pc2emu.common.rvc_msg_t(msg.id.as_int(), msg.data)
            )
            return response
