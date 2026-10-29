/*
 ============================================================================
 Name        : hev-sys-mutex.h
 Author      : Heiher <r@hev.cc>
 Copyright   : Copyright (c) 2026 everyone.
 Description : Mutex
 ============================================================================
 */

#ifndef __HEV_SYS_UNIX_MUTEX_H__
#define __HEV_SYS_UNIX_MUTEX_H__

typedef pthread_mutex_t HevSysMutex;

#define HEV_SYS_MUTEX_INITIALIZER PTHREAD_MUTEX_INITIALIZER

#define hev_sys_mutex_init(m) pthread_mutex_init ((m), NULL)
#define hev_sys_mutex_fini(m) pthread_mutex_destroy (m)
#define hev_sys_mutex_lock(m) pthread_mutex_lock (m)
#define hev_sys_mutex_unlock(m) pthread_mutex_unlock (m)

#endif /* __HEV_SYS_UNIX_MUTEX_H__ */
