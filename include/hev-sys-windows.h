/*
 ============================================================================
 Name        : hev-sys-windows.h
 Author      : hev <r@hev.cc>
 Copyright   : Copyright (c) 2026 hev
 Description : POSIX types and constants of the public API on Windows
 ============================================================================
 */

#ifndef __HEV_SYS_WINDOWS_H__
#define __HEV_SYS_WINDOWS_H__

#include <winsock2.h>
#include <ws2tcpip.h>
#include <basetsd.h>
#include <malloc.h>
#include <fcntl.h>

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

#ifndef alloca
#define alloca _alloca
#endif

#ifndef getpagesize
#define getpagesize() 4096
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

#ifdef __cplusplus
}
#endif

#endif /* __HEV_SYS_WINDOWS_H__ */
