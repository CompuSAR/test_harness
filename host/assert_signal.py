from enum import Enum

from mem_file import array_to_number


class Signals(Enum):
    Reset = 0
    IRQ = 1
    NMI = 2

class AssertSignal:
    def __init__(self, operation: tuple[int]):
        self.pin = Signals(operation[1])
        self.cycles_until_on = array_to_number( operation[2:4] )
        self.cycles_until_off = array_to_number( operation[4:6] )
