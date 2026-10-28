/*
 ============================================================================
 Name        : hev-sys-poll.h
 Author      : Heiher <r@hev.cc>
 Copyright   : Copyright (c) 2026 everyone.
 Description : Poll on virtual file descriptors
 ============================================================================
 */

#ifndef __HEV_SYS_POLL_H__
#define __HEV_SYS_POLL_H__

#ifdef __cplusplus
extern "C" {
#endif

int hev_sys_poll (struct pollfd *fds, int nfds, int timeout);

#ifdef __cplusplus
}
#endif

#endif /* __HEV_SYS_POLL_H__ */
