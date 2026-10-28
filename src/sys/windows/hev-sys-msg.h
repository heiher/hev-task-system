/*
 ============================================================================
 Name        : hev-sys-msg.h
 Author      : Heiher <r@hev.cc>
 Copyright   : Copyright (c) 2026 everyone.
 Description : Socket and file data transfer
 ============================================================================
 */

#ifndef __HEV_SYS_MSG_H__
#define __HEV_SYS_MSG_H__

#ifdef __cplusplus
extern "C" {
#endif

ssize_t hev_sys_recv (int fd, void *buf, size_t len, int flags);
ssize_t hev_sys_send (int fd, const void *buf, size_t len, int flags);
ssize_t hev_sys_recvfrom (int fd, void *buf, size_t len, int flags,
                          struct sockaddr *addr, socklen_t *addrlen);
ssize_t hev_sys_sendto (int fd, const void *buf, size_t len, int flags,
                        const struct sockaddr *addr, socklen_t addrlen);
ssize_t hev_sys_read (int fd, void *buf, size_t count);
ssize_t hev_sys_write (int fd, const void *buf, size_t count);
ssize_t hev_sys_readv (int fd, const struct iovec *iov, int iovcnt);
ssize_t hev_sys_writev (int fd, const struct iovec *iov, int iovcnt);
ssize_t hev_sys_recvmsg (int fd, struct msghdr *msg, int flags);
ssize_t hev_sys_sendmsg (int fd, const struct msghdr *msg, int flags);
ssize_t hev_sys_recvmmsg (int fd, struct mmsghdr *msgv, unsigned int vlen,
                          int flags, void *timeout);
ssize_t hev_sys_sendmmsg (int fd, struct mmsghdr *msgv, unsigned int vlen,
                          int flags);

#ifdef __cplusplus
}
#endif

#endif /* __HEV_SYS_MSG_H__ */
