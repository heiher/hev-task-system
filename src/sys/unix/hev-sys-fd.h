/*
 ============================================================================
 Name        : hev-sys-fd.h
 Author      : Heiher <r@hev.cc>
 Copyright   : Copyright (c) 2026 everyone.
 Description : File descriptor to handle mapping
 ============================================================================
 */

#ifndef __HEV_SYS_UNIX_FD_H__
#define __HEV_SYS_UNIX_FD_H__

#define HEV_SYS_INVALID_SOCKET ((uintptr_t)-1)

#define hev_sys_fd_to_handle(fd) ((uintptr_t)get_osfhandle (fd))
#define hev_sys_handle_to_socket(handle) (handle)

#endif /* __HEV_SYS_UNIX_FD_H__ */
