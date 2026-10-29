/*
 ============================================================================
 Name        : hev-sys-errno.h
 Author      : Heiher <r@hev.cc>
 Copyright   : Copyright (c) 2026 everyone.
 Description : Winsock error to errno mapping
 ============================================================================
 */

#ifndef __HEV_SYS_ERRNO_H__
#define __HEV_SYS_ERRNO_H__

#ifdef __cplusplus
extern "C" {
#endif

void hev_sys_set_errno_from_wsa (void);
void hev_sys_set_errno_from_wsa_code (int error);

#ifdef __cplusplus
}
#endif

#endif /* __HEV_SYS_ERRNO_H__ */
