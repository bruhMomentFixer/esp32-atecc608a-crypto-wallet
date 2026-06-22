#!/usr/bin/env python3
import serial
import sys
import tty
import termios
import select
import time
from threading import Thread


class WalletTerminal:
    def __init__(self, port='/dev/ttyUSB0', baudrate=115200):
        self.ser = serial.Serial(port, baudrate, timeout=0)
        self.ser.dtr = False
        time.sleep(0.1)
        self.ser.dtr = True
        self.stdin_fd = sys.stdin.fileno()
        self.original_termios = termios.tcgetattr(self.stdin_fd)
        self.running = True

    # ---------- TERMINAL SETUP / RESTORE ----------

    def enter_terminal_mode(self):
        # Alternate screen
        sys.stdout.write("\033[?1049h")
        sys.stdout.write("\033[?25l")  # hide cursor
        sys.stdout.flush()

        # Raw mode
        tty.setraw(self.stdin_fd)

    def restore_terminal(self):
        sys.stdout.write("\033[?25h")   # show cursor
        sys.stdout.write("\033[?1049l") # leave alt screen
        sys.stdout.flush()

        termios.tcsetattr(self.stdin_fd, termios.TCSADRAIN, self.original_termios)


    # ---------- INPUT HANDLING ----------

    def input_loop(self):
        while self.running:
            r, _, _ = select.select([sys.stdin], [], [], 0.05)
            if not r:
                continue

            ch = sys.stdin.read(1)
            if not ch:
                continue

            # Quit
            if ch == 'q':
                self.running = False
                return

            # Escape sequence (arrows)
            if ch == '\x1b':
                self.handle_escape_sequence()
                continue

            # Enter
            if ch in ('\r', '\n'):
                self.ser.write(b'\r')
                continue

            # Backspace
            if ch == '\x7f':
                self.ser.write(b'\x7f')
                continue

            # Digits
            if ch.isdigit():
                self.ser.write(ch.encode())
                continue

    def handle_escape_sequence(self):
        # Lee lo que venga inmediatamente (sin sleep ni select lento)
        next_bytes = sys.stdin.read(2)  # normalmente '[A', '[B', etc.
    
        if len(next_bytes) >= 2 and next_bytes[0] == '[':
            direction = next_bytes[1]
            arrows = {'A': b'U','B': b'D','C': b'R','D': b'L'}
            if direction in arrows:
                self.ser.write(f'\x1b[{direction}'.encode())  # envía al dispositivo
                return
    
        # Si no es flecha válida, no hace nada (no sale del programa)
       
   # ---------- SERIAL OUTPUT ----------

    def serial_loop(self):
        while self.running:
            try:
                data = self.ser.read(1024)
                if data:
                    sys.stdout.buffer.write(data)
                    sys.stdout.flush()
            except Exception:
                pass

            time.sleep(0.001)

    # ---------- MAIN ----------

    def run(self):
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


if __name__ == "__main__":
    WalletTerminal().run()

