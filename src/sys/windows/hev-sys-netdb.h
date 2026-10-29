/*
 ============================================================================
 Name        : hev-sys-netdb.h
 Author      : Heiher <r@hev.cc>
 Copyright   : Copyright (c) 2026 everyone.
 Description : Name resolution
 ============================================================================
 */

#ifndef __HEV_SYS_NETDB_H__
#define __HEV_SYS_NETDB_H__

#ifdef __cplusplus
extern "C" {
#endif

int hev_sys_getaddrinfo (const char *node, const char *service,
                         const struct addrinfo *hints, struct addrinfo **res);
int hev_sys_getnameinfo (const struct sockaddr *addr, socklen_t addrlen,
                         char *node, socklen_t nodelen, char *service,
                         socklen_t servicelen, int flags);
void hev_sys_freeaddrinfo (struct addrinfo *res);

#ifdef __cplusplus
}
#endif

#endif /* __HEV_SYS_NETDB_H__ */
