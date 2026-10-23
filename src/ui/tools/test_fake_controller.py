"""Host-only protocol smoke tests; no serial port or third-party modules needed."""
import struct
import unittest
from unittest.mock import patch

from fake_controller import FakeMachine, Parser, frame


class FakeControllerTests(unittest.TestCase):
    def setUp(self):
        self.now = 100.0
        self.clock = patch("fake_controller.time.time", side_effect=lambda: self.now)
        self.clock.start()
        self.addCleanup(self.clock.stop)
        self.machine = FakeMachine()

    def command(self, cmd, value=0):
        self.machine.handle(struct.pack("<BBh", 2, cmd, value))

    def start(self):
        self.machine.handle(struct.pack("<BI", 4, 12))
        self.command(1)
        self.machine.state_payload()

    def test_main_time_and_frozen_revision(self):
        self.start()
        self.now = 107.0
        self.machine.state_payload()
        self.machine.handle(struct.pack("<BI", 4, 99))
        self.now = 135.4
        self.command(5)
        result = struct.unpack("<BQIBIIBBHhh", self.machine.journal_payloads()[1])
        self.assertEqual(result[3:6], (2, 12, 28400))
        self.assertEqual(result[6:9], (2, 5, 25))
        self.assertEqual(self.machine.last_brew_ds, 284)

    def test_abort_is_not_a_shot(self):
        self.start()
        self.now = 104.0
        self.command(5)
        self.assertEqual(self.machine.phase, 3)
        self.assertEqual(self.machine.last_brew_ds, 0)

    def test_hold_included(self):
        self.start()
        self.now = 107.0
        self.machine.state_payload()
        self.command(6)
        self.now = 140.0
        self.machine.state_payload()
        self.assertEqual(self.machine.phase, 1)
        self.command(5)
        self.assertEqual(self.machine.duration_ms, 33000)

    def test_new_packets_fit_serial_frames(self):
        self.start()
        messages = [self.machine.state_payload(), *self.machine.journal_payloads()]
        self.assertEqual([len(p) for p in messages], [19, 17, 30])
        encoded = b"noise" + b"".join(frame(p) for p in messages)
        parser = Parser()
        actual = []
        for byte in encoded:
            actual.extend(parser.feed(bytes([byte])))
        self.assertEqual(actual, messages)

    def test_reordered_setup_cannot_revert_selection(self):
        self.machine.handle(struct.pack("<BI", 4, 12))
        self.machine.handle(struct.pack("<BI", 4, 2))
        self.assertEqual(self.machine.revision, 12)


if __name__ == "__main__":
    unittest.main()
