/* hev-sys.h - Windows system layer: POSIX names mapped to hev_win_* */

#ifndef __HEV_SYS_H__
#define __HEV_SYS_H__

#ifndef _WIN32_WINNT
#define _WIN32_WINNT 0x0600
#endif

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

#include <winsock2.h>
#include <ws2tcpip.h>
#include <windows.h>
#include <io.h>
#include <malloc.h>
#include <fcntl.h>
#include <errno.h>
#include <stdarg.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <sys/types.h>
#include <sys/stat.h>

#include "hev-win-types.h"

#ifdef __cplusplus
extern "C" {
#endif

#ifndef EAI_SYSTEM
#define EAI_SYSTEM 11
#endif

#ifndef IFNAMSIZ
#define IFNAMSIZ 32
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

#ifndef SHUT_RD
#define SHUT_RD SD_RECEIVE
#endif
#ifndef SHUT_WR
#define SHUT_WR SD_SEND
#endif
#ifndef SHUT_RDWR
#define SHUT_RDWR SD_BOTH
#endif

#ifndef CLOCK_MONOTONIC
#define CLOCK_MONOTONIC 1
#endif
#ifndef CLOCK_REALTIME
#define CLOCK_REALTIME 0
#endif

int hev_win_wsa_init (void);
void hev_win_wsa_fini (void);

SOCKET hev_win_fd_socket (int fd);
SOCKET hev_win_fd_close_socket (int fd);
int hev_win_fd_register (SOCKET s);
int hev_win_fd_replace (int fd, SOCKET s, SOCKET *old);

void hev_win_set_errno_from_wsa (void);
void hev_win_set_errno_from_wsa_code (int error);

int hev_win_socket (int domain, int type, int protocol);
int hev_win_accept (int s, struct sockaddr *addr, socklen_t *addrlen);
int hev_win_socketpair (int domain, int type, int protocol, int sv[2]);
int hev_win_connect (int s, const struct sockaddr *name, int namelen);
int hev_win_bind (int s, const struct sockaddr *name, int namelen);
int hev_win_listen (int s, int backlog);
int hev_win_shutdown (int s, int how);
int hev_win_setsockopt (int s, int level, int optname, const void *optval,
                        int optlen);
int hev_win_ioctl (int fd, long request, void *argp);

ssize_t hev_win_recv (int fd, void *buf, size_t len, int flags);
ssize_t hev_win_send (int fd, const void *buf, size_t len, int flags);
ssize_t hev_win_recvfrom (int fd, void *buf, size_t len, int flags,
                          struct sockaddr *addr, socklen_t *addrlen);
ssize_t hev_win_sendto (int fd, const void *buf, size_t len, int flags,
                        const struct sockaddr *addr, socklen_t addrlen);
ssize_t hev_win_read (int fd, void *buf, size_t count);
ssize_t hev_win_write (int fd, const void *buf, size_t count);
ssize_t hev_win_readv (int fd, const struct iovec *iov, int iovcnt);
ssize_t hev_win_writev (int fd, const struct iovec *iov, int iovcnt);
ssize_t hev_win_recvmsg (int fd, struct msghdr *msg, int flags);
ssize_t hev_win_sendmsg (int fd, const struct msghdr *msg, int flags);
ssize_t hev_win_recvmmsg (int fd, struct mmsghdr *msgv, unsigned int vlen,
                          int flags, void *timeout);
ssize_t hev_win_sendmmsg (int fd, struct mmsghdr *msgv, unsigned int vlen,
                          int flags);

int hev_win_dup (int oldfd);
int hev_win_dup2 (int oldfd, int newfd);
int hev_win_pipe (int pipefd[2]);
int hev_win_fcntl (int fd, int cmd, ...);
int hev_win_openat (int dirfd, const char *pathname, int flags, ...);

int hev_win_poll (struct pollfd *fds, int nfds, int timeout);

int hev_win_getaddrinfo (const char *node, const char *service,
                         const struct addrinfo *hints, struct addrinfo **res);
int hev_win_getnameinfo (const struct sockaddr *addr, socklen_t addrlen,
                         char *node, socklen_t nodelen, char *service,
                         socklen_t servicelen, int flags);
void hev_win_freeaddrinfo (struct addrinfo *res);

int hev_win_clock_gettime (int clock_id, struct timespec *ts);

void hev_win_stack_enter (void *bottom, void *limit);
void hev_win_stack_leave (void);
void hev_win_stack_swap (void *bottom, void *limit, void *saved[2]);
void hev_win_stack_restore (void *saved[2]);

void hev_win_aide_fini (void);

__declspec (noreturn) void hev_task_longjmp (void *env, int value);

#ifndef HEV_WIN_RAW

#define socket hev_win_socket
#define accept hev_win_accept
#define socketpair hev_win_socketpair
#define connect hev_win_connect
#define bind hev_win_bind
#define listen hev_win_listen
#define shutdown hev_win_shutdown
#define setsockopt hev_win_setsockopt
#define recv hev_win_recv
#define send hev_win_send
#define recvfrom hev_win_recvfrom
#define sendto hev_win_sendto

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
#define poll hev_win_poll
#define fcntl hev_win_fcntl
#define open _open
#define openat hev_win_openat

#define getaddrinfo hev_win_getaddrinfo
#define getnameinfo hev_win_getnameinfo
#define freeaddrinfo hev_win_freeaddrinfo
#define clock_gettime hev_win_clock_gettime

#define alloca _alloca
#define strdup _strdup
#define getpagesize() 4096

#endif /* HEV_WIN_RAW */

#define HEV_LONGJMP(env, val) hev_task_longjmp ((void *)(env), (val))

/* Heap stacks are outside the TEB stack range, which __chkstk relies on. */
#define HEV_STACK_RANGE_ENTER(task)            \
    hev_win_stack_enter ((task)->stack_bottom, \
                         hev_task_stack_get_base ((task)->stack))
#define HEV_STACK_RANGE_LEAVE() hev_win_stack_leave ()

#define HEV_TASK_CALL_EXECUTE(call, entry)                             \
    do {                                                               \
        void *saved[2];                                                \
        hev_win_stack_swap ((call)->stack_top, (void *)(call), saved); \
        hev_task_execute ((call), (entry));                            \
        hev_win_stack_restore (saved);                                 \
    } while (0)

#define HEV_AIDE_FINI() hev_win_aide_fini ()

#define HevSysMutex SRWLOCK
#define HEV_MUTEX_INIT(m) InitializeSRWLock (m)
#define HEV_MUTEX_FINI(m) ((void)0)
#define HEV_MUTEX_LOCK(m) AcquireSRWLockExclusive (m)
#define HEV_MUTEX_UNLOCK(m) ReleaseSRWLockExclusive (m)

#define HEV_IOCP_NATIVE 1
#define HEV_IOCP_HANDLE_T uintptr_t

#define HEV_FD_TO_HANDLE(fd) ((uintptr_t)hev_win_fd_to_socket (fd))

#ifdef __cplusplus
}
#endif

#endif /* __HEV_SYS_H__ */
