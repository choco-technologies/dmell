# Binary file transfer over TTY

`recvf <new-file>` receives a file and `sendf <file>` sends one. Both are
foreground built-ins, using the current shell's stdin and stdout on the same
full-duplex TTY. They temporarily disable echo and canonical input and restore
the original flags on success or failure. Decoded file bytes are written unchanged,
including NUL, newlines, escape sequences and Ctrl-C.

## Transfer from a PC

Install `pyserial` (`python3 -m pip install pyserial`) and close other serial
monitors while transferring. Match the board's baud rate.

Upload a module to the board:

```sh
python3 tools/tty_file.py --port /dev/ttyACM0 --baud 921600 \
    send example.dmf --remote /example.dmf --command-echo
```

Download it again:

```sh
python3 tools/tty_file.py --port /dev/ttyACM0 --baud 921600 \
    recv downloaded.dmf --remote /example.dmf --command-echo
```

`--remote` issues the corresponding shell command. The tool defaults to a
50 ms delay between command characters and 1 ms between protocol bytes for
polling UART drivers. Use `--byte-delay 0 --command-delay 0` for a UART
that reliably buffers incoming bursts, or increase delays if needed.
`--command-echo` waits for each echoed command character and repeats input
characters that were lost before Enter is sent. Use it with an echoing UART
shell; omit it when shell echo is disabled. Echo pacing requires ASCII paths. Alternatively, run
`recvf /example.dmf` or `sendf /example.dmf` manually in dmell, release the port
from your terminal program, and run the PC tool without `--remote`.

Two dmell instances can exchange files directly: connect their full-duplex
TTY endpoints, run `recvf /destination` on one and `sendf /source` on the other.
Start `sendf` first so that it waits for the handshake, then start `recvf`.
The receiver's ready message must reach the sender after its command has begun;
data arriving while the shell is still editing a command can be consumed by
the editor. With the PC tool you can start `recvf` manually first, since the
serial receive buffer retains its ready message.

## File handling and errors

The receiver refuses an existing destination or `<destination>.recvf.part`.
It writes to that temporary file, checks every block, closes it and renames it
to the destination before reporting completion to the sender. Detected transfer
errors remove the temporary file. This avoids presenting an incomplete module
under its intended name. Delete an old destination explicitly before replacing
it. A reset/power loss may leave a `.recvf.part` file for manual cleanup.

Truncated frames and CRC failures request retransmission of the same block.
A packet is attempted at most ten times; rejected packets are never written to
the file. Storage errors, cancellation and an exhausted retry limit stop the
transfer. Start a fresh transfer after resolving such an error. Ctrl-C (0x03) or CAN (0x18) cancels while waiting for the initial magic;
CAN also rejects an acknowledgement. These bytes are ordinary file data inside
a block, so cancellation is not detected in the middle of a binary payload.

A 30-second deadline applies to each shell read phase, provided the backing
TTY driver returns from reads periodically. A blocking driver can keep a read
waiting longer; a disconnected peer mid-block may require resetting that
session. The PC tool has `--timeout` (default 30 seconds). Use an exclusive,
quiet TTY for transfer: concurrent shell readers, background output, or log
messages on the same output stream can corrupt the protocol. Raw terminal
"send file" functionality does not implement the handshake/CRC protocol; use
the supplied tool or another DMELLF1 peer.

## DMELLF1 wire format

All integers are unsigned little endian. CRC is IEEE CRC-32, identical to
`zlib.crc32`. There is no padding or newline conversion.

Binary packets use byte stuffing: payload/CRC bytes 0x0a and 0x7d are encoded
as 0x7d followed by the original byte XOR 0x20. Each packet ends with two
unescaped LF bytes (0x0a). The receiver ignores empty frames. A single lost LF
is therefore tolerated, and an extra LF left after the final packet becomes
a harmless empty shell command. The file receives only decoded payload bytes.

1. Receiver sends eight bytes `DMELLR1\n`.
2. Sender sends eight bytes `DMELLF1\n`, followed by a framed packet containing
   a 64-bit file length and a 32-bit CRC of those eight length bytes. Lengths
   above `INT64_MAX` are rejected.
3. Receiver validates the header and sends ACK (0x06).
4. Sender sends a framed packet containing up to 128 file bytes and their
   32-bit CRC. The last block contains exactly the remaining bytes. Receiver
   checks the frame length and CRC, writes the payload and sends ACK before the
   next block. No data block is sent for an empty file.
5. Receiver closes and publishes the file, then sends a separate final ACK.
   Sender waits for this before succeeding. There is no EOF marker in the file.

A malformed frame or CRC failure causes NAK (0x15); the sender repeats that
packet without reading another block from the file. Up to ten attempts are
allowed for each header/data packet. A fatal error sends CAN (0x18). Only
negative acknowledgements trigger retransmission: a missing acknowledgement
aborts or waits according to the backing TTY's read behavior, avoiding duplicate
writes after an accepted block. Loss of both delimiters can also require
resetting a blocking session.

Magic scanning tolerates command echo, prompts and CRLF remaining from command
entry. Final ACK confirms the receiver has closed and renamed the file. The
filesystem's close/rename guarantees still apply; the protocol does not provide
power-loss durability or authentication.

## Tests

Build normal dmell modules for the host's architecture, then run:

```sh
DMELL_TEST_DMF=/path/to/native-build/dmf \
    python3 tests/tty_transfer/test_transfer.py
```

These tests run the actual `dmell.dmf` and `dmell_core.dmf` through
`dmod_loader` on pseudo terminals. They check binary round trips, block
boundaries, empty files, lost bytes, corrupt headers/blocks, retransmission, cancellation, existing files,
TTY flag restoration and failure to enter raw mode.
