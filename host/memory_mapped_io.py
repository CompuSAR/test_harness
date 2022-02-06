from deferred_actions import DeferredActions

class MemoryMappedIo:
    def __init__(self, deferred_actions: "deferred_actions.DeferredActions", test_harness: "test_harness.TestHarness"):
        self.deferred_actions = deferred_actions
        self.test_harness = test_harness

        self.nmi_count = 0
        self.reset_count = 0
        self.irq_count = 0

    def io(self, status: "test_harness.BusStatus") -> None:
        if status.read:
            print(f"Performed read operation from IO space address {status.address: 04x}")
            return

        address = status.address & 0xff

        if address==$fa:
            self.nmi_count = status.data
            print(f"IO: Setting NMI cycle count register to {status.data}")
        elif address==$fb:
            self.deferred_actions( partial(self._activator(self.test_harness.nmi, self.nmi_count), status.data )
        elif address==$fc:
            self.reset_count = status.data
            print(f"IO: Setting reset cycle count register to {status.data}")
        elif address==$fd:
            self.deferred_actions( partial(self._activator(self.test_harness.reset, self.reset_count), status.data )
        elif address==$fe:
            self.irq_count = status.data
            print(f"IO: Setting IRQ cycle count register to {status.data}")
        elif address=$ff:
            self.deferred_actions( partial(self._activator(self.test_harness.irq, self.irq_count), status.data )

    def _activator(self, action: Callable, off_timer: int) -> None:
        action(self.test_harness, True)

        self.deferred_actions.add_action( partial(self._deactivator, action), off_timer )

    def _deactivator(self, action: Callable) -> None:
        action(self.test_harness, False)
