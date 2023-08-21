"""

"""


class RvcMsgId:
    def __init__(self, can_id: int) -> None:
        self.sa = (can_id & 0xFF)
        self.dgn = (can_id >> 8) & 0x3FFFF
        self.pri = (can_id >> 26) & 0x07

    def __str__(self):
        return f'SA:0x{self.sa:02x} - DGN:0x{self.dgn:03x} - PRI:{self.pri}'

    def as_int(self) -> int:
        return self.sa | self.dgn << 8 | self.pri << 26


class RvcMsg:
    def __init__(self, msg_id: RvcMsgId, msg_data: list[int]) -> None:
        self.id = msg_id
        self.data = msg_data

    def __str__(self):
        return f'RV-C Msg: id={self.id} - data={self.data}'
