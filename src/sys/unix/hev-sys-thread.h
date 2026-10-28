/*
 ============================================================================
 Name        : hev-sys-thread.h
 Author      : Heiher <r@hev.cc>
 Copyright   : Copyright (c) 2026 everyone.
 Description : Threads, one-time initialization and fork handlers
 ============================================================================
 */

#ifndef __HEV_SYS_UNIX_THREAD_H__
#define __HEV_SYS_UNIX_THREAD_H__

typedef pthread_t HevSysThread;
typedef pthread_once_t HevSysOnce;

#define HEV_SYS_ONCE_INITIALIZER PTHREAD_ONCE_INIT

#define hev_sys_thread_create(t, entry, data) \
    pthread_create ((t), NULL, (entry), (data))
#define hev_sys_thread_join(t) pthread_join ((t), NULL)

#define hev_sys_once(once, func) pthread_once ((once), (func))

#define hev_sys_atfork(prepare, parent, child) \
    pthread_atfork ((prepare), (parent), (child))

#endif /* __HEV_SYS_UNIX_THREAD_H__ */
