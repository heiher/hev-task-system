/*
 ============================================================================
 Name        : hev-sys-fdio.h
 Author      : Heiher <r@hev.cc>
 Copyright   : Copyright (c) 2026 everyone.
 Description : File descriptor operations
 ============================================================================
 */

#ifndef __HEV_SYS_FDIO_H__
#define __HEV_SYS_FDIO_H__

#ifdef __cplusplus
extern "C" {
#endif

int hev_sys_dup (int oldfd);
int hev_sys_dup2 (int oldfd, int newfd);
int hev_sys_pipe (int pipefd[2]);
int hev_sys_fcntl (int fd, int cmd, ...);
int hev_sys_openat (int dirfd, const char *pathname, int flags, ...);

#ifdef __cplusplus
}
#endif

#endif /* __HEV_SYS_FDIO_H__ */
