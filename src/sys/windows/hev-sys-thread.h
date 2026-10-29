/*
 ============================================================================
 Name        : hev-sys-thread.h
 Author      : Heiher <r@hev.cc>
 Copyright   : Copyright (c) 2026 everyone.
 Description : Threads and one-time initialization
 ============================================================================
 */

#ifndef __HEV_SYS_THREAD_H__
#define __HEV_SYS_THREAD_H__

#ifdef __cplusplus
extern "C" {
#endif

typedef HANDLE HevSysThread;
typedef INIT_ONCE HevSysOnce;

#define HEV_SYS_ONCE_INITIALIZER INIT_ONCE_STATIC_INIT

int hev_sys_thread_create (HevSysThread *thread, void *(*entry) (void *),
                           void *data);
int hev_sys_thread_join (HevSysThread thread);

void hev_sys_once (HevSysOnce *once, void (*func) (void));

static inline int
hev_sys_atfork (void (*prepare) (void), void (*parent) (void),
                void (*child) (void))
{
    (void)prepare;
    (void)parent;
    (void)child;

    return 0;
}

#ifdef __cplusplus
}
#endif

#endif /* __HEV_SYS_THREAD_H__ */
