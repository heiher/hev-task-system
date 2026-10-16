/*
 * hev-win-compat.c - POSIX/MSVC compatibility implementations for Windows
 *
 * Implements the socket descriptor registry, POSIX socket API emulation
 * on top of WinSock, and small CRT shims.  Compiled only on Windows.
 */

#include "hev-win-compat.h"

/*
 * Undefine the POSIX name mappings so this file can call the real
 * WinSock and CRT functions directly.
 */
#undef socket
#undef accept
#undef socketpair
#undef connect
#undef bind
#undef listen
#undef shutdown
#undef getsockname
#undef getpeername
#undef getsockopt
#undef setsockopt
#undef read
#undef write
#undef readv
#undef writev
#undef recv
#undef send
#undef recvfrom
#undef sendto
#undef recvmsg
#undef sendmsg
#undef recvmmsg
#undef sendmmsg
#undef ioctl
#undef close
#undef dup
#undef dup2
#undef pipe
#undef getaddrinfo
#undef getnameinfo
#undef freeaddrinfo
#undef clock_gettime
#undef usleep
#undef fopen

#include <assert.h>

/* ------------------------------------------------------------------ */
/* Descriptor registry                                                 */
/* ------------------------------------------------------------------ */

/*
 * Keep logical socket descriptors in a namespace that cannot collide with
 * normal MSVC CRT file descriptors.  Project code still sees an int, while
 * the real pointer-sized SOCKET stays exclusively in this registry.
 *
 * The registry storage grows on demand (realloc, doubling) instead of using
 * a fixed-size table. REG_MAX_FDS is only a hard safety ceiling on the
 * logical fd namespace/growth, not a preallocated array size, so normal
 * session counts never run into an artificial EMFILE.
 */
#define REG_INITIAL_CAP 256
#define REG_MAX_FDS (1 << 20) /* ~1M logical descriptors; safety cap only */
#define REG_FD_BASE 0x40000000

static SRWLOCK reg_lock = SRWLOCK_INIT;
static SOCKET *reg_slots;       /* heap-allocated, grown via realloc */
static size_t reg_capacity;     /* number of usable slots currently allocated */
static volatile LONG reg_state; /* 0 = new, 1 = initializing, 2 = ready */

static void
reg_init (void)
{
    LONG state;

    state = InterlockedCompareExchange (&reg_state, 1, 0);
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

/* Caller must hold reg_lock exclusively. Grows storage so that at least
 * index `need_idx` becomes valid, capped at REG_MAX_FDS. Returns 0 on
 * success (reg_capacity > need_idx), -1 if already at the hard ceiling or
 * out of memory. */
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

SOCKET
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

/* ------------------------------------------------------------------ */
/* WinSock lifecycle and error translation                             */
/* ------------------------------------------------------------------ */

static SRWLOCK wsa_lock = SRWLOCK_INIT;
static LONG wsa_refs;

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

int
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
        /* No free slot in the currently allocated storage: grow instead of
         * failing. Only returns -1 once REG_MAX_FDS is actually reached. */
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

void
hev_win_wsa_fini (void)
{
    AcquireSRWLockExclusive (&wsa_lock);

    if (wsa_refs <= 0) {
        /* A mismatched fini is a programming bug; never underflow. */
        ReleaseSRWLockExclusive (&wsa_lock);
        return;
    }

    wsa_refs--;
    if (wsa_refs == 0)
        WSACleanup ();

    ReleaseSRWLockExclusive (&wsa_lock);
}

static void
hev_win_set_errno_from_wsa_code (int error)
{
    switch (error) {
    case WSAEWOULDBLOCK:
        errno = EAGAIN;
        break;
    case WSAEINPROGRESS:
        errno = EINPROGRESS;
        break;
    case WSAEALREADY:
        errno = EALREADY;
        break;
    case WSAEISCONN:
        errno = EISCONN;
        break;
    case WSAECONNRESET:
        errno = ECONNRESET;
        break;
    case WSAENOTSOCK:
        errno = ENOTSOCK;
        break;
    case WSAEINTR:
        errno = EINTR;
        break;
    case WSAECONNABORTED:
        errno = ECONNABORTED;
        break;
    case WSAECONNREFUSED:
        errno = ECONNREFUSED;
        break;
    case WSAENETDOWN:
        errno = ENETDOWN;
        break;
    case WSAENETUNREACH:
        errno = ENETUNREACH;
        break;
    case WSAEHOSTUNREACH:
        errno = EHOSTUNREACH;
        break;
    case WSAEACCES:
        errno = EACCES;
        break;
    case WSAEMFILE:
        errno = EMFILE;
        break;
    case WSAENOBUFS:
        errno = ENOBUFS;
        break;
    case WSA_NOT_ENOUGH_MEMORY:
        errno = ENOMEM;
        break;
    case WSAEINVAL:
        errno = EINVAL;
        break;
    case WSAEFAULT:
        errno = EFAULT;
        break;
    case WSAEAFNOSUPPORT:
        errno = EAFNOSUPPORT;
        break;
    case WSAEPROTONOSUPPORT:
        errno = EPROTONOSUPPORT;
        break;
    case WSAETIMEDOUT:
        errno = ETIMEDOUT;
        break;
    case WSAESHUTDOWN:
        errno = EPIPE;
        break;
    default:
        errno = EIO;
        break;
    }
}

void
hev_win_set_errno_from_wsa (void)
{
    hev_win_set_errno_from_wsa_code (WSAGetLastError ());
}

/* ------------------------------------------------------------------ */
/* Socket API wrappers                                                 */
/* ------------------------------------------------------------------ */

static SOCKET
fd_socket (int fd)
{
    SOCKET s = hev_win_fd_to_socket (fd);

    if (s == INVALID_SOCKET)
        WSASetLastError (WSAENOTSOCK);

    return s;
}

static int
register_new_socket (SOCKET s)
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
hev_win_socket (int domain, int type, int protocol)
{
    SOCKET s;
    int fd;

    if (hev_win_wsa_init () < 0)
        return -1;

    s = WSASocketW (domain, type, protocol, NULL, 0, WSA_FLAG_OVERLAPPED);
    fd = register_new_socket (s);
    if (fd < 0)
        hev_win_wsa_fini ();

    return fd;
}

int
hev_win_accept (int fd, struct sockaddr *addr, socklen_t *addrlen)
{
    SOCKET s = fd_socket (fd);
    SOCKET c;
    int new_fd;
    u_long nonblock = 1;

    if (s == INVALID_SOCKET) {
        hev_win_set_errno_from_wsa ();
        return -1;
    }

    c = accept (s, addr, addrlen);
    if (c == INVALID_SOCKET) {
        hev_win_set_errno_from_wsa ();
        return -1;
    }

    /* The accepted socket owns its own Winsock lifecycle reference. */
    if (hev_win_wsa_init () < 0) {
        closesocket (c);
        return -1;
    }

    if (ioctlsocket (c, FIONBIO, &nonblock) != 0) {
        hev_win_set_errno_from_wsa ();
        closesocket (c);
        hev_win_wsa_fini ();
        return -1;
    }

    new_fd = hev_win_socket_to_fd (c);
    if (new_fd < 0) {
        closesocket (c);
        hev_win_wsa_fini ();
        errno = EMFILE;
        return -1;
    }

    return new_fd;
}

int
hev_win_socketpair (int domain, int type, int protocol, int sv[2])
{
    SOCKET listener = INVALID_SOCKET;
    SOCKET conn = INVALID_SOCKET;
    SOCKET peer = INVALID_SOCKET;
    struct sockaddr_in addr;
    int addrlen;
    int refs = 0;
    int fd0 = -1;
    int fd1 = -1;
    u_long nonblock = 1;

    (void)domain;
    (void)protocol;

    if (!sv || type != SOCK_STREAM) {
        errno = EINVAL;
        return -1;
    }

    sv[0] = -1;
    sv[1] = -1;

    /* One reference will belong to conn on success. */
    if (hev_win_wsa_init () < 0)
        return -1;
    refs = 1;

    listener = socket (AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (listener == INVALID_SOCKET)
        goto fail;

    memset (&addr, 0, sizeof (addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.S_un.S_addr = htonl (INADDR_LOOPBACK);
    addr.sin_port = 0;

    if (bind (listener, (struct sockaddr *)&addr, sizeof (addr)) != 0)
        goto fail;
    if (listen (listener, 1) != 0)
        goto fail;

    addrlen = sizeof (addr);
    if (getsockname (listener, (struct sockaddr *)&addr, &addrlen) != 0)
        goto fail;

    conn = socket (AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (conn == INVALID_SOCKET)
        goto fail;

    if (connect (conn, (struct sockaddr *)&addr, sizeof (addr)) != 0)
        goto fail;

    peer = accept (listener, NULL, NULL);
    if (peer == INVALID_SOCKET)
        goto fail;

    closesocket (listener);
    listener = INVALID_SOCKET;

    if (ioctlsocket (conn, FIONBIO, &nonblock) != 0)
        goto fail;
    if (ioctlsocket (peer, FIONBIO, &nonblock) != 0)
        goto fail;

    /* A second lifecycle reference belongs to peer. */
    if (hev_win_wsa_init () < 0)
        goto fail;
    refs = 2;

    WSASetLastError (0);
    fd0 = hev_win_socket_to_fd (conn);
    if (fd0 < 0) {
        errno = EMFILE;
        goto fail;
    }
    conn = INVALID_SOCKET; /* registry owns it now */

    WSASetLastError (0);
    fd1 = hev_win_socket_to_fd (peer);
    if (fd1 < 0) {
        errno = EMFILE;
        goto fail;
    }
    peer = INVALID_SOCKET; /* registry owns it now */

    sv[0] = fd0;
    sv[1] = fd1;
    return 0;

fail:
    if (WSAGetLastError () != 0)
        hev_win_set_errno_from_wsa ();

    if (fd0 >= 0) {
        SOCKET s = hev_win_fd_close_socket (fd0);
        if (s != INVALID_SOCKET)
            closesocket (s);
    }
    if (fd1 >= 0) {
        SOCKET s = hev_win_fd_close_socket (fd1);
        if (s != INVALID_SOCKET)
            closesocket (s);
    }
    if (conn != INVALID_SOCKET)
        closesocket (conn);
    if (peer != INVALID_SOCKET)
        closesocket (peer);
    if (listener != INVALID_SOCKET)
        closesocket (listener);

    while (refs-- > 0)
        hev_win_wsa_fini ();

    return -1;
}

int
hev_win_connect (int fd, const struct sockaddr *name, int namelen)
{
    SOCKET s = fd_socket (fd);

    if (s == INVALID_SOCKET) {
        hev_win_set_errno_from_wsa ();
        return -1;
    }

    if (connect (s, name, namelen) != 0) {
        hev_win_set_errno_from_wsa ();
        /* A pending non-blocking connect must surface as EINPROGRESS for the
         * generic retry logic in hev_task_io_socket_connect(); Winsock
         * reports it as WSAEWOULDBLOCK, which maps to EAGAIN above. */
        if (errno == EAGAIN)
            errno = EINPROGRESS;
        return -1;
    }

    return 0;
}

int
hev_win_bind (int fd, const struct sockaddr *name, int namelen)
{
    SOCKET s = fd_socket (fd);

    if (s == INVALID_SOCKET) {
        hev_win_set_errno_from_wsa ();
        return -1;
    }

    if (bind (s, name, namelen) != 0) {
        hev_win_set_errno_from_wsa ();
        return -1;
    }

    return 0;
}

int
hev_win_listen (int fd, int backlog)
{
    SOCKET s = fd_socket (fd);

    if (s == INVALID_SOCKET) {
        hev_win_set_errno_from_wsa ();
        return -1;
    }

    if (listen (s, backlog) != 0) {
        hev_win_set_errno_from_wsa ();
        return -1;
    }

    return 0;
}

int
hev_win_shutdown (int fd, int how)
{
    SOCKET s = fd_socket (fd);

    if (s == INVALID_SOCKET) {
        hev_win_set_errno_from_wsa ();
        return -1;
    }

    if (shutdown (s, how) != 0) {
        hev_win_set_errno_from_wsa ();
        return -1;
    }

    return 0;
}

int
hev_win_getsockname (int fd, struct sockaddr *name, int *namelen)
{
    SOCKET s = fd_socket (fd);

    if (s == INVALID_SOCKET) {
        hev_win_set_errno_from_wsa ();
        return -1;
    }

    if (getsockname (s, name, namelen) != 0) {
        hev_win_set_errno_from_wsa ();
        return -1;
    }

    return 0;
}

int
hev_win_getpeername (int fd, struct sockaddr *name, int *namelen)
{
    SOCKET s = fd_socket (fd);

    if (s == INVALID_SOCKET) {
        hev_win_set_errno_from_wsa ();
        return -1;
    }

    if (getpeername (s, name, namelen) != 0) {
        hev_win_set_errno_from_wsa ();
        return -1;
    }

    return 0;
}

int
hev_win_getsockopt (int fd, int level, int optname, char *optval, int *optlen)
{
    SOCKET s = fd_socket (fd);

    if (s == INVALID_SOCKET) {
        hev_win_set_errno_from_wsa ();
        return -1;
    }

    if (getsockopt (s, level, optname, optval, optlen) != 0) {
        hev_win_set_errno_from_wsa ();
        return -1;
    }

    return 0;
}

int
hev_win_setsockopt (int fd, int level, int optname, const char *optval,
                    int optlen)
{
    SOCKET s = fd_socket (fd);

    if (s == INVALID_SOCKET) {
        hev_win_set_errno_from_wsa ();
        return -1;
    }

    if (setsockopt (s, level, optname, optval, optlen) != 0) {
        hev_win_set_errno_from_wsa ();
        return -1;
    }

    return 0;
}

static int
flags_to_wsa (int flags)
{
    /* MSG_DONTWAIT is project-private; non-blocking is the socket state */
    return flags & (MSG_PEEK | MSG_OOB | MSG_DONTROUTE | MSG_WAITALL);
}

ssize_t
hev_win_recv (int fd, void *buf, size_t len, int flags)
{
    SOCKET s = fd_socket (fd);
    int res;

    if (s == INVALID_SOCKET) {
        hev_win_set_errno_from_wsa ();
        return -1;
    }

    res = recv (s, buf, (int)len, flags_to_wsa (flags));
    if (res < 0)
        hev_win_set_errno_from_wsa ();

    return res;
}

ssize_t
hev_win_send (int fd, const void *buf, size_t len, int flags)
{
    SOCKET s = fd_socket (fd);
    int res;

    if (s == INVALID_SOCKET) {
        hev_win_set_errno_from_wsa ();
        return -1;
    }

    res = send (s, buf, (int)len, flags_to_wsa (flags));
    if (res < 0)
        hev_win_set_errno_from_wsa ();

    return res;
}

ssize_t
hev_win_recvfrom (int fd, void *buf, size_t len, int flags,
                  struct sockaddr *addr, socklen_t *addrlen)
{
    SOCKET s = fd_socket (fd);
    int res;

    if (s == INVALID_SOCKET) {
        hev_win_set_errno_from_wsa ();
        return -1;
    }

    res = recvfrom (s, buf, (int)len, flags_to_wsa (flags), addr, addrlen);
    if (res < 0)
        hev_win_set_errno_from_wsa ();

    return res;
}

ssize_t
hev_win_sendto (int fd, const void *buf, size_t len, int flags,
                const struct sockaddr *addr, socklen_t addrlen)
{
    SOCKET s = fd_socket (fd);
    int res;

    if (s == INVALID_SOCKET) {
        hev_win_set_errno_from_wsa ();
        return -1;
    }

    res = sendto (s, buf, (int)len, flags_to_wsa (flags), addr, addrlen);
    if (res < 0)
        hev_win_set_errno_from_wsa ();

    return res;
}

ssize_t
hev_win_read (int fd, void *buf, size_t count)
{
    SOCKET s = hev_win_fd_to_socket (fd);

    if (s != INVALID_SOCKET)
        return hev_win_recv (fd, buf, count, 0);

    return _read (fd, buf, (unsigned int)count);
}

ssize_t
hev_win_write (int fd, const void *buf, size_t count)
{
    SOCKET s = hev_win_fd_to_socket (fd);

    if (s != INVALID_SOCKET)
        return hev_win_send (fd, buf, count, 0);

    return _write (fd, buf, (unsigned int)count);
}

static int
iov_to_wsabuf (const struct iovec *iov, int iovcnt, WSABUF *wbuf)
{
    int i;

    for (i = 0; i < iovcnt; i++) {
        wbuf[i].buf = (char *)iov[i].iov_base;
        wbuf[i].len = (ULONG)iov[i].iov_len;
    }

    return iovcnt;
}

#define WBUF_STACK 64

typedef struct _HevWinWSABuf HevWinWSABuf;

struct _HevWinWSABuf
{
    WSABUF stack[WBUF_STACK];
    WSABUF *buf;
};

static WSABUF *
wsabuf_get (HevWinWSABuf *w, const struct iovec *iov, int iovcnt)
{
    w->buf = w->stack;
    if (iovcnt > WBUF_STACK) {
        w->buf = (WSABUF *)malloc (sizeof (WSABUF) * iovcnt);
        if (!w->buf)
            return NULL;
    }
    iov_to_wsabuf (iov, iovcnt, w->buf);
    return w->buf;
}

static void
wsabuf_put (HevWinWSABuf *w)
{
    if (w->buf != w->stack)
        free (w->buf);
}

ssize_t
hev_win_readv (int fd, const struct iovec *iov, int iovcnt)
{
    SOCKET s = hev_win_fd_to_socket (fd);
    HevWinWSABuf wbuf;
    DWORD recvd = 0, flags = 0;
    int res;

    if (s == INVALID_SOCKET) {
        ssize_t total = 0;
        int i;

        for (i = 0; i < iovcnt; i++) {
            ssize_t r = _read (fd, iov[i].iov_base,
                               (unsigned int)iov[i].iov_len);
            if (r < 0)
                return total > 0 ? total : r;
            total += r;
            if ((size_t)r < iov[i].iov_len)
                break;
        }
        return total;
    }

    if (!wsabuf_get (&wbuf, iov, iovcnt)) {
        errno = ENOMEM;
        return -1;
    }

    res = WSARecv (s, wbuf.buf, iovcnt, &recvd, &flags, NULL, NULL);
    wsabuf_put (&wbuf);
    if (res != 0) {
        hev_win_set_errno_from_wsa ();
        return -1;
    }

    return (ssize_t)recvd;
}

ssize_t
hev_win_writev (int fd, const struct iovec *iov, int iovcnt)
{
    SOCKET s = hev_win_fd_to_socket (fd);
    HevWinWSABuf wbuf;
    DWORD sent = 0;
    int res;

    if (s == INVALID_SOCKET) {
        ssize_t total = 0;
        int i;

        for (i = 0; i < iovcnt; i++) {
            ssize_t r = _write (fd, iov[i].iov_base,
                                (unsigned int)iov[i].iov_len);
            if (r < 0)
                return total > 0 ? total : r;
            total += r;
            if ((size_t)r < iov[i].iov_len)
                break;
        }
        return total;
    }

    if (!wsabuf_get (&wbuf, iov, iovcnt)) {
        errno = ENOMEM;
        return -1;
    }

    res = WSASend (s, wbuf.buf, iovcnt, &sent, 0, NULL, NULL);
    wsabuf_put (&wbuf);
    if (res != 0) {
        hev_win_set_errno_from_wsa ();
        return -1;
    }

    return (ssize_t)sent;
}

static ssize_t
hev_win_recvmsg_impl (SOCKET s, struct msghdr *msg, int flags)
{
    HevWinWSABuf wbuf;
    DWORD recvd = 0, wflags = 0;
    int iovcnt = (int)msg->msg_iovlen;
    int res;

    if (!wsabuf_get (&wbuf, msg->msg_iov, iovcnt)) {
        errno = ENOMEM;
        return -1;
    }
    wflags = (DWORD)flags_to_wsa (flags);

    if (msg->msg_name) {
        res = WSARecvFrom (s, wbuf.buf, iovcnt, &recvd, &wflags,
                           (struct sockaddr *)msg->msg_name,
                           &msg->msg_namelen, NULL, NULL);
    } else {
        res = WSARecv (s, wbuf.buf, iovcnt, &recvd, &wflags, NULL, NULL);
    }
    wsabuf_put (&wbuf);
    if (res != 0) {
        hev_win_set_errno_from_wsa ();
        return -1;
    }

    msg->msg_flags = (int)wflags;

    return (ssize_t)recvd;
}

static ssize_t
hev_win_sendmsg_impl (SOCKET s, const struct msghdr *msg, int flags)
{
    HevWinWSABuf wbuf;
    DWORD sent = 0;
    int iovcnt = (int)msg->msg_iovlen;
    int res;

    if (!wsabuf_get (&wbuf, msg->msg_iov, iovcnt)) {
        errno = ENOMEM;
        return -1;
    }

    if (msg->msg_name) {
        res = WSASendTo (s, wbuf.buf, iovcnt, &sent, (DWORD)flags_to_wsa (flags),
                         (const struct sockaddr *)msg->msg_name,
                         msg->msg_namelen, NULL, NULL);
    } else {
        res = WSASend (s, wbuf.buf, iovcnt, &sent, (DWORD)flags_to_wsa (flags),
                       NULL, NULL);
    }
    wsabuf_put (&wbuf);
    if (res != 0) {
        hev_win_set_errno_from_wsa ();
        return -1;
    }

    return (ssize_t)sent;
}

ssize_t
hev_win_recvmsg (int fd, struct msghdr *msg, int flags)
{
    SOCKET s = fd_socket (fd);

    if (s == INVALID_SOCKET) {
        hev_win_set_errno_from_wsa ();
        return -1;
    }

    return hev_win_recvmsg_impl (s, msg, flags);
}

ssize_t
hev_win_sendmsg (int fd, const struct msghdr *msg, int flags)
{
    SOCKET s = fd_socket (fd);

    if (s == INVALID_SOCKET) {
        hev_win_set_errno_from_wsa ();
        return -1;
    }

    return hev_win_sendmsg_impl (s, msg, flags);
}

ssize_t
hev_win_recvmmsg (int fd, struct mmsghdr *msgv, unsigned int vlen, int flags,
                  void *timeout)
{
    SOCKET s = fd_socket (fd);
    unsigned int i;
    ssize_t r;

    (void)timeout;

    if (s == INVALID_SOCKET) {
        hev_win_set_errno_from_wsa ();
        return -1;
    }

    for (i = 0; i < vlen; i++) {
        r = hev_win_recvmsg_impl (s, &msgv[i].msg_hdr, flags);
        if (r < 0) {
            if (i > 0)
                return (ssize_t)i;
            return -1;
        }
        msgv[i].msg_len = (unsigned int)r;
    }

    return (ssize_t)vlen;
}

ssize_t
hev_win_sendmmsg (int fd, struct mmsghdr *msgv, unsigned int vlen, int flags)
{
    SOCKET s = fd_socket (fd);
    unsigned int i;
    ssize_t r;

    if (s == INVALID_SOCKET) {
        hev_win_set_errno_from_wsa ();
        return -1;
    }

    for (i = 0; i < vlen; i++) {
        r = hev_win_sendmsg_impl (s, &msgv[i].msg_hdr, flags);
        if (r < 0) {
            if (i > 0)
                return (ssize_t)i;
            return -1;
        }
        msgv[i].msg_len = (unsigned int)r;
    }

    return (ssize_t)vlen;
}

int
hev_win_ioctl (int fd, long request, void *argp)
{
    SOCKET s = hev_win_fd_to_socket (fd);
    u_long mode;
    int res;

    if (s != INVALID_SOCKET) {
        if (request != FIONBIO) {
            errno = EINVAL;
            return -1;
        }
        mode = argp ? *(u_long *)argp : 0;
        res = ioctlsocket (s, FIONBIO, &mode);
        if (res != 0) {
            hev_win_set_errno_from_wsa ();
            return -1;
        }
        return 0;
    }

    /* CRT file descriptor: FIONBIO is a no-op for files */
    (void)request;
    (void)argp;

    return 0;
}

static SOCKET
hev_win_duplicate_socket_handle (SOCKET s)
{
    WSAPROTOCOL_INFOW info;

    memset (&info, 0, sizeof (info));
    if (WSADuplicateSocketW (s, GetCurrentProcessId (), &info) != 0)
        return INVALID_SOCKET;

    return WSASocketW (FROM_PROTOCOL_INFO, FROM_PROTOCOL_INFO,
                       FROM_PROTOCOL_INFO, &info, 0, WSA_FLAG_OVERLAPPED);
}

int
hev_win_dup (int oldfd)
{
    SOCKET s = hev_win_fd_to_socket (oldfd);
    SOCKET copy;
    int fd;

    if (s == INVALID_SOCKET)
        return _dup (oldfd);

    if (hev_win_wsa_init () < 0)
        return -1;

    copy = hev_win_duplicate_socket_handle (s);
    if (copy == INVALID_SOCKET) {
        hev_win_set_errno_from_wsa ();
        hev_win_wsa_fini ();
        return -1;
    }

    fd = register_new_socket (copy);
    if (fd < 0)
        hev_win_wsa_fini ();

    return fd;
}

int
hev_win_dup2 (int oldfd, int newfd)
{
    SOCKET s = hev_win_fd_to_socket (oldfd);
    SOCKET copy;
    SOCKET replaced;
    int idx;

    if (s == INVALID_SOCKET)
        return _dup2 (oldfd, newfd);

    if (oldfd == newfd)
        return newfd;

    idx = reg_index_from_fd (newfd);
    if (idx < 0) {
        /* Mixing a logical socket into the CRT descriptor namespace would
         * reintroduce the collision this registry is specifically avoiding. */
        errno = EINVAL;
        return -1;
    }

    if (hev_win_wsa_init () < 0)
        return -1;

    copy = hev_win_duplicate_socket_handle (s);
    if (copy == INVALID_SOCKET) {
        hev_win_set_errno_from_wsa ();
        hev_win_wsa_fini ();
        return -1;
    }

    {
        u_long nonblock = 1;
        if (ioctlsocket (copy, FIONBIO, &nonblock) != 0) {
            hev_win_set_errno_from_wsa ();
            closesocket (copy);
            hev_win_wsa_fini ();
            return -1;
        }
    }

    reg_init ();
    AcquireSRWLockExclusive (&reg_lock);
    if ((size_t)idx >= reg_capacity && reg_grow_locked ((size_t)idx) < 0) {
        ReleaseSRWLockExclusive (&reg_lock);
        closesocket (copy);
        hev_win_wsa_fini ();
        errno = EMFILE;
        return -1;
    }
    replaced = reg_slots[idx];
    reg_slots[idx] = copy;
    ReleaseSRWLockExclusive (&reg_lock);

    if (replaced != INVALID_SOCKET) {
        closesocket (replaced);
        hev_win_wsa_fini ();
    }

    return newfd;
}

int
hev_win_close (int fd)
{
    SOCKET s = hev_win_fd_close_socket (fd);

    if (s != INVALID_SOCKET) {
        int res = closesocket (s);

        if (res != 0)
            hev_win_set_errno_from_wsa ();

        /* The registry no longer owns this socket even if closesocket failed. */
        hev_win_wsa_fini ();
        return res == 0 ? 0 : -1;
    }

    return _close (fd);
}

int
hev_win_getaddrinfo (const char *node, const char *service,
                     const struct addrinfo *hints, struct addrinfo **res)
{
    int ret;

    if (hev_win_wsa_init () < 0)
        return EAI_FAIL;

    ret = getaddrinfo (node, service, hints, res);
    if (ret != 0)
        hev_win_set_errno_from_wsa_code (ret);

    hev_win_wsa_fini ();
    return ret;
}

int
hev_win_getnameinfo (const struct sockaddr *addr, socklen_t addrlen,
                     char *node, socklen_t nodelen, char *service,
                     socklen_t servicelen, int flags)
{
    int ret;

    if (hev_win_wsa_init () < 0)
        return EAI_FAIL;

    ret = getnameinfo (addr, addrlen, node, nodelen, service, servicelen,
                       flags);
    if (ret != 0)
        hev_win_set_errno_from_wsa_code (ret);

    hev_win_wsa_fini ();
    return ret;
}

void
hev_win_freeaddrinfo (struct addrinfo *res)
{
    freeaddrinfo (res);
}

int
hev_win_clock_gettime (int clock_id, struct timespec *ts)
{
    LARGE_INTEGER freq, counter;

    (void)clock_id; /* always monotonic on Windows */

    if (!ts)
        return -1;

    if (!QueryPerformanceFrequency (&freq) ||
        !QueryPerformanceCounter (&counter))
        return -1;

    ts->tv_sec = (time_t)(counter.QuadPart / freq.QuadPart);
    ts->tv_nsec =
        (long)((counter.QuadPart % freq.QuadPart) * 1000000000LL /
               freq.QuadPart);

    return 0;
}

void
hev_win_usleep (unsigned int usec)
{
    if (usec == 0) {
        Sleep (0);
        return;
    }

    /* Sleep(0) would turn sub-millisecond sleeps into a busy yield. */
    Sleep ((usec + 999U) / 1000U);
}

int
hev_win_pipe (int pipefd[2])
{
    return hev_win_socketpair (AF_INET, SOCK_STREAM, 0, pipefd);
}

int
hev_win_poll (struct pollfd *fds, int nfds, int timeout)
{
    WSAPOLLFD stack_fds[64];
    int stack_map[64];
    WSAPOLLFD *native = stack_fds;
    int *map = stack_map;
    int invalid = 0;
    int valid = 0;
    int i;
    int res;

    if (nfds < 0 || (nfds > 0 && !fds)) {
        errno = EINVAL;
        return -1;
    }

    if (nfds > (int)(sizeof (stack_fds) / sizeof (stack_fds[0]))) {
        native = (WSAPOLLFD *)malloc (sizeof (*native) * (size_t)nfds);
        map = (int *)malloc (sizeof (*map) * (size_t)nfds);
        if (!native || !map) {
            free (native == stack_fds ? NULL : native);
            free (map == stack_map ? NULL : map);
            errno = ENOMEM;
            return -1;
        }
    }

    for (i = 0; i < nfds; i++) {
        SOCKET s;

        fds[i].revents = 0;
        if ((intptr_t)fds[i].fd < 0)
            continue;

        s = hev_win_fd_to_socket ((int)fds[i].fd);
        if (s == INVALID_SOCKET) {
            fds[i].revents = POLLNVAL;
            invalid++;
            continue;
        }

        native[valid].fd = s;
        native[valid].events = fds[i].events;
        native[valid].revents = 0;
        map[valid] = i;
        valid++;
    }

    if (valid == 0) {
        if (invalid == 0) {
            if (timeout < 0)
                Sleep (INFINITE);
            else if (timeout > 0)
                Sleep ((DWORD)timeout);
        }
        res = invalid;
        goto exit;
    }

    /* POSIX poll returns immediately when any descriptor is invalid. */
    res = WSAPoll (native, (ULONG)valid, invalid ? 0 : timeout);
    if (res == SOCKET_ERROR) {
        hev_win_set_errno_from_wsa ();
        res = -1;
        goto exit;
    }

    for (i = 0; i < valid; i++)
        fds[map[i]].revents = native[i].revents;

    res += invalid;

exit:
    if (native != stack_fds)
        free (native);
    if (map != stack_map)
        free (map);

    return res;
}

int
hev_win_openat (int dirfd, const char *pathname, int flags, ...)
{
    va_list ap;
    int mode = 0;
    int fd;

    (void)dirfd; /* relative paths are not supported on Windows */

    if (flags & _O_CREAT) {
        va_start (ap, flags);
        mode = va_arg (ap, int);
        va_end (ap);
    }

    fd = _open (pathname, flags, mode);
    if (fd < 0)
        return -1;

    return fd;
}

int
hev_win_fcntl (int fd, int cmd, ...)
{
    (void)fd;

    /*
     * Descriptors handled by this layer are non-blocking by construction:
     * registry sockets are created with FIONBIO, and the CRT file shim treats
     * everything as non-blocking (O_NONBLOCK == 0).  Report and accept the
     * status flags without pretending to support full POSIX fcntl semantics.
     */
    switch (cmd) {
    case F_GETFD:
        return 0;
    case F_SETFD:
        return 0;
    case F_GETFL:
        return O_NONBLOCK;
    case F_SETFL:
        return 0;
    default:
        errno = EINVAL;
        return -1;
    }
}

/* ------------------------------------------------------------------ */
/* UTF-8 path handling                                                 */
/* ------------------------------------------------------------------ */

wchar_t *
hev_win_utf8_to_wide (const char *utf8)
{
    wchar_t *wide;
    int len;

    if (!utf8) {
        errno = EINVAL;
        return NULL;
    }

    len = MultiByteToWideChar (CP_UTF8, 0, utf8, -1, NULL, 0);
    if (len <= 0) {
        errno = EINVAL;
        return NULL;
    }

    wide = (wchar_t *)malloc (sizeof (wchar_t) * (size_t)len);
    if (!wide) {
        errno = ENOMEM;
        return NULL;
    }

    if (MultiByteToWideChar (CP_UTF8, 0, utf8, -1, wide, len) <= 0) {
        free (wide);
        errno = EINVAL;
        return NULL;
    }

    return wide;
}

FILE *
hev_win_fopen (const char *path, const char *mode)
{
    wchar_t wmode[8];
    FILE *fp;
    wchar_t *wpath;

    if (!path || !mode) {
        errno = EINVAL;
        return NULL;
    }

    /* The mode string is pure ASCII, but widen it uniformly anyway. */
    if (MultiByteToWideChar (CP_UTF8, 0, mode, -1, wmode,
                             (int)(sizeof (wmode) / sizeof (wmode[0]))) <= 0) {
        errno = EINVAL;
        return NULL;
    }

    wpath = hev_win_utf8_to_wide (path);
    if (!wpath)
        return NULL;

    fp = _wfopen (wpath, wmode);
    free (wpath);

    return fp;
}

/* ------------------------------------------------------------------ */
/* end of file                                                         */
/* ------------------------------------------------------------------ */
