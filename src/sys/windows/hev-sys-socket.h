/*
 ============================================================================
 Name        : hev-sys-socket.h
 Author      : Heiher <r@hev.cc>
 Copyright   : Copyright (c) 2026 everyone.
 Description : Socket operations
 ============================================================================
 */

#ifndef __HEV_SYS_SOCKET_H__
#define __HEV_SYS_SOCKET_H__

#ifdef __cplusplus
extern "C" {
#endif

int hev_sys_socket (int domain, int type, int protocol);
int hev_sys_accept (int s, struct sockaddr *addr, socklen_t *addrlen);
int hev_sys_socketpair (int domain, int type, int protocol, int sv[2]);
int hev_sys_connect (int s, const struct sockaddr *name, int namelen);
int hev_sys_bind (int s, const struct sockaddr *name, int namelen);
int hev_sys_listen (int s, int backlog);
int hev_sys_shutdown (int s, int how);
int hev_sys_setsockopt (int s, int level, int optname, const void *optval,
                        int optlen);
int hev_sys_getsockname (int s, struct sockaddr *addr, socklen_t *addrlen);
int hev_sys_getpeername (int s, struct sockaddr *addr, socklen_t *addrlen);
int hev_sys_ioctl (int fd, long request, void *argp);

#ifdef __cplusplus
}
#endif

#endif /* __HEV_SYS_SOCKET_H__ */
