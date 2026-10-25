/* hev-win-fd.c - Virtual file descriptors for Winsock sockets */

#define HEV_WIN_RAW
#include "hev-sys.h"
#include "lib/misc/hev-compiler.h"

#define REG_INITIAL_CAP 256
#define REG_MAX_FDS (1 << 20)
#define REG_FD_BASE 0x40000000

static SRWLOCK reg_lock = SRWLOCK_INIT;
static SOCKET *reg_slots;
static size_t reg_capacity;
static volatile LONG reg_state;

static SRWLOCK wsa_lock = SRWLOCK_INIT;
static LONG wsa_refs;

static void
reg_init (void)
{
    LONG state = InterlockedCompareExchange (&reg_state, 1, 0);

    if (state == 0) {
        reg_slots = (SOCKET *)malloc (sizeof (SOCKET) * REG_INITIAL_CAP);
        if (reg_slots) {
            size_t i;

            for (i = 0; i < REG_INITIAL_CAP; i++)
                reg_slots[i] = INVALID_SOCKET;
            reg_capacity = REG_INITIAL_CAP;
        } else {
            reg_capacity = 0;
        }

        MemoryBarrier ();
        InterlockedExchange (&reg_state, 2);
        return;
    }

    while (InterlockedCompareExchange (&reg_state, 2, 2) != 2)
        YieldProcessor ();
}

static int
reg_index_from_fd (int fd)
{
    if (fd < REG_FD_BASE || fd >= REG_FD_BASE + REG_MAX_FDS)
        return -1;

    return fd - REG_FD_BASE;
}

static int
reg_grow_locked (size_t need_idx)
{
    size_t new_cap;
    SOCKET *new_slots;
    size_t i;

    if (need_idx >= REG_MAX_FDS)
        return -1;

    new_cap = reg_capacity ? reg_capacity * 2 : REG_INITIAL_CAP;
    if (new_cap <= need_idx)
        new_cap = need_idx + 1;
    if (new_cap > REG_MAX_FDS)
        new_cap = REG_MAX_FDS;
    if (new_cap <= reg_capacity)
        return -1;

    new_slots = (SOCKET *)realloc (reg_slots, sizeof (SOCKET) * new_cap);
    if (!new_slots)
        return -1;

    for (i = reg_capacity; i < new_cap; i++)
        new_slots[i] = INVALID_SOCKET;

    reg_slots = new_slots;
    reg_capacity = new_cap;

    return 0;
}

EXPORT_SYMBOL SOCKET
hev_win_fd_to_socket (int fd)
{
    SOCKET s = INVALID_SOCKET;
    int idx;

    reg_init ();
    idx = reg_index_from_fd (fd);
    if (idx < 0)
        return INVALID_SOCKET;

    AcquireSRWLockShared (&reg_lock);
    if ((size_t)idx < reg_capacity)
        s = reg_slots[idx];
    ReleaseSRWLockShared (&reg_lock);

    return s;
}

EXPORT_SYMBOL int
hev_win_socket_to_fd (SOCKET s)
{
    size_t i;
    int fd = -1;

    if (s == INVALID_SOCKET)
        return -1;

    reg_init ();

    AcquireSRWLockExclusive (&reg_lock);

    for (i = 0; i < reg_capacity; i++) {
        if (reg_slots[i] == INVALID_SOCKET) {
            reg_slots[i] = s;
            fd = REG_FD_BASE + (int)i;
            break;
        }
    }

    if (fd < 0) {
        size_t old_cap = reg_capacity;

        if (reg_grow_locked (old_cap) == 0) {
            reg_slots[old_cap] = s;
            fd = REG_FD_BASE + (int)old_cap;
        }
    }

    ReleaseSRWLockExclusive (&reg_lock);

    return fd;
}

SOCKET
hev_win_fd_close_socket (int fd)
{
    SOCKET s = INVALID_SOCKET;
    int idx;

    reg_init ();
    idx = reg_index_from_fd (fd);
    if (idx < 0)
        return INVALID_SOCKET;

    AcquireSRWLockExclusive (&reg_lock);
    if ((size_t)idx < reg_capacity) {
        s = reg_slots[idx];
        reg_slots[idx] = INVALID_SOCKET;
    }
    ReleaseSRWLockExclusive (&reg_lock);

    return s;
}

int
hev_win_fd_replace (int fd, SOCKET s, SOCKET *old)
{
    int idx = reg_index_from_fd (fd);

    if (idx < 0) {
        errno = EINVAL;
        return -1;
    }

    reg_init ();
    AcquireSRWLockExclusive (&reg_lock);
    if ((size_t)idx >= reg_capacity && reg_grow_locked ((size_t)idx) < 0) {
        ReleaseSRWLockExclusive (&reg_lock);
        errno = EMFILE;
        return -1;
    }
    *old = reg_slots[idx];
    reg_slots[idx] = s;
    ReleaseSRWLockExclusive (&reg_lock);

    return 0;
}

SOCKET
hev_win_fd_socket (int fd)
{
    SOCKET s = hev_win_fd_to_socket (fd);

    if (s == INVALID_SOCKET)
        WSASetLastError (WSAENOTSOCK);

    return s;
}

int
hev_win_fd_register (SOCKET s)
{
    int fd;
    u_long nonblock = 1;

    if (s == INVALID_SOCKET) {
        hev_win_set_errno_from_wsa ();
        return -1;
    }

    if (ioctlsocket (s, FIONBIO, &nonblock) != 0) {
        hev_win_set_errno_from_wsa ();
        closesocket (s);
        return -1;
    }

    fd = hev_win_socket_to_fd (s);
    if (fd < 0) {
        closesocket (s);
        errno = EMFILE;
        return -1;
    }

    return fd;
}

int
hev_win_wsa_init (void)
{
    int res = 0;

    reg_init ();
    AcquireSRWLockExclusive (&wsa_lock);

    if (wsa_refs == 0) {
        WSADATA data;

        res = WSAStartup (MAKEWORD (2, 2), &data);
        if (res != 0) {
            WSASetLastError (res);
            ReleaseSRWLockExclusive (&wsa_lock);
            return -1;
        }
    }

    wsa_refs++;
    ReleaseSRWLockExclusive (&wsa_lock);

    return 0;
}

void
hev_win_wsa_fini (void)
{
    AcquireSRWLockExclusive (&wsa_lock);

    if (wsa_refs <= 0) {
        ReleaseSRWLockExclusive (&wsa_lock);
        return;
    }

    wsa_refs--;
    if (wsa_refs == 0)
        WSACleanup ();

    ReleaseSRWLockExclusive (&wsa_lock);
}
