/*
 ============================================================================
 Name        : hev-sys-msg.c
 Author      : Heiher <r@hev.cc>
 Copyright   : Copyright (c) 2026 everyone.
 Description : Message and scatter/gather I/O on top of Winsock
 ============================================================================
 */

#if defined(_WIN32) && !defined(__MSYS__) && !defined(__CYGWIN__)

#define HEV_SYS_RAW
#include "sys/hev-sys-private.h"

#define WBUF_STACK 64

typedef struct _HevWinWSABuf HevWinWSABuf;

struct _HevWinWSABuf
{
    WSABUF stack[WBUF_STACK];
    WSABUF *buf;
};

static int
flags_to_wsa (int flags)
{
    return flags & (MSG_PEEK | MSG_OOB | MSG_DONTROUTE | MSG_WAITALL);
}

static WSABUF *
wsabuf_get (HevWinWSABuf *w, const struct iovec *iov, int iovcnt)
{
    int i;

    w->buf = w->stack;
    if (iovcnt > WBUF_STACK) {
        w->buf = (WSABUF *)malloc (sizeof (WSABUF) * iovcnt);
        if (!w->buf)
            return NULL;
    }

    for (i = 0; i < iovcnt; i++) {
        w->buf[i].buf = (char *)iov[i].iov_base;
        w->buf[i].len = (ULONG)iov[i].iov_len;
    }

    return w->buf;
}

static void
wsabuf_put (HevWinWSABuf *w)
{
    if (w->buf != w->stack)
        free (w->buf);
}

ssize_t
hev_sys_recv (int fd, void *buf, size_t len, int flags)
{
    SOCKET s = hev_sys_fd_socket (fd);
    int res;

    if (s == INVALID_SOCKET) {
        hev_sys_set_errno_from_wsa ();
        return -1;
    }

    res = recv (s, buf, (int)len, flags_to_wsa (flags));
    if (res < 0)
        hev_sys_set_errno_from_wsa ();

    return res;
}

ssize_t
hev_sys_send (int fd, const void *buf, size_t len, int flags)
{
    SOCKET s = hev_sys_fd_socket (fd);
    int res;

    if (s == INVALID_SOCKET) {
        hev_sys_set_errno_from_wsa ();
        return -1;
    }

    res = send (s, buf, (int)len, flags_to_wsa (flags));
    if (res < 0)
        hev_sys_set_errno_from_wsa ();

    return res;
}

ssize_t
hev_sys_recvfrom (int fd, void *buf, size_t len, int flags,
                  struct sockaddr *addr, socklen_t *addrlen)
{
    SOCKET s = hev_sys_fd_socket (fd);
    int res;

    if (s == INVALID_SOCKET) {
        hev_sys_set_errno_from_wsa ();
        return -1;
    }

    res = recvfrom (s, buf, (int)len, flags_to_wsa (flags), addr, addrlen);
    if (res < 0)
        hev_sys_set_errno_from_wsa ();

    return res;
}

ssize_t
hev_sys_sendto (int fd, const void *buf, size_t len, int flags,
                const struct sockaddr *addr, socklen_t addrlen)
{
    SOCKET s = hev_sys_fd_socket (fd);
    int res;

    if (s == INVALID_SOCKET) {
        hev_sys_set_errno_from_wsa ();
        return -1;
    }

    res = sendto (s, buf, (int)len, flags_to_wsa (flags), addr, addrlen);
    if (res < 0)
        hev_sys_set_errno_from_wsa ();

    return res;
}

ssize_t
hev_sys_read (int fd, void *buf, size_t count)
{
    SOCKET s = hev_sys_fd_to_socket (fd);

    if (s != INVALID_SOCKET)
        return hev_sys_recv (fd, buf, count, 0);

    return _read (fd, buf, (unsigned int)count);
}

ssize_t
hev_sys_write (int fd, const void *buf, size_t count)
{
    SOCKET s = hev_sys_fd_to_socket (fd);

    if (s != INVALID_SOCKET)
        return hev_sys_send (fd, buf, count, 0);

    return _write (fd, buf, (unsigned int)count);
}

ssize_t
hev_sys_readv (int fd, const struct iovec *iov, int iovcnt)
{
    SOCKET s = hev_sys_fd_to_socket (fd);
    HevWinWSABuf wbuf;
    DWORD recvd = 0, flags = 0;
    int res;

    if (s == INVALID_SOCKET) {
        ssize_t total = 0;
        int i;

        for (i = 0; i < iovcnt; i++) {
            ssize_t r =
                _read (fd, iov[i].iov_base, (unsigned int)iov[i].iov_len);
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
        hev_sys_set_errno_from_wsa ();
        return -1;
    }

    return (ssize_t)recvd;
}

ssize_t
hev_sys_writev (int fd, const struct iovec *iov, int iovcnt)
{
    SOCKET s = hev_sys_fd_to_socket (fd);
    HevWinWSABuf wbuf;
    DWORD sent = 0;
    int res;

    if (s == INVALID_SOCKET) {
        ssize_t total = 0;
        int i;

        for (i = 0; i < iovcnt; i++) {
            ssize_t r =
                _write (fd, iov[i].iov_base, (unsigned int)iov[i].iov_len);
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
        hev_sys_set_errno_from_wsa ();
        return -1;
    }

    return (ssize_t)sent;
}

static ssize_t
hev_sys_recvmsg_impl (SOCKET s, struct msghdr *msg, int flags)
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
                           (struct sockaddr *)msg->msg_name, &msg->msg_namelen,
                           NULL, NULL);
    } else {
        res = WSARecv (s, wbuf.buf, iovcnt, &recvd, &wflags, NULL, NULL);
    }
    wsabuf_put (&wbuf);
    if (res != 0) {
        hev_sys_set_errno_from_wsa ();
        return -1;
    }

    msg->msg_flags = (int)wflags;

    return (ssize_t)recvd;
}

static ssize_t
hev_sys_sendmsg_impl (SOCKET s, const struct msghdr *msg, int flags)
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
        res = WSASendTo (s, wbuf.buf, iovcnt, &sent,
                         (DWORD)flags_to_wsa (flags),
                         (const struct sockaddr *)msg->msg_name,
                         msg->msg_namelen, NULL, NULL);
    } else {
        res = WSASend (s, wbuf.buf, iovcnt, &sent, (DWORD)flags_to_wsa (flags),
                       NULL, NULL);
    }
    wsabuf_put (&wbuf);
    if (res != 0) {
        hev_sys_set_errno_from_wsa ();
        return -1;
    }

    return (ssize_t)sent;
}

ssize_t
hev_sys_recvmsg (int fd, struct msghdr *msg, int flags)
{
    SOCKET s = hev_sys_fd_socket (fd);

    if (s == INVALID_SOCKET) {
        hev_sys_set_errno_from_wsa ();
        return -1;
    }

    return hev_sys_recvmsg_impl (s, msg, flags);
}

ssize_t
hev_sys_sendmsg (int fd, const struct msghdr *msg, int flags)
{
    SOCKET s = hev_sys_fd_socket (fd);

    if (s == INVALID_SOCKET) {
        hev_sys_set_errno_from_wsa ();
        return -1;
    }

    return hev_sys_sendmsg_impl (s, msg, flags);
}

ssize_t
hev_sys_recvmmsg (int fd, struct mmsghdr *msgv, unsigned int vlen, int flags,
                  void *timeout)
{
    SOCKET s = hev_sys_fd_socket (fd);
    unsigned int i;
    ssize_t r;

    (void)timeout;

    if (s == INVALID_SOCKET) {
        hev_sys_set_errno_from_wsa ();
        return -1;
    }

    for (i = 0; i < vlen; i++) {
        r = hev_sys_recvmsg_impl (s, &msgv[i].msg_hdr, flags);
        if (r < 0)
            return i > 0 ? (ssize_t)i : -1;
        msgv[i].msg_len = (unsigned int)r;
    }

    return (ssize_t)vlen;
}

ssize_t
hev_sys_sendmmsg (int fd, struct mmsghdr *msgv, unsigned int vlen, int flags)
{
    SOCKET s = hev_sys_fd_socket (fd);
    unsigned int i;
    ssize_t r;

    if (s == INVALID_SOCKET) {
        hev_sys_set_errno_from_wsa ();
        return -1;
    }

    for (i = 0; i < vlen; i++) {
        r = hev_sys_sendmsg_impl (s, &msgv[i].msg_hdr, flags);
        if (r < 0)
            return i > 0 ? (ssize_t)i : -1;
        msgv[i].msg_len = (unsigned int)r;
    }

    return (ssize_t)vlen;
}

#endif /* defined(_WIN32) && !defined(__MSYS__) && !defined(__CYGWIN__) */
