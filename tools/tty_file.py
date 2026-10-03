#!/usr/bin/env python3
"""Binary DMELLF1 peer. Requires pyserial only for the serial CLI."""
import argparse
import os
from pathlib import Path
import struct
import sys
import time
import zlib

READY = b"DMELLR1\n"
START = b"DMELLF1\n"
ACK = b"\x06"
NAK = b"\x15"
FLAG = 0x0a
ESCAPE = 0x7d
RETRIES = 10
CANCEL = b"\x18"
BLOCK_SIZE = 128


def read_exact(port, size, timeout=30):
    deadline = time.monotonic() + timeout
    data = bytearray()
    while len(data) < size:
        if time.monotonic() >= deadline:
            raise TimeoutError("TTY transfer timed out")
        chunk = port.read(size - len(data))
        if chunk:
            data.extend(chunk)
    return bytes(data)


def write_all(port, data):
    while data:
        count = port.write(data)
        if not count or count > len(data):
            raise OSError("TTY write failed")
        data = data[count:]


def wait_magic(port, magic, timeout=30):
    deadline = time.monotonic() + timeout
    matched = 0
    while matched < len(magic):
        remaining = deadline - time.monotonic()
        if remaining <= 0:
            raise TimeoutError("TTY handshake timed out")
        byte = read_exact(port, 1, remaining)
        if byte in (CANCEL, b"\x03"):
            raise OSError("Peer cancelled the transfer")
        matched = matched + 1 if byte[0] == magic[matched] else int(byte == magic[:1])


def wait_ack(port, timeout):
    if read_exact(port, 1, timeout) != ACK:
        raise OSError("Peer rejected the transfer")


def encode_frame(data):
    wire = bytearray()
    for byte in data:
        if byte in (FLAG, ESCAPE):
            wire.append(ESCAPE)
            byte ^= 0x20
        wire.append(byte)
    wire.extend((FLAG, FLAG))
    return wire


def write_frame(port, data):
    write_all(port, encode_frame(data))


def read_frame(port, size, timeout=30):
    deadline = time.monotonic() + timeout
    data = bytearray()
    escaped = invalid = False
    while True:
        remaining = deadline - time.monotonic()
        if remaining <= 0:
            raise TimeoutError("TTY packet timed out")
        byte = read_exact(port, 1, remaining)[0]
        if byte == FLAG:
            if not data and not invalid and not escaped:
                continue
            if len(data) != size or invalid or escaped:
                raise ValueError("Invalid packet length/escaping")
            return bytes(data)
        if escaped:
            if byte not in (FLAG ^ 0x20, ESCAPE ^ 0x20):
                invalid = True
            byte ^= 0x20
            escaped = False
        elif byte == ESCAPE:
            escaped = True
            continue
        if len(data) < size:
            data.append(byte)
        else:
            invalid = True


def send_packet(port, data, timeout):
    for _ in range(RETRIES):
        write_frame(port, data)
        reply = read_exact(port, 1, timeout)
        if reply == ACK:
            return
        if reply != NAK:
            raise OSError("Peer rejected the transfer")
    raise OSError("Packet retry limit exceeded")


def receive_packet(port, size, timeout):
    for attempt in range(RETRIES):
        try:
            packet = read_frame(port, size + 4, timeout)
            data, crc = packet[:-4], struct.unpack("<I", packet[-4:])[0]
            if zlib.crc32(data) != crc:
                raise ValueError("Invalid packet CRC")
            return data
        except ValueError:
            if attempt + 1 == RETRIES:
                raise OSError("Packet retry limit exceeded")
            write_all(port, NAK)


def send_file(port, path, timeout=30):
    with open(path, "rb") as source:
        size = os.fstat(source.fileno()).st_size
        wait_magic(port, READY, timeout)
        header = struct.pack("<Q", size)
        write_all(port, START)
        send_packet(port, header + struct.pack("<I", zlib.crc32(header)), timeout)
        remaining = size
        while remaining:
            data = source.read(min(BLOCK_SIZE, remaining))
            if not data:
                raise OSError("Source file changed during transfer")
            send_packet(port, data + struct.pack("<I", zlib.crc32(data)), timeout)
            remaining -= len(data)
        wait_ack(port, timeout)
    return size


def receive_file(port, path, timeout=30):
    path = Path(path)
    temporary = path.with_name(path.name + ".recvf.part")
    if path.exists():
        raise FileExistsError(path)
    with temporary.open("xb") as target:
        try:
            write_all(port, READY)
            wait_magic(port, START, timeout)
            size = struct.unpack("<Q", receive_packet(port, 8, timeout))[0]
            if size > (1 << 63) - 1:
                raise OSError("File too large")
            write_all(port, ACK)
            remaining = size
            while remaining:
                count = min(BLOCK_SIZE, remaining)
                data = receive_packet(port, count, timeout)
                target.write(data)
                write_all(port, ACK)
                remaining -= count
            target.flush()
        except BaseException:
            target.close()
            temporary.unlink()
            raise
    try:
        os.link(temporary, path)
        temporary.unlink()
        write_all(port, ACK)
    except BaseException:
        temporary.unlink(missing_ok=True)
        raise
    return size


class PacedPort:
    """Allow polling UART drivers to consume each incoming byte."""
    def __init__(self, port, byte_delay):
        self.port = port
        self.byte_delay = byte_delay

    def read(self, size):
        return self.port.read(size)

    def write(self, data):
        if self.byte_delay == 0:
            return self.port.write(data)
        for byte in data:
            write_all(self.port, bytes([byte]))
            time.sleep(self.byte_delay)
        return len(data)


def write_command(port, command, command_delay=0.05, command_echo=False):
    if command_echo:
        if any(byte > 126 for byte in command):
            raise ValueError("Echo pacing requires an ASCII remote path")
        while port.read(4096):
            pass
    for byte in command:
        attempts = RETRIES if command_echo and 32 <= byte <= 126 else 1
        for _ in range(attempts):
            write_all(port, bytes([byte]))
            if attempts == 1:
                break
            deadline = time.monotonic() + 0.4
            echoed = False
            while time.monotonic() < deadline:
                if port.read(1) == bytes([byte]):
                    echoed = True
                    break
            if echoed:
                break
        else:
            raise TimeoutError("Shell command echo timed out")
        if command_delay:
            time.sleep(command_delay)


def start_remote(port, mode, path, command_delay=0.05, command_echo=False):
    write_command(port, remote_command(mode, path), command_delay, command_echo)


def remote_command(mode, path):
    # dmell's parser has no shell escape syntax; reject ambiguous paths.
    if not path or any(ord(c) < 32 or c in '\"\'$`\\' for c in path):
        raise ValueError("Remote path contains unsupported command characters")
    command = "recvf" if mode == "send" else "sendf"
    return f'{command} "{path}"\r'.encode("utf-8")


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--port", required=True)
    parser.add_argument("--baud", type=int, default=115200)
    parser.add_argument("--timeout", type=float, default=30)
    parser.add_argument("--byte-delay", type=float, default=0.001,
                        help="Seconds between protocol bytes (0 for buffered UART)")
    parser.add_argument("--command-delay", type=float, default=0.05,
                        help="Seconds between shell command characters")
    parser.add_argument("--command-echo", action="store_true",
                        help="Wait for each echoed command character; retry lost input")
    parser.add_argument("mode", choices=("send", "recv"))
    parser.add_argument("file", type=Path, help="Local file")
    parser.add_argument("--remote", help="Run recvf/sendf for this remote path first")
    args = parser.parse_args()
    if args.timeout <= 0 or args.byte_delay < 0 or args.command_delay < 0:
        parser.error("Timeout must be positive and delays nonnegative")
    try:
        import serial
        with serial.Serial(args.port, args.baud, timeout=0.1,
                           write_timeout=args.timeout, exclusive=True) as port:
            if args.remote:
                # Validate local paths before putting the shell into transfer mode.
                if args.mode == "send" and not args.file.is_file():
                    raise FileNotFoundError(args.file)
                if args.mode == "recv" and (args.file.exists() or
                        args.file.with_name(args.file.name + ".recvf.part").exists()):
                    raise FileExistsError(args.file)
                start_remote(port, args.mode, args.remote, args.command_delay, args.command_echo)
            transfer_port = PacedPort(port, args.byte_delay)
            try:
                operation = send_file if args.mode == "send" else receive_file
                size = operation(transfer_port, args.file, args.timeout)
            except BaseException:
                write_all(transfer_port, CANCEL)
                raise
        print(f"Transferred {size} bytes")
    except (OSError, TimeoutError, ValueError, ImportError) as error:
        print(f"tty_file: {error}", file=sys.stderr)
        return 1
    except KeyboardInterrupt:
        return 130
    return 0


if __name__ == "__main__":
    sys.exit(main())
