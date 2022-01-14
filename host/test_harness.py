import re
import serial
import time


class TestHarness:
    """
    A class for controlling a 6502 connected to the test harness.

    Parameters:
    port_path (str): The name of the serial port to which the test harness is connected.
    """
    _BAUDRATE = 115200
    _TIMEOUT = 2

    def __init__(self, port_path: str):
        print(f"Setting up test harness on {port_path}")
        self.port = serial.Serial(
                port_path, baudrate=TestHarness._BAUDRATE, timeout=TestHarness._TIMEOUT, write_timeout=TestHarness._TIMEOUT)

        self.hard_reset()

        time.sleep(2)  # We may have just reset the Arduino. Give it time to reload
        line = None
        print("> d")
        self.port.write(b"d\n")
        while line != "d":
            line = self._get_line()
        self._get_line() # status result
        print("Open")

    def hard_reset(self) -> None:
        """
        Performs a hard reset of the Arduino controlling the CPU (but not of the CPU itself).
        """
        self.port.dtr = False
        time.sleep(0.5)
        self.port.dtr = True

    def send_command(self, command_line: str) -> None:
        print(f"> {command_line}")
        self.port.write(f"{command_line}\n".encode())
        echo = self._get_line()
        assert echo==command_line, "Mismatch between command sent and echo received"

    def read_memory(self, address: int) -> int:
        """
        Read a single byte of memory.

        Parameters:
        address (int): The address to read from.

        Returns:
        int: The byte read.
        """
        self.send_command(f"m {address:04x}")

        parsed = self._wait_reply(r'(?P<address> [0-9a-fA-F]{4}): \s (?P<data> [0-9a-fA-F]{2})')

        assert int(parsed['address'], 16)==address, "Read from incorrect address"

        return int(parsed['data'], 16)

    def write_memory(self, address: int, data: int) -> None:
        """
        Write a single byte to memory.

        Parameters:
        address (int): The address to write to.
        data (int): The byte to write.
        """
        self.send_command(f"M {address:04x},{data:02x}")

        parsed = self._wait_reply(r'Written \s (?P<data> [0-9a-fA-F]{2}) \s to \s (?P<address> [0-9a-fA-F]{4})')

        assert int(parsed['address'], 16) == address
        assert int(parsed['data'], 16) == data

    def _wait_reply(self, expression: str) -> re.Match:
        while True:
            answer: str = self._get_line()
            parsed = re.match(expression, answer, re.X)

            if parsed:
                return parsed

    def _get_line(self) -> str:
        line = self.port.read_until()
        if not line:
            raise serial.SerialTimeoutException()

        line = line.decode().rstrip('\r\n')
        print(f"< {line}")

        return line
