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
#include "hev-sys-msys-api.h"
#endif

#include "hev-sys-task-stack.h"
#include "hev-sys-task-execute.h"
#include "hev-sys-thread.h"
#include "hev-sys-mutex.h"
#include "hev-sys-fd.h"

#define hev_longjmp(env, val) _longjmp ((env), (val))

static inline int
hev_sys_init (void)
{
    return 0;
}

static inline void
hev_sys_fini (void)
{
}

#endif /* __HEV_SYS_UNIX_PRIVATE_H__ */
