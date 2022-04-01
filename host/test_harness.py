import re
import serial
import time

from typing import Optional

import colored


class BusStatus:
    def __init__(self, address: str, data: str, read: bool, flags: tuple[str], opcode: Optional[str] = None):
        self.address = int(address, 16)
        if data is not None:
            self.data = int(data, 16)
        self.read = read
        self.write = not read
        self.flags = set( [flag for flag in flags if flag] )
        self.opcode = opcode


class TestHarness:
    """
    A class for controlling a 6502 connected to the test harness.

    Parameters:
    port_path (str): The name of the serial port to which the test harness is connected.
    """
    _BAUDRATE = 1000000
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
            line = self.get_line()
        self.get_line() # status result
        print("Open")

    def hard_reset(self) -> None:
        """
        Performs a hard reset of the Arduino controlling the CPU (but not of the CPU itself).
        """
        self.port.dtr = False
        time.sleep(0.5)
        self.port.dtr = True

    def send_command(self, command_line: str) -> None:
        print(f"{colored.fore.BLUE}> {command_line}{colored.attr(0)}")
        self.port.write(f"{command_line}\n".encode())
        echo = self.get_line()
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

    def reset(self, state: bool) -> None:
        if state:
            self.send_command("R")

            self._wait_reply("Reset \s LOW")
        else:
            self.send_command("r")

            self._wait_reply("Reset \s HIGH")

    def irq(self, state: bool) -> None:
        if state:
            self.send_command("I")

            self._wait_reply("IRQ \s low")
        else:
            self.send_command("i")

            self._wait_reply("IRQ \s high")

    def nmi(self, state: bool) -> None:
        if state:
            self.send_command("N")

            self._wait_reply("NMI \s low")
        else:
            self.send_command("n")

            self._wait_reply("NMI \s high")

    def ready(self, state: bool) -> None:
        if state:
            self.send_command("W")

            self._wait_reply("Ready \s LOW")
        else:
            self.send_command("w")

            self._wait_reply("Ready \s HIGH")

    def setOverflow(self, state: bool) -> None:
        if state:
            self.send_command("V")

            self._wait_reply("Set \s overflow \s LOW")
        else:
            self.send_command("v")

            self._wait_reply("Set \s overflow \s HIGH")

    def cycle(self) -> BusStatus:
        self.send_command('c')
        self.get_status()
        return self.get_status()


    def get_line(self) -> str:
        """
        Reads a single line of response from the CPU.

        If no line is received within timeout, raises a SerialTimeoutException
        """
        line = self.port.read_until()
        if not line:
            raise serial.SerialTimeoutException()

        line = line.decode().rstrip('\r\n')
        print(f"{colored.fore.LIGHT_YELLOW}< {line}{colored.attr(0)}")

        return line


    def get_status(self) -> BusStatus:
        parsed = self._wait_reply(
                r'''
                ^
                (?P<cycle> [0-9]+) :\s
                A: (?P<address> [0-9a-fA-F]{4}) \s
                D: ( (?P<data> [0-9a-fA-F]{2}) | -- ) \s
                ( " (?P<opcode> [^"]+) " \s )?
                ( (?P<read> Read) | (?P<write> Write) )
                (?P<flags> .*)
                $
                ''')

        return BusStatus(
                parsed['address'],
                parsed['data'],
                parsed['read'] is not None,
                parsed['flags'].split(' '),
                parsed['opcode'] )


    def _wait_reply(self, expression: str) -> re.Match:
        while True:
            answer: str = self.get_line()
            parsed = re.match(expression, answer, re.X)

            if parsed:
                return parsed
