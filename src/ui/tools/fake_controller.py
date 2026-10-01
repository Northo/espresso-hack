#!/usr/bin/env -S uv run --script
# /// script
# requires-python = ">=3.9"
# dependencies = ["pyserial"]
# ///
"""Fake controller for the display's serial link (env T-RGB-serial).

Sends state messages and prints/reacts to commands from the display.
Protocol: see src/link/protocol.h. Frame: 0x7E | len | payload | crc8(len+payload).

Usage: uv run tools/fake_controller.py /dev/cu.usbmodemXXXX [--rate 2]
(dependencies are declared inline, uv installs them automatically)
"""
import argparse
import struct
import time

import serial

START = 0x7E
MSG_STATE, MSG_COMMAND = 1, 2
FLAG_BREWING, FLAG_STEAM = 1, 2
CMD_BREW_START, CMD_STEAM_TOGGLE, CMD_SET_SETPOINT, CMD_SET_BREW_DURATION = 1, 2, 3, 4
CMD_NAMES = {1: "BREW_START", 2: "STEAM_TOGGLE", 3: "SET_SETPOINT", 4: "SET_BREW_DURATION"}
STEAM_TEMP = 130.0


def crc8(data: bytes, crc: int = 0) -> int:
    for b in data:
        crc ^= b
        for _ in range(8):
            crc = ((crc << 1) ^ 0x07) & 0xFF if crc & 0x80 else (crc << 1) & 0xFF
    return crc


def frame(payload: bytes) -> bytes:
    n = bytes([len(payload)])
    return bytes([START]) + n + payload + bytes([crc8(payload, crc8(n))])


class Parser:
    """Incremental frame parser, mirrors transport_serial.cpp."""

    def __init__(self):
        self.buf = bytearray()

    def feed(self, data: bytes):
        self.buf += data
        while True:
            try:
                i = self.buf.index(START)
            except ValueError:
                self.buf.clear()
                return
            del self.buf[:i]
            if len(self.buf) < 2:
                return
            n = self.buf[1]
            if n == 0 or n > 16:
                del self.buf[0]
                continue
            if len(self.buf) < n + 3:
                return
            payload = bytes(self.buf[2:2 + n])
            if self.buf[2 + n] == crc8(payload, crc8(bytes([n]))):
                del self.buf[:n + 3]
                yield payload
            else:
                del self.buf[0]


class FakeMachine:
    def __init__(self):
        self.temp = 25.0
        self.setpoint = 93.0
        self.brew_duration = 25
        self.steam = False
        self.brew_until = 0.0
        self.brew_start = 0.0

    def target(self):
        return STEAM_TEMP if self.steam else self.setpoint

    def step(self, dt: float):
        # Crude first-order heating / cooling towards the target.
        self.temp += (self.target() - self.temp) * min(1.0, 0.05 * dt) + (0.8 * dt if self.temp < self.target() else 0)
        if self.temp > self.target():
            self.temp -= 0.2 * dt

    def state_payload(self) -> bytes:
        brewing = time.time() < self.brew_until
        flags = (FLAG_BREWING if brewing else 0) | (FLAG_STEAM if self.steam else 0)
        elapsed = max(0.0, time.time() - self.brew_start) if brewing else 0.0
        stage = 0
        if brewing:
            stage = 1 if elapsed < 2 else 2 if elapsed < 7 else 3
        return struct.pack("<BhhHBHB", MSG_STATE, round(self.temp * 10), round(self.target() * 10),
                           self.brew_duration, flags, int(elapsed), stage)

    def handle(self, payload: bytes):
        if len(payload) != 4 or payload[0] != MSG_COMMAND:
            print(f"<- unknown message: {payload.hex()}")
            return
        _, cmd, value = struct.unpack("<BBh", payload)
        print(f"<- {CMD_NAMES.get(cmd, cmd)} value={value}")
        if cmd == CMD_BREW_START:
            if time.time() < self.brew_until:
                self.brew_until = 0.0  # cancel, like the real controller
            else:
                self.brew_start = time.time()
                self.brew_until = self.brew_start + 7 + self.brew_duration
        elif cmd == CMD_STEAM_TOGGLE:
            self.steam = not self.steam
        elif cmd == CMD_SET_SETPOINT:
            self.setpoint = value / 10
        elif cmd == CMD_SET_BREW_DURATION:
            self.brew_duration = value


def main():
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("port")
    ap.add_argument("--baud", type=int, default=115200)
    ap.add_argument("--rate", type=float, default=2.0, help="state messages per second")
    args = ap.parse_args()

    m, parser = FakeMachine(), Parser()
    with serial.Serial(args.port, args.baud, timeout=0.05) as ser:
        last = time.time()
        next_tx = 0.0
        while True:
            for payload in parser.feed(ser.read(256)):
                m.handle(payload)
            now = time.time()
            m.step(now - last)
            last = now
            if now >= next_tx:
                ser.write(frame(m.state_payload()))
                next_tx = now + 1.0 / args.rate
                print(f"-> temp={m.temp:.1f} target={m.target():.1f} steam={m.steam}", end="\r")


if __name__ == "__main__":
    try:
        main()
    except KeyboardInterrupt:
        pass
