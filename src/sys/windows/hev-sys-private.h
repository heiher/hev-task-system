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
#include "hev-sys-errno.h"
#include "hev-sys-fd.h"
#include "hev-sys-fdio.h"
#include "hev-sys-msg.h"
#include "hev-sys-netdb.h"
#include "hev-sys-poll.h"
#include "hev-sys-socket.h"
#include "hev-sys-thread.h"
#include "hev-sys-mutex.h"
#include "hev-sys-time.h"
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

int hev_sys_init (void);
void hev_sys_fini (void);

__declspec (noreturn) void hev_task_longjmp (void *env, int value);

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
