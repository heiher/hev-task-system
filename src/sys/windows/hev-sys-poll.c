/*
 ============================================================================
 Name        : hev-sys-poll.c
 Author      : Heiher <r@hev.cc>
 Copyright   : Copyright (c) 2026 everyone.
 Description : poll() on top of WSAPoll
 ============================================================================
 */

#if defined(_WIN32) && !defined(__MSYS__) && !defined(__CYGWIN__)

#define HEV_SYS_RAW
#include "sys/hev-sys-private.h"

int
hev_sys_poll (struct pollfd *fds, int nfds, int timeout)
{
    WSAPOLLFD stack_fds[64];
    int stack_map[64];
    WSAPOLLFD *native = stack_fds;
    int *map = stack_map;
    int invalid = 0;
    int valid = 0;
    int i;
    int res;

    if (nfds < 0 || (nfds > 0 && !fds)) {
        errno = EINVAL;
        return -1;
    }

    if (nfds > (int)(sizeof (stack_fds) / sizeof (stack_fds[0]))) {
        native = (WSAPOLLFD *)malloc (sizeof (*native) * (size_t)nfds);
        map = (int *)malloc (sizeof (*map) * (size_t)nfds);
        if (!native || !map) {
            free (native == stack_fds ? NULL : native);
            free (map == stack_map ? NULL : map);
            errno = ENOMEM;
            return -1;
        }
    }

    for (i = 0; i < nfds; i++) {
        SOCKET s;

        fds[i].revents = 0;
        if ((intptr_t)fds[i].fd < 0)
            continue;

        s = hev_sys_fd_to_socket ((int)fds[i].fd);
        if (s == INVALID_SOCKET) {
            fds[i].revents = POLLNVAL;
            invalid++;
            continue;
        }

        native[valid].fd = s;
        native[valid].events = fds[i].events;
        native[valid].revents = 0;
        map[valid] = i;
        valid++;
    }

    if (valid == 0) {
        if (invalid == 0) {
            if (timeout < 0)
                Sleep (INFINITE);
            else if (timeout > 0)
                Sleep ((DWORD)timeout);
        }
        res = invalid;
        goto exit;
    }

    res = WSAPoll (native, (ULONG)valid, invalid ? 0 : timeout);
    if (res == SOCKET_ERROR) {
        hev_sys_set_errno_from_wsa ();
        res = -1;
        goto exit;
    }

    for (i = 0; i < valid; i++)
        fds[map[i]].revents = native[i].revents;

    res += invalid;

exit:
    if (native != stack_fds)
        free (native);
    if (map != stack_map)
        free (map);

    return res;
}

#endif /* defined(_WIN32) && !defined(__MSYS__) && !defined(__CYGWIN__) */
