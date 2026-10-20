/* hev-win-compat.h - POSIX/MSVC compatibility shim for hev-task-system */

#ifndef __HEV_WIN_COMPAT_H__
#define __HEV_WIN_COMPAT_H__

#ifndef _WIN32
#error "This compatibility header is only for Windows builds"
#endif

#ifndef _WIN32_WINNT
#define _WIN32_WINNT 0x0600
#endif

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

#include <winsock2.h>
#include <ws2tcpip.h>
#include <windows.h>
#include <wincrypt.h>
#include <io.h>
#include <fcntl.h>
#include <process.h>
#include <basetsd.h>
#include <errno.h>
#include <stdarg.h>
#include <sys/types.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <wchar.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Remove Windows SDK <inaddr.h> macro leaks project-wide. */
#undef s_addr
#undef s_host
#undef s_net
#undef s_imp
#undef s_impno
#undef s_lh

#ifndef EAI_SYSTEM
#define EAI_SYSTEM 11
#endif

typedef SSIZE_T ssize_t;
typedef unsigned int mode_t;

#ifndef IFNAMSIZ
#define IFNAMSIZ 32
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

#ifndef O_NONBLOCK
#define O_NONBLOCK 0
#endif

#ifndef F_GETFD
#define F_GETFD 1
#endif
#ifndef F_SETFD
#define F_SETFD 2
#endif
#ifndef F_GETFL
#define F_GETFL 3
#endif
#ifndef F_SETFL
#define F_SETFL 4
#endif

#define getpid _getpid
#define open _open
#define openat hev_win_openat
#define lseek _lseek

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

#ifndef CLOCK_MONOTONIC
#define CLOCK_MONOTONIC 1
#endif
#ifndef CLOCK_REALTIME
#define CLOCK_REALTIME 0
#endif

SOCKET hev_win_fd_to_socket (int fd);
int hev_win_socket_to_fd (SOCKET s);
SOCKET hev_win_fd_close_socket (int fd);

void hev_win_set_errno_from_wsa (void);

int hev_win_wsa_init (void);
void hev_win_wsa_fini (void);

int hev_win_socket (int domain, int type, int protocol);
int hev_win_accept (int s, struct sockaddr *addr, socklen_t *addrlen);
int hev_win_socketpair (int domain, int type, int protocol, int sv[2]);
int hev_win_connect (int s, const struct sockaddr *name, int namelen);
int hev_win_bind (int s, const struct sockaddr *name, int namelen);
int hev_win_listen (int s, int backlog);
int hev_win_shutdown (int s, int how);
int hev_win_getsockname (int s, struct sockaddr *name, int *namelen);
int hev_win_getpeername (int s, struct sockaddr *name, int *namelen);
int hev_win_getsockopt (int s, int level, int optname, char *optval, int *optlen);
int hev_win_setsockopt (int s, int level, int optname, const char *optval, int optlen);

ssize_t hev_win_recv (int fd, void *buf, size_t len, int flags);
ssize_t hev_win_send (int fd, const void *buf, size_t len, int flags);
ssize_t hev_win_recvfrom (int fd, void *buf, size_t len, int flags, struct sockaddr *addr, socklen_t *addrlen);
ssize_t hev_win_sendto (int fd, const void *buf, size_t len, int flags, const struct sockaddr *addr, socklen_t addrlen);

ssize_t hev_win_read (int fd, void *buf, size_t count);
ssize_t hev_win_write (int fd, const void *buf, size_t count);
ssize_t hev_win_readv (int fd, const struct iovec *iov, int iovcnt);
ssize_t hev_win_writev (int fd, const struct iovec *iov, int iovcnt);

ssize_t hev_win_recvmsg (int fd, struct msghdr *msg, int flags);
ssize_t hev_win_sendmsg (int fd, const struct msghdr *msg, int flags);
ssize_t hev_win_recvmmsg (int fd, struct mmsghdr *msgv, unsigned int vlen, int flags, void *timeout);
ssize_t hev_win_sendmmsg (int fd, struct mmsghdr *msgv, unsigned int vlen, int flags);

int hev_win_ioctl (int fd, long request, void *argp);
int hev_win_dup (int oldfd);
int hev_win_dup2 (int oldfd, int newfd);
int hev_win_close (int fd);

int hev_win_getaddrinfo (const char *node, const char *service, const struct addrinfo *hints, struct addrinfo **res);
int hev_win_getnameinfo (const struct sockaddr *addr, socklen_t addrlen, char *node, socklen_t nodelen, char *service, socklen_t servicelen, int flags);
void hev_win_freeaddrinfo (struct addrinfo *res);

int hev_win_clock_gettime (int clock_id, struct timespec *ts);
void hev_win_usleep (unsigned int usec);
int hev_win_pipe (int pipefd[2]);
int hev_win_openat (int dirfd, const char *pathname, int flags, ...);
int hev_win_poll (struct pollfd *fds, int nfds, int timeout);
int hev_win_fcntl (int fd, int cmd, ...);

wchar_t *hev_win_utf8_to_wide (const char *utf8);

FILE *hev_win_fopen (const char *path, const char *mode);

#define socket hev_win_socket
#define accept hev_win_accept
#define socketpair hev_win_socketpair
#define connect hev_win_connect
#define bind hev_win_bind
#define listen hev_win_listen
#define shutdown hev_win_shutdown
#define getsockname hev_win_getsockname
#define getpeername hev_win_getpeername
#define getsockopt hev_win_getsockopt
#define setsockopt hev_win_setsockopt
#define recv hev_win_recv
#define send hev_win_send
#define recvfrom hev_win_recvfrom
#define sendto hev_win_sendto

#ifndef SHUT_RD
#define SHUT_RD SD_RECEIVE
#endif
#ifndef SHUT_WR
#define SHUT_WR SD_SEND
#endif
#ifndef SHUT_RDWR
#define SHUT_RDWR SD_BOTH
#endif

#define read hev_win_read
#define write hev_win_write
#define readv hev_win_readv
#define writev hev_win_writev

#define recvmsg hev_win_recvmsg
#define sendmsg hev_win_sendmsg
#define recvmmsg hev_win_recvmmsg
#define sendmmsg hev_win_sendmmsg

#define ioctl hev_win_ioctl
#define dup hev_win_dup
#define dup2 hev_win_dup2
#define close hev_win_close
#define pipe hev_win_pipe

#define getaddrinfo hev_win_getaddrinfo
#define getnameinfo hev_win_getnameinfo
#define freeaddrinfo hev_win_freeaddrinfo

#define clock_gettime hev_win_clock_gettime
#define usleep hev_win_usleep
#define poll hev_win_poll
#define fcntl hev_win_fcntl

#define fopen hev_win_fopen

void hev_task_msvc_longjmp (void *env, int val);
#define _longjmp(env, val) hev_task_msvc_longjmp ((env), (val))

#ifndef strcasecmp
#define strcasecmp _stricmp
#endif
#ifndef strncasecmp
#define strncasecmp _strnicmp
#endif
#define strdup _strdup

#ifdef __cplusplus
}
#endif

#endif /* __HEV_WIN_COMPAT_H__ */
