#!/usr/bin/env python3
"""
Cross-platform serial terminal helper for the ESP32 ATECC608A wallet prototype.

Supported systems:
    - Windows: uses msvcrt
    - Linux/macOS: uses termios, tty and select

Usage:
    Windows:
        py wallet_terminal_crossplatform.py COM5
        py wallet_terminal_crossplatform.py --list

    Linux:
        python3 wallet_terminal_crossplatform.py /dev/ttyUSB0
        python3 wallet_terminal_crossplatform.py --list

Controls:
    Arrow keys  -> navigation
    Enter       -> select / confirm
    Backspace   -> delete
    Digits      -> PIN/input digits
    q           -> quit
"""

import argparse
import os
import platform
import sys
import time
from threading import Thread

import serial
from serial.tools import list_ports


ESC_UP = b"\x1b[A"
ESC_DOWN = b"\x1b[B"
ESC_RIGHT = b"\x1b[C"
ESC_LEFT = b"\x1b[D"


def list_serial_ports() -> None:
    ports = list(list_ports.comports())

    if not ports:
        print("No serial ports found.")
        return

    for port in ports:
        print(f"{port.device}")
        print(f"  Description : {port.description}")
        print(f"  HWID        : {port.hwid}")
        print()


def default_port() -> str:
    system = platform.system().lower()

    if system == "windows":
        return "COM5"

    if system == "darwin":
        return "/dev/tty.usbserial"

    return "/dev/ttyUSB0"


class BaseWalletTerminal:
    def __init__(self, port: str, baudrate: int = 115200, reset_on_connect: bool = True):
        self.port = port
        self.baudrate = baudrate
        self.running = True

        self.ser = serial.Serial(port, baudrate, timeout=0.05)

        if reset_on_connect:
            self.toggle_dtr_reset()

    def toggle_dtr_reset(self) -> None:
        """
        Some ESP32 boards reset when DTR changes after opening the serial port.
        This is useful in many cases, but can be disabled with --no-reset.
        """
        try:
            self.ser.dtr = False
            time.sleep(0.1)
            self.ser.dtr = True
            time.sleep(0.2)
        except Exception:
            pass

    def serial_loop(self) -> None:
        while self.running:
            try:
                data = self.ser.read(1024)
                if data:
                    sys.stdout.buffer.write(data)
                    sys.stdout.flush()
            except Exception as exc:
                print(f"\n[serial error] {exc}")
                self.running = False
                return

            time.sleep(0.001)

    def send_enter(self) -> None:
        self.ser.write(b"\r")

    def send_backspace(self) -> None:
        self.ser.write(b"\x7f")

    def send_digit(self, digit: str) -> None:
        self.ser.write(digit.encode("ascii"))

    def run(self) -> None:
        raise NotImplementedError

    def close(self) -> None:
        self.running = False
        time.sleep(0.05)
        self.ser.close()


class WindowsWalletTerminal(BaseWalletTerminal):
    def input_loop(self) -> None:
        import msvcrt

        while self.running:
            if not msvcrt.kbhit():
                time.sleep(0.01)
                continue

            ch = msvcrt.getwch()

            if ch.lower() == "q":
                self.running = False
                return

            # Arrow/function keys in Windows console are prefixed by \x00 or \xe0
            if ch in ("\x00", "\xe0"):
                key = msvcrt.getwch()
                arrows = {
                    "H": ESC_UP,
                    "P": ESC_DOWN,
                    "M": ESC_RIGHT,
                    "K": ESC_LEFT,
                }
                if key in arrows:
                    self.ser.write(arrows[key])
                continue

            if ch in ("\r", "\n"):
                self.send_enter()
                continue

            if ch == "\b":
                self.send_backspace()
                continue

            if ch.isdigit():
                self.send_digit(ch)
                continue

    def run(self) -> None:
        print("ESP32 Wallet Terminal - Windows mode")
        print("Controls: arrows = navigation, Enter = select/confirm, Backspace = delete, q = quit")
        print("-" * 78)

        input_thread = Thread(target=self.input_loop, daemon=True)
        input_thread.start()

        try:
            self.serial_loop()
        except KeyboardInterrupt:
            self.running = False
        finally:
            self.close()
            print("\nTerminal closed.")


class UnixWalletTerminal(BaseWalletTerminal):
    def __init__(self, port: str, baudrate: int = 115200, reset_on_connect: bool = True):
        super().__init__(port, baudrate, reset_on_connect)
        self.stdin_fd = sys.stdin.fileno()
        self.original_termios = None

    def enter_terminal_mode(self) -> None:
        import termios
        import tty

        self.original_termios = termios.tcgetattr(self.stdin_fd)

        # Alternate screen + hide cursor
        sys.stdout.write("\033[?1049h")
        sys.stdout.write("\033[?25l")
        sys.stdout.flush()

        tty.setraw(self.stdin_fd)

    def restore_terminal(self) -> None:
        import termios

        # Show cursor + leave alternate screen
        sys.stdout.write("\033[?25h")
        sys.stdout.write("\033[?1049l")
        sys.stdout.flush()

        if self.original_termios is not None:
            termios.tcsetattr(self.stdin_fd, termios.TCSADRAIN, self.original_termios)

    def input_loop(self) -> None:
        import select

        while self.running:
            r, _, _ = select.select([sys.stdin], [], [], 0.05)
            if not r:
                continue

            ch = sys.stdin.read(1)
            if not ch:
                continue

            if ch == "q":
                self.running = False
                return

            if ch == "\x1b":
                self.handle_escape_sequence()
                continue

            if ch in ("\r", "\n"):
                self.send_enter()
                continue

            if ch == "\x7f":
                self.send_backspace()
                continue

            if ch.isdigit():
                self.send_digit(ch)
                continue

    def handle_escape_sequence(self) -> None:
        next_bytes = sys.stdin.read(2)

        if len(next_bytes) >= 2 and next_bytes[0] == "[":
            direction = next_bytes[1]
            arrows = {
                "A": ESC_UP,
                "B": ESC_DOWN,
                "C": ESC_RIGHT,
                "D": ESC_LEFT,
            }
            if direction in arrows:
                self.ser.write(arrows[direction])

    def run(self) -> None:
        try:
            self.enter_terminal_mode()

            input_thread = Thread(target=self.input_loop, daemon=True)
            input_thread.start()

            self.serial_loop()

        except KeyboardInterrupt:
            self.running = False

        finally:
            self.running = False
            time.sleep(0.05)
            self.restore_terminal()
            self.ser.close()


def create_terminal(port: str, baudrate: int, reset_on_connect: bool) -> BaseWalletTerminal:
    system = platform.system().lower()

    if system == "windows":
        return WindowsWalletTerminal(port, baudrate, reset_on_connect)

    return UnixWalletTerminal(port, baudrate, reset_on_connect)


def main() -> None:
    parser = argparse.ArgumentParser(
        description="Cross-platform serial terminal helper for ESP32 wallet prototype."
    )
    parser.add_argument(
        "port",
        nargs="?",
        default=None,
        help="Serial port, for example COM5 on Windows or /dev/ttyUSB0 on Linux.",
    )
    parser.add_argument(
        "--baud",
        type=int,
        default=115200,
        help="Serial baudrate. Default: 115200.",
    )
    parser.add_argument(
        "--list",
        action="store_true",
        help="List available serial ports and exit.",
    )
    parser.add_argument(
        "--no-reset",
        action="store_true",
        help="Do not toggle DTR after opening the serial port.",
    )

    args = parser.parse_args()

    if args.list:
        list_serial_ports()
        return

    port = args.port or default_port()

    print(f"Detected OS : {platform.system()}")
    print(f"Serial port : {port}")
    print(f"Baudrate    : {args.baud}")
    print()

    try:
        terminal = create_terminal(port, args.baud, reset_on_connect=not args.no_reset)
        terminal.run()
    except serial.SerialException as exc:
        print(f"Could not open serial port {port}: {exc}")
        print()
        print("Try listing available ports with:")
        if platform.system().lower() == "windows":
            print("  py wallet_terminal_crossplatform.py --list")
        else:
            print("  python3 wallet_terminal_crossplatform.py --list")
        sys.exit(1)


if __name__ == "__main__":
    main()
