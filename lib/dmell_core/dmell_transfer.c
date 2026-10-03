#include <dmod.h>
#include <errno.h>
#include <stdint.h>
#include <string.h>
#include "dmell_transfer.h"

#define TRANSFER_BLOCK_SIZE 128
#define TRANSFER_TIMEOUT_MS 30000
#define TRANSFER_ACK 0x06
#define TRANSFER_NAK 0x15
#define TRANSFER_FLAG 0x0a
#define TRANSFER_ESCAPE 0x7d
#define TRANSFER_RETRIES 10
#define TRANSFER_PACKET_SIZE (TRANSFER_BLOCK_SIZE + 4)
#define TRANSFER_BUFFER_SIZE (3 * TRANSFER_PACKET_SIZE + 2)
#define TRANSFER_CANCEL 0x18
#define TRANSFER_READY "DMELLR1\n"
#define TRANSFER_START "DMELLF1\n"
#define TRANSFER_MAGIC_SIZE 8

/* Wire integers are little endian; CRC is IEEE CRC-32 (as in zlib). */
static uint32_t transfer_crc(const uint8_t *data, size_t size)
{
    uint32_t crc = UINT32_MAX;
    for (size_t i = 0; i < size; i++)
    {
        crc ^= data[i];
        for (unsigned bit = 0; bit < 8; bit++)
            crc = (crc >> 1) ^ (0xedb88320u & (0u - (crc & 1u)));
    }
    return ~crc;
}

static void transfer_encode(uint8_t *data, uint64_t value, size_t size)
{
    for (size_t i = 0; i < size; i++)
    {
        data[i] = (uint8_t)value;
        value >>= 8;
    }
}

static uint64_t transfer_decode(const uint8_t *data, size_t size)
{
    uint64_t value = 0;
    for (size_t i = 0; i < size; i++)
        value |= (uint64_t)data[i] << (8 * i);
    return value;
}

/* The backing TTY must return from reads periodically for the timeout to
 * apply. No text/escape processing is performed once the handshake ends. */
static int transfer_read(uint8_t *data, size_t size)
{
    Dmod_Timestamp_t started = Dmod_GetUptime();
    while (size > 0)
    {
        if (Dmod_GetUptime() - started >= TRANSFER_TIMEOUT_MS)
            return -ETIMEDOUT;
        size_t count = Dmod_FileRead(data, 1, size, DMOD_STDIN);
        if (count > size) return -EIO;
        if (count == 0)
        {
            Dmod_ThreadSleep(1);
            continue;
        }
        data += count;
        size -= count;
    }
    return 0;
}

static int transfer_write(void *file, const uint8_t *data, size_t size)
{
    while (size > 0)
    {
        size_t count = Dmod_FileWrite(data, 1, size, file);
        if (count == 0 || count > size) return -EIO;
        data += count;
        size -= count;
    }
    return 0;
}

static int transfer_reply(uint8_t byte)
{
    return transfer_write(DMOD_STDOUT, &byte, 1);
}

static int transfer_wait_ack(void)
{
    uint8_t byte;
    int result = transfer_read(&byte, 1);
    if (result != 0) return result;
    if (byte == TRANSFER_NAK) return -EAGAIN;
    return byte == TRANSFER_ACK ? 0 : -ECANCELED;
}

/* Ignore command echo/prompts and the LF left by a CRLF command line.
 * Ctrl-C/CAN are cancellation only here, never inside binary payloads. */
static int transfer_wait_magic(const char *magic)
{
    size_t matched = 0;
    Dmod_Timestamp_t started = Dmod_GetUptime();
    while (matched < TRANSFER_MAGIC_SIZE)
    {
        uint8_t byte;
        if (Dmod_GetUptime() - started >= TRANSFER_TIMEOUT_MS)
            return -ETIMEDOUT;
        int result = transfer_read(&byte, 1);
        if (result != 0) return result;
        if (byte == 3 || byte == TRANSFER_CANCEL) return -ECANCELED;
        matched = byte == (uint8_t)magic[matched] ? matched + 1 :
                  (byte == (uint8_t)magic[0] ? 1 : 0);
    }
    return 0;
}

/* Byte stuffing makes FLAG an unambiguous end of a packet even after a
 * byte is lost. Two LF end flags tolerate a single lost delimiter.
 * A leftover LF is a harmless empty command when returning to the shell. */
static int transfer_write_frame(const uint8_t *data, size_t size, uint8_t *wire)
{
    size_t count = 0;
    for (size_t i = 0; i < size; i++)
    {
        uint8_t byte = data[i];
        if (byte == TRANSFER_FLAG || byte == TRANSFER_ESCAPE)
        {
            wire[count++] = TRANSFER_ESCAPE;
            byte ^= 0x20;
        }
        wire[count++] = byte;
    }
    wire[count++] = TRANSFER_FLAG;
    wire[count++] = TRANSFER_FLAG;
    return transfer_write(DMOD_STDOUT, wire, count);
}

static int transfer_read_frame(uint8_t *data, size_t size)
{
    size_t count = 0;
    bool escaped = false, invalid = false;
    Dmod_Timestamp_t started = Dmod_GetUptime();
    for (;;)
    {
        uint8_t byte;
        if (Dmod_GetUptime() - started >= TRANSFER_TIMEOUT_MS) return -ETIMEDOUT;
        int result = transfer_read(&byte, 1);
        if (result != 0) return result;
        if (byte == TRANSFER_FLAG)
        {
            if (count == 0 && !invalid && !escaped) continue;
            return count == size && !invalid && !escaped ? 0 : -EBADMSG;
        }
        if (escaped)
        {
            if (byte != (TRANSFER_FLAG ^ 0x20) && byte != (TRANSFER_ESCAPE ^ 0x20))
                invalid = true;
            byte ^= 0x20;
            escaped = false;
        }
        else if (byte == TRANSFER_ESCAPE)
        {
            escaped = true;
            continue;
        }
        if (count < size) data[count++] = byte;
        else invalid = true;
    }
}

static int transfer_send_packet(const uint8_t *data, size_t size, uint8_t *wire)
{
    for (unsigned attempt = 0; attempt < TRANSFER_RETRIES; attempt++)
    {
        int result = transfer_write_frame(data, size, wire);
        if (result == 0) result = transfer_wait_ack();
        if (result != -EAGAIN) return result;
    }
    return -EBADMSG;
}

static int transfer_receive_packet(uint8_t *data, size_t payload_size)
{
    for (unsigned attempt = 0; attempt < TRANSFER_RETRIES; attempt++)
    {
        int result = transfer_read_frame(data, payload_size + 4);
        if (result == 0 && transfer_decode(data + payload_size, 4) !=
                           transfer_crc(data, payload_size)) result = -EBADMSG;
        if (result != -EBADMSG) return result;
        if (attempt + 1 == TRANSFER_RETRIES) return -EBADMSG;
        result = transfer_reply(TRANSFER_NAK);
        if (result != 0) return result;
    }
    return -EBADMSG;
}

static int transfer_send_header(uint64_t size, uint8_t *wire)
{
    uint8_t header[12];
    transfer_encode(header, size, 8);
    transfer_encode(header + 8, transfer_crc(header, 8), 4);
    int result = transfer_wait_magic(TRANSFER_READY);
    if (result == 0)
        result = transfer_write(DMOD_STDOUT, (const uint8_t *)TRANSFER_START,
                                TRANSFER_MAGIC_SIZE);
    if (result == 0) result = transfer_send_packet(header, sizeof(header), wire);
    return result;
}

static int transfer_receive_header(uint64_t *size)
{
    uint8_t header[12];
    int result = transfer_write(DMOD_STDOUT, (const uint8_t *)TRANSFER_READY,
                                TRANSFER_MAGIC_SIZE);
    if (result == 0) result = transfer_wait_magic(TRANSFER_START);
    if (result == 0) result = transfer_receive_packet(header, 8);
    if (result != 0) return result;
    *size = transfer_decode(header, 8);
    if (*size > INT64_MAX) return -EFBIG;
    return transfer_reply(TRANSFER_ACK);
}

static int transfer_send_block(void *file, uint8_t *buffer, uint8_t *wire, size_t size)
{
    size_t offset = 0;
    while (offset < size)
    {
        size_t count = Dmod_FileRead(buffer + offset, 1, size - offset, file);
        if (count == 0 || count > size - offset) return -EIO;
        offset += count;
    }
    transfer_encode(buffer + size, transfer_crc(buffer, size), 4);
    return transfer_send_packet(buffer, size + 4, wire);
}

static int transfer_receive_block(void *file, uint8_t *buffer, size_t size)
{
    int result = transfer_receive_packet(buffer, size);
    if (result == 0) result = transfer_write(file, buffer, size);
    if (result == 0) result = transfer_reply(TRANSFER_ACK);
    return result;
}

static int transfer_blocks(void *file, uint8_t *buffer, uint8_t *wire, uint64_t size, bool receive)
{
    int result = 0;
    while (size > 0 && result == 0)
    {
        size_t count = size < TRANSFER_BLOCK_SIZE ? (size_t)size : TRANSFER_BLOCK_SIZE;
        result = receive ? transfer_receive_block(file, buffer, count) :
                           transfer_send_block(file, buffer, wire, count);
        size -= count;
    }
    return result;
}

/* These commands stay in the foreground shell: spawning a second reader
 * would lose dmtty foreground ownership. Restore flags on every exit. */
static int transfer_run(void *file, const char *temporary, const char *path)
{
    bool receive = temporary != NULL;
    uint8_t *buffer = Dmod_Malloc(TRANSFER_BUFFER_SIZE);
    if (buffer == NULL)
    {
        if (receive) Dmod_FileClose(file);
        return -ENOMEM;
    }
    uint32_t flags = Dmod_Stdin_GetFlags();
    int result = Dmod_Stdin_SetFlags(flags & ~(DMOD_STDIN_FLAG_ECHO |
                                              DMOD_STDIN_FLAG_CANONICAL));
    if (result != 0)
    {
        if (receive) Dmod_FileClose(file);
        Dmod_Free(buffer);
        return result;
    }
    uint64_t size = 0;
    if (receive)
        result = transfer_receive_header(&size);
    else
    {
        size = Dmod_FileSize(file);
        result = size > INT64_MAX ? -EFBIG : transfer_send_header(size, buffer + TRANSFER_PACKET_SIZE);
    }
    if (result == 0) result = transfer_blocks(file, buffer, buffer + TRANSFER_PACKET_SIZE, size, receive);
    if (receive)
    {
        Dmod_FileClose(file);
        if (result == 0) result = Dmod_Rename(temporary, path);
        if (result == 0) result = transfer_reply(TRANSFER_ACK);
    }
    else if (result == 0)
        result = transfer_wait_ack();
    if (result != 0) transfer_reply(TRANSFER_CANCEL);
    Dmod_Free(buffer);
    int restored = Dmod_Stdin_SetFlags(flags);
    return result != 0 ? result : restored;
}

int dmell_handler_sendf(int argc, char **argv, dmell_ctx_t *ctx)
{
    (void)ctx;
    if (argc != 2 || argv[1][0] == '\0')
    {
        Dmod_Printf("Usage: sendf <file>\n");
        return -EINVAL;
    }
    void *file = Dmod_FileOpen(argv[1], "rb");
    if (file == NULL)
    {
        Dmod_Printf("sendf: cannot open %s\n", argv[1]);
        return -ENOENT;
    }
    int result = transfer_run(file, NULL, argv[1]);
    Dmod_FileClose(file);
    if (result != 0) Dmod_Printf("sendf: transfer failed (%d)\n", result);
    return result;
}

static char *transfer_temporary_path(const char *path)
{
    const char *suffix = ".recvf.part";
    size_t length = strlen(path);
    if (length > SIZE_MAX - strlen(suffix) - 1) return NULL;
    char *temporary = Dmod_Malloc(length + strlen(suffix) + 1);
    if (temporary != NULL)
    {
        memcpy(temporary, path, length);
        memcpy(temporary + length, suffix, strlen(suffix) + 1);
    }
    return temporary;
}

int dmell_handler_recvf(int argc, char **argv, dmell_ctx_t *ctx)
{
    (void)ctx;
    if (argc != 2 || argv[1][0] == '\0')
    {
        Dmod_Printf("Usage: recvf <new-file>\n");
        return -EINVAL;
    }
    char *temporary = transfer_temporary_path(argv[1]);
    if (temporary == NULL) return -ENOMEM;
    int result = -EEXIST;
    if (!Dmod_FileAvailable(argv[1]) && !Dmod_FileAvailable(temporary))
    {
        void *file = Dmod_FileOpen(temporary, "wb");
        result = file == NULL ? -ENOENT : transfer_run(file, temporary, argv[1]);
        if (file != NULL && result != 0) Dmod_FileRemove(temporary);
    }
    Dmod_Free(temporary);
    if (result != 0) Dmod_Printf("recvf: transfer failed (%d)\n", result);
    return result;
}
