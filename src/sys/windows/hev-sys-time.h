/*
 ============================================================================
 Name        : hev-sys-time.h
 Author      : Heiher <r@hev.cc>
 Copyright   : Copyright (c) 2026 everyone.
 Description : Clock functions
 ============================================================================
 */

#ifndef __HEV_SYS_TIME_H__
#define __HEV_SYS_TIME_H__

#ifdef __cplusplus
extern "C" {
#endif

int hev_sys_clock_gettime (int clock_id, struct timespec *ts);

#ifdef __cplusplus
}
#endif

#endif /* __HEV_SYS_TIME_H__ */
