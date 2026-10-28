/*
 ============================================================================
 Name        : hev-sys-private.h
 Author      : Heiher <r@hev.cc>
 Copyright   : Copyright (c) 2026 everyone.
 Description : POSIX system layer of the library
 ============================================================================
 */

#ifndef __HEV_SYS_UNIX_PRIVATE_H__
#define __HEV_SYS_UNIX_PRIVATE_H__

#include <stdint.h>
#include <fcntl.h>
#include <pthread.h>
#include <sys/mman.h>
#include <sys/stat.h>

#include <hev-sys.h>

#if defined(__MSYS__)
#include <io.h>
#include "hev-windows-api.h"
#endif

typedef pthread_t HevSysThread;
typedef pthread_once_t HevSysOnce;
typedef pthread_mutex_t HevSysMutex;

typedef struct _HevSysTaskStack HevSysTaskStack;

struct _HevSysTaskStack
{
};

#define HEV_SYS_ONCE_INITIALIZER PTHREAD_ONCE_INIT
#define HEV_SYS_MUTEX_INITIALIZER PTHREAD_MUTEX_INITIALIZER

#define hev_longjmp(env, val) _longjmp ((env), (val))

#define hev_sys_task_stack_enter(self, base, limit) ((void)0)
#define hev_sys_task_stack_leave(self) ((void)0)

#define hev_sys_task_execute(ctx, top, limit, entry) \
    hev_task_execute ((ctx), (entry))

#define hev_sys_mutex_init(m) pthread_mutex_init ((m), NULL)
#define hev_sys_mutex_fini(m) pthread_mutex_destroy (m)
#define hev_sys_mutex_lock(m) pthread_mutex_lock (m)
#define hev_sys_mutex_unlock(m) pthread_mutex_unlock (m)

#define hev_sys_thread_create(t, entry, data) \
    pthread_create ((t), NULL, (entry), (data))
#define hev_sys_thread_join(t) pthread_join ((t), NULL)

#define hev_sys_once(once, func) pthread_once ((once), (func))

#define hev_sys_atfork(prepare, parent, child) \
    pthread_atfork ((prepare), (parent), (child))

#define HEV_SYS_INVALID_SOCKET ((uintptr_t)-1)

#define hev_sys_fd_to_handle(fd) ((uintptr_t)get_osfhandle (fd))
#define hev_sys_handle_to_socket(handle) (handle)

#endif /* __HEV_SYS_UNIX_PRIVATE_H__ */
