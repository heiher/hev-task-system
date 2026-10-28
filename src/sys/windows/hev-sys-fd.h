/*
 ============================================================================
 Name        : hev-sys-fd.h
 Author      : Heiher <r@hev.cc>
 Copyright   : Copyright (c) 2026 everyone.
 Description : Virtual file descriptors for Winsock sockets
 ============================================================================
 */

#ifndef __HEV_SYS_FD_H__
#define __HEV_SYS_FD_H__

#ifdef __cplusplus
extern "C" {
#endif

int hev_sys_wsa_init (void);
void hev_sys_wsa_fini (void);

SOCKET hev_sys_fd_socket (int fd);
uintptr_t hev_sys_fd_to_token (int fd);
SOCKET hev_sys_token_to_socket (uintptr_t token);
SOCKET hev_sys_fd_close_socket (int fd);
int hev_sys_fd_register (SOCKET s);
int hev_sys_fd_replace (int fd, SOCKET s, SOCKET *old);

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

#ifdef __cplusplus
}
#endif

#endif /* __HEV_SYS_FD_H__ */
