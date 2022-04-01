from typing import Callable
from functools import partial

from deferred_actions import DeferredActions

class MemoryMappedIo:
    def __init__(self, deferred_actions: "deferred_actions.DeferredActions", test_harness: "test_harness.TestHarness"):
        self.deferred_actions = deferred_actions
        self.test_harness = test_harness

        self.nmi_count = 0
        self.reset_count = 0
        self.irq_count = 0
        self.finished = False

    def io(self, status: "test_harness.BusStatus") -> None:
        if status.read:
            print(f"Performed read operation from IO space address {status.address: 04x}")
            return

        address = status.address & 0xff

        if address==0x00:
            self.finished = True
            print("Test done triggered")
        elif address==0x80:
            self.ready_count = status.data
            print(f"IO: Setting READY cycle count register to {status.data}")
        elif address==0x81:
            self.deferred_actions.add_action( partial(self._activator, self.test_harness.ready, self.ready_count), status.data )
        elif address==0x82:
            self.set_overflow_count = status.data
            print(f"IO: Setting SO cycle count register to {status.data}")
        elif address==0x83:
            self.deferred_actions.add_action( partial(self._activator, self.test_harness.setOverflow, self.set_overflow_count), status.data )
        elif address==0xfa:
            self.nmi_count = status.data
            print(f"IO: Setting NMI cycle count register to {status.data}")
        elif address==0xfb:
            self.deferred_actions.add_action( partial(self._activator, self.test_harness.nmi, self.nmi_count), status.data )
        elif address==0xfc:
            self.reset_count = status.data
            print(f"IO: Setting reset cycle count register to {status.data}")
        elif address==0xfd:
            self.deferred_actions.add_action( partial(self._activator, self.test_harness.reset, self.reset_count), status.data )
        elif address==0xfe:
            self.irq_count = status.data
            print(f"IO: Setting IRQ cycle count register to {status.data}")
        elif address==0xff:
            self.deferred_actions.add_action( partial(self._activator, self.test_harness.irq, self.irq_count), status.data )
        else:
            print(f"Unknown IO {address:02x}")

    def _activator(self, action: Callable, off_timer: int) -> None:
        action(True)

        self.deferred_actions.add_action( partial(self._deactivator, action), off_timer )

    def _deactivator(self, action: Callable) -> None:
        action(False)
