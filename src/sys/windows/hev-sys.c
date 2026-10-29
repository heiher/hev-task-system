/*
 ============================================================================
 Name        : hev-sys.c
 Author      : hev <r@hev.cc>
 Copyright   : Copyright (c) 2026 hev
 Description : System layer initialization
 ============================================================================
 */

#if defined(_WIN32) && !defined(__MSYS__) && !defined(__CYGWIN__)

#define HEV_SYS_RAW
#include "sys/hev-sys-private.h"

int
hev_sys_init (void)
{
    WSADATA data;
    int res;

    res = WSAStartup (MAKEWORD (2, 2), &data);
    if (res != 0) {
        WSASetLastError (res);
        return -1;
    }

    return 0;
}

void
hev_sys_fini (void)
{
    WSACleanup ();
}

#endif /* defined(_WIN32) && !defined(__MSYS__) && !defined(__CYGWIN__) */
