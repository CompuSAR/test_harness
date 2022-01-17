from enum import Enum

class Signals(Enum):
    Reset = 0
    IRQ = 1
    NMI = 2

class AssertSignal:
    def __init__(self, operation: tuple[int]):
        self.pin = Signals(operation[1])
        self.cycles_until_on = operation[2]*16 + operation[3]
        self.cycles_until_off = operation[4]*16 + operation[5]
