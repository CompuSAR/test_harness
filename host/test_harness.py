import serial
import time


class TestHarness:
    _BAUDRATE = 115200
    _TIMEOUT = 2

    def __init__(self, port_path: str):
        print(f"Setting up test harness on {port_path}")
        self.port = serial.Serial(
                port_path, baudrate=TestHarness._BAUDRATE, timeout=TestHarness._TIMEOUT, write_timeout=TestHarness._TIMEOUT)

        self.hard_reset()

        time.sleep(2)  # We may have just reset the Arduino. Give it time to reload
        line = None
        print("< d")
        self.port.write(b"d\n")
        while line != "d":
            line = self.get_line()
        self.get_line() # status result
        print("Open")

    def hard_reset(self):
        self.port.dtr = False
        time.sleep(0.5)
        self.port.dtr = True

    def get_line(self):
        line = self.port.read_until()
        if not line:
            raise serial.SerialTimeoutException()

        line = line.decode().rstrip('\r\n')
        print(f"> {line}")

        return line
