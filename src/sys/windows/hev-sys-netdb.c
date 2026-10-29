/*
 ============================================================================
 Name        : hev-sys-netdb.c
 Author      : Heiher <r@hev.cc>
 Copyright   : Copyright (c) 2026 everyone.
 Description : Name resolution on top of Winsock
 ============================================================================
 */

#if defined(_WIN32) && !defined(__MSYS__) && !defined(__CYGWIN__)

#define HEV_SYS_RAW
#include "sys/hev-sys-private.h"

int
hev_sys_getaddrinfo (const char *node, const char *service,
                     const struct addrinfo *hints, struct addrinfo **res)
{
    int ret;

    ret = getaddrinfo (node, service, hints, res);
    if (ret != 0)
        hev_sys_set_errno_from_wsa_code (ret);

    return ret;
}

int
hev_sys_getnameinfo (const struct sockaddr *addr, socklen_t addrlen, char *node,
                     socklen_t nodelen, char *service, socklen_t servicelen,
                     int flags)
{
    int ret;

    ret =
        getnameinfo (addr, addrlen, node, nodelen, service, servicelen, flags);
    if (ret != 0)
        hev_sys_set_errno_from_wsa_code (ret);

    return ret;
}

void
hev_sys_freeaddrinfo (struct addrinfo *res)
{
    freeaddrinfo (res);
}

#endif /* defined(_WIN32) && !defined(__MSYS__) && !defined(__CYGWIN__) */
