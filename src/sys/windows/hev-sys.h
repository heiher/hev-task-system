/* hev-sys.h - POSIX types and constants of the public API on Windows */

#ifndef __HEV_SYS_WINDOWS_H__
#define __HEV_SYS_WINDOWS_H__

#include <winsock2.h>
#include <ws2tcpip.h>
#include <basetsd.h>

#ifdef __cplusplus
extern "C" {
#endif

#ifndef _SSIZE_T_DEFINED
#define _SSIZE_T_DEFINED
typedef SSIZE_T ssize_t;
#endif

#ifndef _MODE_T_DEFINED
#define _MODE_T_DEFINED
typedef unsigned int mode_t;
#endif

#ifndef MSG_DONTWAIT
#define MSG_DONTWAIT 0x40000000
#endif

#ifndef MSG_WAITFORONE
#define MSG_WAITFORONE 0x20000000
#endif

#ifndef PF_LOCAL
#define PF_LOCAL AF_INET
#endif

struct iovec
{
    void *iov_base;
    size_t iov_len;
};

struct msghdr
{
    void *msg_name;
    socklen_t msg_namelen;
    struct iovec *msg_iov;
    size_t msg_iovlen;
    void *msg_control;
    size_t msg_controllen;
    int msg_flags;
};

struct mmsghdr
{
    struct msghdr msg_hdr;
    unsigned int msg_len;
};

/* Descriptors are virtual: a socket is only usable through its fd. */
int hev_win_socket_to_fd (SOCKET s);
SOCKET hev_win_fd_to_socket (int fd);
int hev_win_close (int fd);

#ifdef __cplusplus
}
#endif

#endif /* __HEV_SYS_WINDOWS_H__ */
