/* hev-sys-fdio.c - Descriptor operations for sockets and CRT descriptors */

#define HEV_SYS_RAW
#include "hev-sys-private.h"
#include "lib/misc/hev-compiler.h"

static SOCKET
hev_sys_duplicate_socket_handle (SOCKET s)
{
    WSAPROTOCOL_INFOW info;

    memset (&info, 0, sizeof (info));
    if (WSADuplicateSocketW (s, GetCurrentProcessId (), &info) != 0)
        return INVALID_SOCKET;

    return WSASocketW (FROM_PROTOCOL_INFO, FROM_PROTOCOL_INFO,
                       FROM_PROTOCOL_INFO, &info, 0, WSA_FLAG_OVERLAPPED);
}

int
hev_sys_dup (int oldfd)
{
    SOCKET s = hev_sys_fd_to_socket (oldfd);
    SOCKET copy;
    int fd;

    if (s == INVALID_SOCKET)
        return _dup (oldfd);

    if (hev_sys_wsa_init () < 0)
        return -1;

    copy = hev_sys_duplicate_socket_handle (s);
    if (copy == INVALID_SOCKET) {
        hev_sys_set_errno_from_wsa ();
        hev_sys_wsa_fini ();
        return -1;
    }

    fd = hev_sys_fd_register (copy);
    if (fd < 0)
        hev_sys_wsa_fini ();

    return fd;
}

int
hev_sys_dup2 (int oldfd, int newfd)
{
    SOCKET s = hev_sys_fd_to_socket (oldfd);
    SOCKET copy;
    SOCKET replaced;
    u_long nonblock = 1;

    if (s == INVALID_SOCKET)
        return _dup2 (oldfd, newfd);

    if (oldfd == newfd)
        return newfd;

    if (hev_sys_wsa_init () < 0)
        return -1;

    copy = hev_sys_duplicate_socket_handle (s);
    if (copy == INVALID_SOCKET) {
        hev_sys_set_errno_from_wsa ();
        hev_sys_wsa_fini ();
        return -1;
    }

    if (ioctlsocket (copy, FIONBIO, &nonblock) != 0) {
        hev_sys_set_errno_from_wsa ();
        closesocket (copy);
        hev_sys_wsa_fini ();
        return -1;
    }

    if (hev_sys_fd_replace (newfd, copy, &replaced) < 0) {
        closesocket (copy);
        hev_sys_wsa_fini ();
        return -1;
    }

    if (replaced != INVALID_SOCKET) {
        closesocket (replaced);
        hev_sys_wsa_fini ();
    }

    return newfd;
}

EXPORT_SYMBOL int
hev_sys_close (int fd)
{
    SOCKET s = hev_sys_fd_close_socket (fd);

    if (s != INVALID_SOCKET) {
        int res = closesocket (s);

        if (res != 0)
            hev_sys_set_errno_from_wsa ();

        hev_sys_wsa_fini ();
        return res == 0 ? 0 : -1;
    }

    return _close (fd);
}

int
hev_sys_pipe (int pipefd[2])
{
    return hev_sys_socketpair (AF_INET, SOCK_STREAM, 0, pipefd);
}

int
hev_sys_fcntl (int fd, int cmd, ...)
{
    (void)fd;

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

int
hev_sys_openat (int dirfd, const char *pathname, int flags, ...)
{
    va_list ap;
    int mode = 0;

    (void)dirfd;

    if (flags & _O_CREAT) {
        va_start (ap, flags);
        mode = va_arg (ap, int);
        va_end (ap);
    }

    return _open (pathname, flags, mode);
}
