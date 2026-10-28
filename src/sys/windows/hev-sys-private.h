/*
 ============================================================================
 Name        : hev-sys-private.h
 Author      : Heiher <r@hev.cc>
 Copyright   : Copyright (c) 2026 everyone.
 Description : Windows system layer of the library
 ============================================================================
 */

#ifndef __HEV_SYS_WINDOWS_PRIVATE_H__
#define __HEV_SYS_WINDOWS_PRIVATE_H__

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

#include <hev-sys.h>
#include "hev-sys-task-stack.h"
#include "hev-sys-task-execute.h"

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

#ifndef FD_CLOEXEC
#define FD_CLOEXEC 1
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

int hev_sys_wsa_init (void);
void hev_sys_wsa_fini (void);

SOCKET hev_sys_fd_socket (int fd);
uintptr_t hev_sys_fd_to_token (int fd);
SOCKET hev_sys_token_to_socket (uintptr_t token);
SOCKET hev_sys_fd_close_socket (int fd);
int hev_sys_fd_register (SOCKET s);
int hev_sys_fd_replace (int fd, SOCKET s, SOCKET *old);

void hev_sys_set_errno_from_wsa (void);
void hev_sys_set_errno_from_wsa_code (int error);

int hev_sys_socket (int domain, int type, int protocol);
int hev_sys_accept (int s, struct sockaddr *addr, socklen_t *addrlen);
int hev_sys_socketpair (int domain, int type, int protocol, int sv[2]);
int hev_sys_connect (int s, const struct sockaddr *name, int namelen);
int hev_sys_bind (int s, const struct sockaddr *name, int namelen);
int hev_sys_listen (int s, int backlog);
int hev_sys_shutdown (int s, int how);
int hev_sys_setsockopt (int s, int level, int optname, const void *optval,
                        int optlen);
int hev_sys_ioctl (int fd, long request, void *argp);

ssize_t hev_sys_recv (int fd, void *buf, size_t len, int flags);
ssize_t hev_sys_send (int fd, const void *buf, size_t len, int flags);
ssize_t hev_sys_recvfrom (int fd, void *buf, size_t len, int flags,
                          struct sockaddr *addr, socklen_t *addrlen);
ssize_t hev_sys_sendto (int fd, const void *buf, size_t len, int flags,
                        const struct sockaddr *addr, socklen_t addrlen);
ssize_t hev_sys_read (int fd, void *buf, size_t count);
ssize_t hev_sys_write (int fd, const void *buf, size_t count);
ssize_t hev_sys_readv (int fd, const struct iovec *iov, int iovcnt);
ssize_t hev_sys_writev (int fd, const struct iovec *iov, int iovcnt);
ssize_t hev_sys_recvmsg (int fd, struct msghdr *msg, int flags);
ssize_t hev_sys_sendmsg (int fd, const struct msghdr *msg, int flags);
ssize_t hev_sys_recvmmsg (int fd, struct mmsghdr *msgv, unsigned int vlen,
                          int flags, void *timeout);
ssize_t hev_sys_sendmmsg (int fd, struct mmsghdr *msgv, unsigned int vlen,
                          int flags);

int hev_sys_dup (int oldfd);
int hev_sys_dup2 (int oldfd, int newfd);
int hev_sys_pipe (int pipefd[2]);
int hev_sys_fcntl (int fd, int cmd, ...);
int hev_sys_openat (int dirfd, const char *pathname, int flags, ...);

int hev_sys_poll (struct pollfd *fds, int nfds, int timeout);

int hev_sys_getaddrinfo (const char *node, const char *service,
                         const struct addrinfo *hints, struct addrinfo **res);
int hev_sys_getnameinfo (const struct sockaddr *addr, socklen_t addrlen,
                         char *node, socklen_t nodelen, char *service,
                         socklen_t servicelen, int flags);
void hev_sys_freeaddrinfo (struct addrinfo *res);

int hev_sys_clock_gettime (int clock_id, struct timespec *ts);

__declspec (noreturn) void hev_task_longjmp (void *env, int value);

typedef HANDLE HevSysThread;
typedef INIT_ONCE HevSysOnce;
typedef SRWLOCK HevSysMutex;

#define HEV_SYS_ONCE_INITIALIZER INIT_ONCE_STATIC_INIT
#define HEV_SYS_MUTEX_INITIALIZER SRWLOCK_INIT

static inline void
hev_sys_mutex_init (HevSysMutex *m)
{
    InitializeSRWLock (m);
}

static inline void
hev_sys_mutex_fini (HevSysMutex *m)
{
    (void)m;
}

static inline void
hev_sys_mutex_lock (HevSysMutex *m)
{
    AcquireSRWLockExclusive (m);
}

static inline void
hev_sys_mutex_unlock (HevSysMutex *m)
{
    ReleaseSRWLockExclusive (m);
}

int hev_sys_thread_create (HevSysThread *thread, void *(*entry) (void *),
                           void *data);
int hev_sys_thread_join (HevSysThread thread);

void hev_sys_once (HevSysOnce *once, void (*func) (void));

static inline int
hev_sys_atfork (void (*prepare) (void), void (*parent) (void),
                void (*child) (void))
{
    (void)prepare;
    (void)parent;
    (void)child;

    return 0;
}

#define HEV_SYS_INVALID_SOCKET ((uintptr_t)INVALID_SOCKET)

static inline uintptr_t
hev_sys_fd_to_handle (int fd)
{
    return hev_sys_fd_to_token (fd);
}

static inline uintptr_t
hev_sys_handle_to_socket (uintptr_t handle)
{
    return (uintptr_t)hev_sys_token_to_socket (handle);
}

#ifndef HEV_SYS_RAW

#define socket hev_sys_socket
#define accept hev_sys_accept
#define socketpair hev_sys_socketpair
#define connect hev_sys_connect
#define bind hev_sys_bind
#define listen hev_sys_listen
#define shutdown hev_sys_shutdown
#define setsockopt hev_sys_setsockopt
#define recv hev_sys_recv
#define send hev_sys_send
#define recvfrom hev_sys_recvfrom
#define sendto hev_sys_sendto

#define read hev_sys_read
#define write hev_sys_write
#define readv hev_sys_readv
#define writev hev_sys_writev
#define recvmsg hev_sys_recvmsg
#define sendmsg hev_sys_sendmsg
#define recvmmsg hev_sys_recvmmsg
#define sendmmsg hev_sys_sendmmsg

#define ioctl hev_sys_ioctl
#define dup hev_sys_dup
#define dup2 hev_sys_dup2
#define close hev_sys_close
#define pipe hev_sys_pipe
#define poll hev_sys_poll
#define fcntl hev_sys_fcntl
#define open _open
#define openat hev_sys_openat

#define getaddrinfo hev_sys_getaddrinfo
#define getnameinfo hev_sys_getnameinfo
#define freeaddrinfo hev_sys_freeaddrinfo
#define clock_gettime hev_sys_clock_gettime

#define alloca _alloca
#define strdup _strdup
#define getpagesize() 4096

#endif /* HEV_SYS_RAW */

#define hev_longjmp(env, val) hev_task_longjmp ((void *)(env), (val))

#ifdef __cplusplus
}
#endif

#endif /* __HEV_SYS_WINDOWS_PRIVATE_H__ */
