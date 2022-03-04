from typing import Set

class TestPlanWriter:
    def __init__(self, output_file):
        self._output_file = output_file

    def bus_op(self, status: "test_harness.BusStatus") -> None:
        binary_flags = 0
        if status.read:
            binary_flags |= 0x01
        if "SYNC" in status.flags:
            binary_flags |= 0x02
        if "MemoryLock" in status.flags:
            binary_flags |= 0x04
        if "VectorPull" in status.flags:
            binary_flags |= 0x08

        out_str = f"1_{status.address:04x}_{status.data:02x}_{binary_flags:02x}"
        if status.opcode:
            out_str += f"    // {status.opcode}"
        print(out_str, file=self._output_file)

    def wait(self, num_cycles: int) -> None:
        print(f"0_0000_00_{num_cycles:02x}", file=self._output_file)
