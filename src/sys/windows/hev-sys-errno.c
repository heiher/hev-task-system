/*
 ============================================================================
 Name        : hev-sys-errno.c
 Author      : Heiher <r@hev.cc>
 Copyright   : Copyright (c) 2026 everyone.
 Description : Winsock error to errno mapping
 ============================================================================
 */

#if defined(_WIN32) && !defined(__MSYS__) && !defined(__CYGWIN__)

#define HEV_SYS_RAW
#include "sys/hev-sys-private.h"

void
hev_sys_set_errno_from_wsa_code (int error)
{
    switch (error) {
    case WSAEWOULDBLOCK:
        errno = EAGAIN;
        break;
    case WSAEBADF:
        errno = EBADF;
        break;
    case WSAEINPROGRESS:
        errno = EINPROGRESS;
        break;
    case WSAEMSGSIZE:
        errno = EMSGSIZE;
        break;
    case WSAEALREADY:
        errno = EALREADY;
        break;
    case WSAEISCONN:
        errno = EISCONN;
        break;
    case WSAENOTCONN:
        errno = ENOTCONN;
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
    case WSAEADDRINUSE:
        errno = EADDRINUSE;
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
    case WSAEOPNOTSUPP:
        errno = EOPNOTSUPP;
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
hev_sys_set_errno_from_wsa (void)
{
    hev_sys_set_errno_from_wsa_code (WSAGetLastError ());
}

#endif /* defined(_WIN32) && !defined(__MSYS__) && !defined(__CYGWIN__) */
