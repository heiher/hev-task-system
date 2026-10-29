/*
 ============================================================================
 Name        : hev-sys-mutex.h
 Author      : Heiher <r@hev.cc>
 Copyright   : Copyright (c) 2026 everyone.
 Description : Mutex
 ============================================================================
 */

#ifndef __HEV_SYS_MUTEX_H__
#define __HEV_SYS_MUTEX_H__

#ifdef __cplusplus
extern "C" {
#endif

typedef SRWLOCK HevSysMutex;

#define HEV_SYS_MUTEX_INITIALIZER SRWLOCK_INIT

static inline void
hev_sys_mutex_init (HevSysMutex *m)
{
    InitializeSRWLock (m);
}

static inline void
hev_sys_mutex_fini (HevSysMutex *m)
{
    (void)m;
}

static inline void
hev_sys_mutex_lock (HevSysMutex *m)
{
    AcquireSRWLockExclusive (m);
}

static inline void
hev_sys_mutex_unlock (HevSysMutex *m)
{
    ReleaseSRWLockExclusive (m);
}

#ifdef __cplusplus
}
#endif

#endif /* __HEV_SYS_MUTEX_H__ */
