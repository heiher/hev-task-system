/* hev-sys-socket.c - Socket management on top of Winsock */

#define HEV_SYS_RAW
#include "hev-sys-private.h"

int
hev_sys_socket (int domain, int type, int protocol)
{
    SOCKET s;
    int fd;

    if (hev_sys_wsa_init () < 0)
        return -1;

    s = WSASocketW (domain, type, protocol, NULL, 0, WSA_FLAG_OVERLAPPED);
    fd = hev_sys_fd_register (s);
    if (fd < 0)
        hev_sys_wsa_fini ();

    return fd;
}

int
hev_sys_accept (int fd, struct sockaddr *addr, socklen_t *addrlen)
{
    SOCKET s = hev_sys_fd_socket (fd);
    SOCKET c;
    int new_fd;
    u_long nonblock = 1;

    if (s == INVALID_SOCKET) {
        hev_sys_set_errno_from_wsa ();
        return -1;
    }

    c = accept (s, addr, addrlen);
    if (c == INVALID_SOCKET) {
        hev_sys_set_errno_from_wsa ();
        return -1;
    }

    if (hev_sys_wsa_init () < 0) {
        closesocket (c);
        return -1;
    }

    if (ioctlsocket (c, FIONBIO, &nonblock) != 0) {
        hev_sys_set_errno_from_wsa ();
        closesocket (c);
        hev_sys_wsa_fini ();
        return -1;
    }

    new_fd = hev_sys_socket_to_fd (c);
    if (new_fd < 0) {
        closesocket (c);
        hev_sys_wsa_fini ();
        errno = EMFILE;
        return -1;
    }

    return new_fd;
}

int
hev_sys_socketpair (int domain, int type, int protocol, int sv[2])
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

    if (hev_sys_wsa_init () < 0)
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

    if (hev_sys_wsa_init () < 0)
        goto fail;
    refs = 2;

    WSASetLastError (0);
    fd0 = hev_sys_socket_to_fd (conn);
    if (fd0 < 0) {
        errno = EMFILE;
        goto fail;
    }
    conn = INVALID_SOCKET;

    WSASetLastError (0);
    fd1 = hev_sys_socket_to_fd (peer);
    if (fd1 < 0) {
        errno = EMFILE;
        goto fail;
    }
    peer = INVALID_SOCKET;

    sv[0] = fd0;
    sv[1] = fd1;
    return 0;

fail:
    if (WSAGetLastError () != 0)
        hev_sys_set_errno_from_wsa ();

    if (fd0 >= 0) {
        SOCKET s = hev_sys_fd_close_socket (fd0);
        if (s != INVALID_SOCKET)
            closesocket (s);
    }
    if (fd1 >= 0) {
        SOCKET s = hev_sys_fd_close_socket (fd1);
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
        hev_sys_wsa_fini ();

    return -1;
}

int
hev_sys_connect (int fd, const struct sockaddr *name, int namelen)
{
    SOCKET s = hev_sys_fd_socket (fd);

    if (s == INVALID_SOCKET) {
        hev_sys_set_errno_from_wsa ();
        return -1;
    }

    if (connect (s, name, namelen) != 0) {
        hev_sys_set_errno_from_wsa ();
        if (errno == EAGAIN)
            errno = EINPROGRESS;
        return -1;
    }

    return 0;
}

int
hev_sys_bind (int fd, const struct sockaddr *name, int namelen)
{
    SOCKET s = hev_sys_fd_socket (fd);

    if (s == INVALID_SOCKET) {
        hev_sys_set_errno_from_wsa ();
        return -1;
    }
    if (bind (s, name, namelen) != 0) {
        hev_sys_set_errno_from_wsa ();
        return -1;
    }

    return 0;
}

int
hev_sys_listen (int fd, int backlog)
{
    SOCKET s = hev_sys_fd_socket (fd);

    if (s == INVALID_SOCKET) {
        hev_sys_set_errno_from_wsa ();
        return -1;
    }
    if (listen (s, backlog) != 0) {
        hev_sys_set_errno_from_wsa ();
        return -1;
    }

    return 0;
}

int
hev_sys_shutdown (int fd, int how)
{
    SOCKET s = hev_sys_fd_socket (fd);

    if (s == INVALID_SOCKET) {
        hev_sys_set_errno_from_wsa ();
        return -1;
    }
    if (shutdown (s, how) != 0) {
        hev_sys_set_errno_from_wsa ();
        return -1;
    }

    return 0;
}

int
hev_sys_setsockopt (int fd, int level, int optname, const void *optval,
                    int optlen)
{
    SOCKET s = hev_sys_fd_socket (fd);

    if (s == INVALID_SOCKET) {
        hev_sys_set_errno_from_wsa ();
        return -1;
    }
    if (setsockopt (s, level, optname, (const char *)optval, optlen) != 0) {
        hev_sys_set_errno_from_wsa ();
        return -1;
    }

    return 0;
}

int
hev_sys_ioctl (int fd, long request, void *argp)
{
    SOCKET s = hev_sys_fd_to_socket (fd);

    if (s != INVALID_SOCKET) {
        u_long mode;
        int res;

        if (request != FIONBIO) {
            errno = EINVAL;
            return -1;
        }

        mode = argp ? *(u_long *)argp : 0;
        res = ioctlsocket (s, FIONBIO, &mode);
        if (res != 0) {
            hev_sys_set_errno_from_wsa ();
            return -1;
        }

        return 0;
    }

    (void)request;
    (void)argp;

    return 0;
}
