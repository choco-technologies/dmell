#include <dmod.h>
#include <dmosi.h>
#include <errno.h>
#include <string.h>

/*
 * ymodem - receive files over the console (YMODEM batch).
 *
 *   ymodem recv [-d DIR]
 *
 * The console's terminal is put in raw mode for the transfer: what the
 * sender sends (sb of lrzsz, minicom, Tera Term, dmod-boot's
 * scripts/ymodem_send.py) is the files, one after another, each with its
 * name and size - a name may have directories (made as needed), under
 * DIR (default: the current directory). Blocks of 128 and 1024 bytes,
 * CRC-16.
 *
 * Reads wait for data - they have no timeout (a console's read returns once
 * something came), so the time is kept by a thread: until the sender
 * starts, it sends 'C' every second (a sender started after the command
 * does not see one sent before it), and it answers what the receiver gave
 * up on with NAK - a packet that is not one (its start, its number and
 * complement, its CRC) once the line has been quiet for SKIP_MS (what was
 * left of it read), a packet that stopped coming after PACKET_MS. What
 * comes after that NAK is the sender's next packet, from its start: an EOT
 * or CAN is only taken there, never a byte of data. Each packet is answered
 * once - by the receiver (ACK, NAK) or by the thread (NAK).
 *
 * A sender gone without cancelling leaves the receiver waiting: CAN (^X)
 * twice stops it.
 */

#define SOH         0x01u
#define STX         0x02u
#define EOT         0x04u
#define ACK         0x06u
#define NAK         0x15u
#define CAN         0x18u
#define CRC         'C'

#define MAX_ERRORS  16u
#define BEACON_MS   1000u
#define TICK_MS     10u
#define SKIP_MS     100u            /* The line quiet that long: what was sent is read (a packet is ~11 ms) */
#define PACKET_MS   1000u           /* A packet's bytes stopped that long: some were lost */
#define MAX_PATH    256u
#define OPEN_MS     2000u           /* A file that does not open is tried again that long (one being read) */
#define OPEN_TRY_MS 100u
#define MAX_PACKET  1024u
#define CHUNK       64u

#define PACKET_OK   0
#define PACKET_EOT  1
#define PACKET_CAN  2
#define PACKET_BAD  3               /* Answered (NAK) already */

#define WAIT_START  0               /* For a packet's start (in step with the sender): no NAK for the line quiet */
#define WAIT_PACKET 1               /* For the rest of a packet */
#define WAIT_SKIP   2               /* Skipping what is left of a bad one */

typedef struct
{
    volatile bool       started;    /* The sender sent something: no more 'C' */
    volatile bool       done;
    dmosi_mutex_t       lock;       /* The state below and the thread's NAK: one answer a packet */
    uint32_t            reads;      /* Reads that gave something - the line not quiet */
    uint8_t             waiting;    /* WAIT_* */
    bool                answered;   /* The thread's NAK sent for this packet */
    uint8_t             pending[MAX_PACKET];    /* Read after that NAK: the next packet's start */
    size_t              pending_len, pending_pos;
} link_t;

static void put(uint8_t b)
{
    (void)Dmod_FileWrite(&b, 1, 1, DMOD_STDOUT);
}

/* Waiting for something else: false - the thread answered (NAK) the packet already */
static bool wait_for(link_t* link, uint8_t waiting)
{
    dmosi_mutex_lock(link->lock);
    bool answered = link->answered;
    link->waiting = waiting;
    if (waiting == WAIT_START)
        link->answered = false;
    link->reads++;                          /* (the line's quiet counted from now) */
    dmosi_mutex_unlock(link->lock);
    return !answered;
}

/*
 * Up to n bytes (at least 1), waiting for them: what was put aside first.
 * 0 - the thread answered (NAK) the packet meanwhile: what came is the
 * sender's next packet, put aside.
 */
static size_t read_some(link_t* link, uint8_t* out, size_t n)
{
    if (link->pending_pos < link->pending_len)
    {
        size_t k = link->pending_len - link->pending_pos;
        k = (k < n) ? k : n;
        memcpy(out, link->pending + link->pending_pos, k);
        link->pending_pos += k;
        return k;
    }
    for (;;)
    {
        size_t k = Dmod_FileRead(out, 1, n, DMOD_STDIN);
        if (k == 0 || k > n)
        {
            Dmod_ThreadSleep(1);
            continue;
        }
        link->started = true;
        dmosi_mutex_lock(link->lock);
        link->reads++;
        bool answered = link->answered && link->waiting != WAIT_START;
        dmosi_mutex_unlock(link->lock);
        if (!answered)
            return k;
        memcpy(link->pending, out, k);
        link->pending_len = k;
        link->pending_pos = 0;
        return 0;
    }
}

/* One byte, -1: answered meanwhile */
static int get(link_t* link)
{
    uint8_t b;
    return (read_some(link, &b, 1) == 1) ? b : -1;
}

/* false: answered meanwhile */
static bool get_all(link_t* link, uint8_t* out, size_t n)
{
    for (size_t got = 0; got < n; )
    {
        size_t k = read_some(link, out + got, n - got);
        if (k == 0)
            return false;
        got += k;
    }
    return true;
}

/* CRC-16/XMODEM (CCITT, 0x1021, from 0) */
static uint16_t crc16(const uint8_t* data, size_t n)
{
    uint16_t crc = 0;
    for (size_t i = 0; i < n; i++)
    {
        crc ^= (uint16_t)data[i] << 8;
        for (int k = 0; k < 8; k++)
            crc = (crc & 0x8000u) ? (uint16_t)((crc << 1) ^ 0x1021u) : (uint16_t)(crc << 1);
    }
    return crc;
}

/* The time: 'C' every second until the sender starts, the NAK of what the receiver gave up on */
static void timer_thread(void* arg)
{
    link_t* link = arg;
    uint32_t beacon = BEACON_MS, quiet = 0, reads = 0;
    while (!link->done)
    {
        if (!link->started && (beacon += TICK_MS) >= BEACON_MS)
        {
            put(CRC);
            beacon = 0;
        }
        dmosi_mutex_lock(link->lock);
        if (link->reads != reads || link->waiting == WAIT_START || link->answered)
        {
            reads = link->reads;
            quiet = 0;
        }
        else if ((quiet += TICK_MS) >= ((link->waiting == WAIT_SKIP) ? SKIP_MS : PACKET_MS))
        {
            link->answered = true;
            put(NAK);
        }
        dmosi_mutex_unlock(link->lock);
        Dmod_ThreadSleep(TICK_MS);
    }
}

/* What is left of a bad packet, skipped - until the thread's NAK */
static int skip(link_t* link)
{
    if (!wait_for(link, WAIT_SKIP))
        return PACKET_BAD;
    uint8_t buf[CHUNK];
    while (read_some(link, buf, sizeof(buf)) != 0)
        ;
    return PACKET_BAD;
}

/* A packet: its block number and data (128 / 1024 bytes). Not one - PACKET_BAD (answered) */
static int receive_packet(link_t* link, uint8_t* data, size_t* size, uint8_t* block)
{
    (void)wait_for(link, WAIT_START);
    int c = get(link);
    if (c == EOT)
        return PACKET_EOT;
    if (!wait_for(link, WAIT_PACKET))
        return PACKET_BAD;
    if (c == CAN)
    {
        c = get(link);
        return (c < 0) ? PACKET_BAD : (c == CAN) ? PACKET_CAN : skip(link);
    }
    if (c != SOH && c != STX)
        return skip(link);
    *size = (c == SOH) ? 128u : 1024u;
    uint8_t head[2], tail[2];
    if (!get_all(link, head, 2))
        return PACKET_BAD;
    if ((uint8_t)(head[0] + head[1]) != 0xFFu)
        return skip(link);
    if (!get_all(link, data, *size) || !get_all(link, tail, 2))
        return PACKET_BAD;
    if (crc16(data, *size) != (uint16_t)((tail[0] << 8) | tail[1]))
        return skip(link);
    if (!wait_for(link, WAIT_START))
        return PACKET_BAD;                  /* (answered just now: the sender sends it again) */
    *block = head[0];
    return PACKET_OK;
}

/* The parent directories of a path, made */
static void make_parents(char* path)
{
    for (char* p = path + 1; *p != '\0'; p++)
    {
        if (*p != '/')
            continue;
        *p = '\0';
        (void)Dmod_MakeDir(path, 0755);
        *p = '/';
    }
}

/* A sent name under the directory: no '..' (nothing outside it), '\' as '/' */
static bool target_path(const char* dir, const char* name, char* out, size_t size)
{
    if (name[0] == '\0')
        return false;
    for (const char* p = name; *p != '\0'; p++)
    {
        if (p[0] == '.' && p[1] == '.' && (p == name || p[-1] == '/' || p[-1] == '\\') && (p[2] == '\0' || p[2] == '/' || p[2] == '\\'))
            return false;
    }
    while (*name == '/' || *name == '\\')
        name++;
    int n = Dmod_SnPrintf(out, size, "%s%s%s", dir, (dir[0] != '\0' && dir[strlen(dir) - 1U] != '/') ? "/" : "", name);
    if (n < 0 || (size_t)n >= size)
        return false;
    for (char* p = out; *p != '\0'; p++)
        if (*p == '\\')
            *p = '/';
    return true;
}

static void cancel(void)
{
    for (int i = 0; i < 4; i++)
        put(CAN);
}

typedef struct
{
    uint32_t    files;
    uint32_t    bytes;
    char        failed[MAX_PATH];   /* What could not be written, "" */
    uint32_t    skipped;            /* Files received but not written: they did not open (in use) */
    char        skipped_names[MAX_PATH];
} summary_t;

/* A file that does not open is received all the same - the batch goes on - and named at the end */
static void skipped(summary_t* sum, const char* name)
{
    size_t n = strlen(sum->skipped_names);
    if (n + strlen(name) + 3U < sizeof(sum->skipped_names))
        Dmod_SnPrintf(sum->skipped_names + n, sizeof(sum->skipped_names) - n, "%s%s", (n > 0) ? ", " : "", name);
    else if (n >= 3U && n + 6U < sizeof(sum->skipped_names) && strcmp(sum->skipped_names + n - 3U, "...") != 0)
        Dmod_SnPrintf(sum->skipped_names + n, sizeof(sum->skipped_names) - n, ", ...");
    sum->skipped++;
}

/* The files of a batch, until the empty name */
static int receive(link_t* link, const char* dir, uint8_t* data, summary_t* sum)
{
    uint32_t errors = 0;
    for (;;)
    {
        /* The header: name \0 size ... */
        size_t size;
        uint8_t block;
        int r = receive_packet(link, data, &size, &block);
        if (r == PACKET_CAN)
            return -ECANCELED;
        if (r == PACKET_EOT)
        {
            put(ACK);                       /* (an EOT repeated: our ACK lost) */
            continue;
        }
        if (r != PACKET_OK || block != 0)
        {
            if (++errors > MAX_ERRORS)
            {
                cancel();
                return -EIO;
            }
            if (r != PACKET_BAD)
                put(NAK);                   /* (a bad one: answered already) */
            continue;
        }
        errors = 0;
        data[size - 1U] = '\0';
        const char* name = (const char*)data;
        if (name[0] == '\0')
        {
            put(ACK);                       /* The end of the batch */
            return 0;
        }
        uint32_t length = 0;
        for (const char* p = name + strlen(name) + 1; *p >= '0' && *p <= '9'; p++)
            length = length * 10u + (uint32_t)(*p - '0');

        char path[MAX_PATH];
        void* file = NULL;
        if (target_path(dir, name, path, sizeof(path)))
        {
            /* A file another program reads just then (a view being shown)
             * does not open for writing until it is closed: tried again for
             * a while - the sender waits for the header's answer longer */
            make_parents(path);
            file = Dmod_FileOpen(path, "wb");
            for (uint32_t waited = 0; file == NULL && waited < OPEN_MS; waited += OPEN_TRY_MS)
            {
                dmosi_thread_sleep(OPEN_TRY_MS);
                file = Dmod_FileOpen(path, "wb");
            }
        }
        if (file == NULL)
            skipped(sum, name);             /* (played, shown: received and left out) */
        put(ACK);
        put(CRC);

        /* Its blocks, 1 ... (255 wraps to 0), up to its size */
        uint8_t expected = 1;
        uint32_t written = 0;
        bool eot_once = false;
        for (;;)
        {
            r = receive_packet(link, data, &size, &block);
            if (r == PACKET_CAN)
            {
                if (file != NULL)
                    Dmod_FileClose(file);
                return -ECANCELED;
            }
            if (r == PACKET_EOT)
            {
                if (!eot_once)
                {
                    eot_once = true;
                    put(NAK);               /* The first EOT: is it one? */
                    continue;
                }
                put(ACK);
                break;
            }
            if (r == PACKET_BAD || (block != expected && block != (uint8_t)(expected - 1U)))
            {
                if (++errors > MAX_ERRORS)
                {
                    if (file != NULL)
                        Dmod_FileClose(file);
                    cancel();
                    return -EIO;
                }
                if (r != PACKET_BAD)
                    put(NAK);
                continue;
            }
            errors = 0;
            if (block == expected)
            {
                uint32_t n = (uint32_t)size;
                if (length > 0 && written + n > length)
                    n = length - written;        /* Its padding (0x1A) left out */
                if (n > 0 && file != NULL && Dmod_FileWrite(data, 1, n, file) != n)
                {
                    Dmod_FileClose(file);
                    Dmod_SnPrintf(sum->failed, sizeof(sum->failed), "%s", path);
                    cancel();
                    return -EIO;
                }
                written += n;
                expected++;
            }
            put(ACK);                       /* (a block repeated: our ACK lost - acknowledged again) */
        }
        if (file != NULL)
        {
            Dmod_FileClose(file);
            sum->files++;
            sum->bytes += written;
        }
        put(CRC);                           /* The next file's header */
    }
}

static void usage(void)
{
    Dmod_Printf("Usage: ymodem recv [-d DIR]\n"
                "  Receives files (YMODEM batch) over this console: sb of lrzsz, minicom, Tera Term,\n"
                "  dmod-boot's scripts/ymodem_send.py. Their names (with directories) are under DIR.\n");
}

int main(int argc, char** argv)
{
    const char* dir = "";
    if (argc < 2 || strcmp(argv[1], "recv") != 0)
    {
        usage();
        return -EINVAL;
    }
    for (int i = 2; i < argc; i++)
    {
        if (strcmp(argv[i], "-d") == 0 && i + 1 < argc)
            dir = argv[++i];
        else
        {
            usage();
            return -EINVAL;
        }
    }
    if (dir[0] != '\0')
        (void)Dmod_MakeDir(dir, 0755);

    uint8_t* data = Dmod_Malloc(1024u + 4u);
    if (data == NULL)
        return -ENOMEM;
    summary_t sum;
    memset(&sum, 0, sizeof(sum));

    /* Raw: no echo, no lines - the bytes as they come */
    uint32_t flags = Dmod_Stdin_GetFlags();
    Dmod_Stdin_SetFlags(0);
    link_t* link = Dmod_Malloc(sizeof(link_t));
    int status = -ENOMEM;
    if (link != NULL)
    {
        memset(link, 0, sizeof(*link));
        link->lock = dmosi_mutex_create(false);
    }
    dmosi_thread_t thread = (link != NULL && link->lock != NULL)
                          ? dmosi_thread_create(timer_thread, link, dmosi_thread_get_priority(NULL),
                                                1024u + DMOSI_THREAD_STACK_OVERHEAD, "ymodem_timer", NULL)
                          : NULL;
    if (thread != NULL)
    {
        status = receive(link, dir, data, &sum);
        link->done = true;
        (void)dmosi_thread_join(thread);
    }
    if (link != NULL)
    {
        if (link->lock != NULL)
            dmosi_mutex_destroy(link->lock);
        Dmod_Free(link);
    }
    Dmod_ThreadSleep(100);                  /* The sender's last bytes, before the terminal is a terminal again */
    Dmod_Stdin_SetFlags(flags);
    Dmod_Free(data);

    if (status == 0 && sum.skipped > 0)
        Dmod_Printf("\nymodem: %u file(s), %u bytes; %u not written (in use?): %s\n", (unsigned)sum.files,
                    (unsigned)sum.bytes, (unsigned)sum.skipped, sum.skipped_names);
    else if (status == 0)
        Dmod_Printf("\nymodem: %u file(s), %u bytes\n", (unsigned)sum.files, (unsigned)sum.bytes);
    else if (status == -ECANCELED)
        Dmod_Printf("\nymodem: cancelled (%u file(s) received)\n", (unsigned)sum.files);
    else
        Dmod_Printf("\nymodem: failed (%d)%s%s\n", status, (sum.failed[0] != '\0') ? " at " : "", sum.failed);
    return status;
}
