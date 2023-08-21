"""

"""
from abc import ABC, abstractmethod
from rvc import RvcMsg


class Listener(ABC):

    @abstractmethod
    def on_msg_rcvd(self, msg: RvcMsg):
        pass
