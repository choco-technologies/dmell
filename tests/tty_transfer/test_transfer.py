#!/usr/bin/env python3
"""Integration tests against real dmell DMF modules loaded by dmod_loader.

DMELL_TEST_DMF=/path/to/build/dmf python3 tests/tty_transfer/test_transfer.py
"""
import importlib.util
import os
from pathlib import Path
import pty
import select
import struct
import subprocess
import tempfile
import termios
import tty
import time
import unittest
import zlib

ROOT = Path(__file__).resolve().parents[2]
spec = importlib.util.spec_from_file_location("tty_file", ROOT / "tools/tty_file.py")
peer = importlib.util.module_from_spec(spec)
spec.loader.exec_module(peer)
DMF = Path(os.environ.get("DMELL_TEST_DMF", ROOT / "build/dmf")).resolve()


class Shell:
    def __init__(self, command, pipe_input=False, unbuffered=False):
        self.master, self.slave = pty.openpty()
        tty.setraw(self.slave)
        attributes = termios.tcgetattr(self.slave)
        attributes[3] |= termios.ECHO | termios.ICANON
        termios.tcsetattr(self.slave, termios.TCSANOW, attributes)
        self.flags = attributes[3]
        env = dict(os.environ, DMOD_DMF_DIR=str(DMF), DMOD_REPO_DIR=str(DMF))
        arguments = [os.environ.get("DMOD_LOADER", "dmod_loader"),
                     str(DMF / "dmell.dmf")]
        if unbuffered:
            # Native stdio buffers single-character echoes; embedded UART
            # writes them immediately. Match that transport for echo pacing.
            arguments = ["stdbuf", "-o0"] + arguments
        if command is not None:
            arguments += ["--args", "-c", command]
        self.proc = subprocess.Popen(
            arguments,
            stdin=subprocess.DEVNULL if pipe_input else self.slave,
            stdout=self.slave, stderr=self.slave, env=env)

    def read(self, size):
        if not select.select([self.master], [], [], .1)[0]:
            return b""
        return os.read(self.master, size)

    def write(self, data):
        return os.write(self.master, data)

    def finish(self, success=True):
        status = self.proc.wait(timeout=5)
        if success:
            assert status == 0, status
        else:
            assert status != 0, status
        flags = termios.tcgetattr(self.slave)[3]
        assert flags == self.flags, (flags, self.flags)

    def close(self):
        if self.proc.poll() is None:
            self.proc.kill()
            self.proc.wait()
        os.close(self.master)
        os.close(self.slave)


@unittest.skipUnless((DMF / "dmell.dmf").is_file(), "Build native dmell first")
class TransferTests(unittest.TestCase):
    def setUp(self):
        self.temp = tempfile.TemporaryDirectory()
        self.root = Path(self.temp.name)
        self.shells = []

    def tearDown(self):
        for shell in self.shells:
            shell.close()
        self.temp.cleanup()

    def shell(self, command, **kwargs):
        shell = Shell(command, **kwargs)
        self.shells.append(shell)
        return shell

    def header(self, shell, size, crc=None):
        peer.wait_magic(shell, peer.READY, 5)
        data = struct.pack("<Q", size)
        peer.write_all(shell, peer.START)
        peer.write_frame(shell, data + struct.pack(
            "<I", zlib.crc32(data) if crc is None else crc))

    def test_interactive_command_ignores_nul(self):
        source, target = self.root / "nul-source", self.root / "nul-target"
        data = b"\x00\x03\x18\r\n\x7d\xff"
        source.write_bytes(data)
        shell = self.shell(None)
        command = b're\x00cvf "' + str(target).encode() + b'"\x00\r'
        peer.write_all(shell, command)
        peer.send_file(shell, source, 5)
        self.assertEqual(target.read_bytes(), data)
        peer.write_all(shell, b"exit\r")
        shell.finish()

    def test_remote_command_with_echo_pacing(self):
        source, target = self.root / "echo-source", self.root / "echo-target"
        source.write_bytes(bytes(range(256)))
        shell = self.shell(None, unbuffered=True)
        peer.start_remote(shell, "send", str(target), 0, True)
        peer.send_file(shell, source, 5)
        self.assertEqual(target.read_bytes(), source.read_bytes())
        peer.write_all(shell, b"exit\r")
        shell.finish()

    def test_binary_round_trips(self):
        for size in (0, 1, 127, 128, 129, 255, 256, 257, 4097, 65536):
            with self.subTest(size=size):
                data = (bytes(range(256)) * ((size + 255) // 256))[:size]
                source, received, returned = [self.root / f"{size}-{x}.bin"
                                              for x in ("source", "received", "returned")]
                source.write_bytes(data)
                receiver = self.shell(f'recvf "{received}"')
                self.assertEqual(peer.send_file(receiver, source, 5), size)
                receiver.finish()
                self.assertEqual(received.read_bytes(), data)
                sender = self.shell(f'sendf "{received}"')
                self.assertEqual(peer.receive_file(sender, returned, 5), size)
                sender.finish()
                self.assertEqual(returned.read_bytes(), data)

    def test_pc_paced_transport(self):
        source, target = self.root / "paced-source", self.root / "paced-target"
        data = bytes(range(256)) + b"\x00\x03\x18\r\n"
        source.write_bytes(data)
        shell = self.shell(f'recvf "{target}"')
        self.assertEqual(peer.send_file(peer.PacedPort(shell, .0001), source, 5), len(data))
        shell.finish()
        self.assertEqual(target.read_bytes(), data)

    def test_two_dmell_peers(self):
        source, target = self.root / "source", self.root / "target"
        data = bytes(range(256)) * 16 + b"\x00\xff\x03\x18\r\n"
        source.write_bytes(data)
        sender = self.shell(f'sendf "{source}"')
        deadline = time.monotonic() + 5
        while termios.tcgetattr(sender.slave)[3] & termios.ICANON:
            self.assertLess(time.monotonic(), deadline)
            time.sleep(.005)
        receiver = self.shell(f'recvf "{target}"')
        links = {sender.master: receiver, receiver.master: sender}
        while sender.proc.poll() is None or receiver.proc.poll() is None:
            self.assertLess(time.monotonic(), deadline)
            for fd in select.select(list(links), [], [], .1)[0]:
                peer.write_all(links[fd], os.read(fd, 4096))
        sender.finish()
        receiver.finish()
        self.assertEqual(target.read_bytes(), data)

    def test_corrupt_header_and_oversized_header(self):
        for size, crc in ((123, 0), (1 << 63, None)):
            with self.subTest(size=size):
                path = self.root / "bad-header"
                shell = self.shell(f'recvf "{path}"')
                self.header(shell, size, crc)
                if crc == 0:
                    data = struct.pack("<Q", size) + struct.pack("<I", crc)
                    for _ in range(peer.RETRIES - 1):
                        self.assertEqual(peer.read_exact(shell, 1, 5), peer.NAK)
                        peer.write_frame(shell, data)
                self.assertEqual(peer.read_exact(shell, 1, 5), peer.CANCEL)
                shell.finish(False)
                self.assertFalse(path.exists())
                self.assertFalse(Path(str(path) + ".recvf.part").exists())

    def test_corrupt_block_removes_partial_file(self):
        path = self.root / "bad-block"
        shell = self.shell(f'recvf "{path}"')
        self.header(shell, 129)
        peer.wait_ack(shell, 5)
        data = bytes(range(128))
        peer.write_frame(shell, data + struct.pack("<I", zlib.crc32(data)))
        peer.wait_ack(shell, 5)
        for attempt in range(peer.RETRIES):
            peer.write_frame(shell, b"a" + struct.pack("<I", 0))
            if attempt + 1 < peer.RETRIES:
                self.assertEqual(peer.read_exact(shell, 1, 5), peer.NAK)
        self.assertEqual(peer.read_exact(shell, 1, 5), peer.CANCEL)
        shell.finish(False)
        self.assertFalse(path.exists())
        self.assertFalse(Path(str(path) + ".recvf.part").exists())

    def test_lost_byte_and_crc_retry(self):
        path = self.root / "retry"
        shell = self.shell(f'recvf "{path}"')
        data = bytes(range(128))
        packet = data + struct.pack("<I", zlib.crc32(data))
        self.header(shell, len(data))
        peer.wait_ack(shell, 5)
        peer.write_frame(shell, packet[:42] + packet[43:])
        self.assertEqual(peer.read_exact(shell, 1, 5), peer.NAK)
        peer.write_frame(shell, packet[:-1] + bytes([packet[-1] ^ 1]))
        self.assertEqual(peer.read_exact(shell, 1, 5), peer.NAK)
        peer.write_frame(shell, packet)
        peer.wait_ack(shell, 5)
        peer.wait_ack(shell, 5)
        shell.finish()
        self.assertEqual(path.read_bytes(), data)

    def test_single_lost_delimiter(self):
        path = self.root / "delimiter"
        shell = self.shell(f'recvf "{path}"')
        data = bytes(range(128))
        self.header(shell, len(data))
        peer.wait_ack(shell, 5)
        packet = data + struct.pack("<I", zlib.crc32(data))
        peer.write_all(shell, peer.encode_frame(packet)[:-1])
        peer.wait_ack(shell, 5)
        peer.wait_ack(shell, 5)
        shell.finish()
        self.assertEqual(path.read_bytes(), data)

    def test_cancel_handshake_and_crlf(self):
        path = self.root / "cancelled"
        shell = self.shell(f'recvf "{path}"')
        peer.wait_magic(shell, peer.READY, 5)
        peer.write_all(shell, b"\r\n\x03")
        self.assertEqual(peer.read_exact(shell, 1, 5), peer.CANCEL)
        shell.finish(False)
        self.assertFalse(path.exists())
        self.assertFalse(Path(str(path) + ".recvf.part").exists())

    def test_refuse_existing_destination_and_partial(self):
        for suffix in ("", ".recvf.part"):
            path = self.root / ("exists" + str(len(suffix)))
            protected = Path(str(path) + suffix)
            protected.write_bytes(b"keep")
            shell = self.shell(f'recvf "{path}"')
            shell.finish(False)
            self.assertEqual(protected.read_bytes(), b"keep")

    def test_raw_mode_failure_cleans_up(self):
        path = self.root / "not-a-tty"
        shell = self.shell(f'recvf "{path}"', pipe_input=True)
        shell.finish(False)
        self.assertFalse(path.exists())
        self.assertFalse(Path(str(path) + ".recvf.part").exists())

    def test_sender_repeats_rejected_packet(self):
        path = self.root / "source"
        data = bytes(range(128))
        path.write_bytes(data)
        shell = self.shell(f'sendf "{path}"')
        peer.write_all(shell, peer.READY)
        peer.wait_magic(shell, peer.START, 5)
        peer.read_frame(shell, 12, 5)
        peer.write_all(shell, peer.ACK)
        packet = peer.read_frame(shell, len(data) + 4, 5)
        peer.write_all(shell, peer.NAK)
        repeated = peer.read_frame(shell, len(data) + 4, 5)
        self.assertEqual(packet, repeated)
        self.assertEqual(packet[:-4], data)
        peer.write_all(shell, peer.ACK + peer.ACK)
        shell.finish()

    def test_sender_peer_rejection(self):
        path = self.root / "source"
        path.write_bytes(b"abc")
        shell = self.shell(f'sendf "{path}"')
        peer.write_all(shell, peer.READY)
        peer.wait_magic(shell, peer.START, 5)
        peer.read_frame(shell, 12, 5)
        peer.write_all(shell, peer.CANCEL)
        reply = peer.read_exact(shell, 1, 5)
        while reply == bytes([peer.FLAG]):
            reply = peer.read_exact(shell, 1, 5)
        self.assertEqual(reply, peer.CANCEL)
        shell.finish(False)
        self.assertEqual(path.read_bytes(), b"abc")


if __name__ == "__main__":
    if not (DMF / "dmell.dmf").is_file():
        raise SystemExit("Set DMELL_TEST_DMF to a native dmell build/dmf directory")
    unittest.main(verbosity=2)
