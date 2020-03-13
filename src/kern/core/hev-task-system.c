/*
 ============================================================================
 Name        : hev-task-system.c
 Author      : Heiher <r@hev.cc>
 Copyright   : Copyright (c) 2017 everyone.
 Description :
 ============================================================================
 */

#include <stdlib.h>

#ifdef ENABLE_PTHREAD
#include <pthread.h>
#endif

#include "lib/utils/hev-compiler.h"
#include "mem/api/hev-memory-allocator-api.h"
#include "mem/slice/hev-memory-allocator-slice.h"

#include "hev-task-system-private.h"

#include "hev-task-system.h"

#ifdef ENABLE_PTHREAD
static pthread_key_t key;
static pthread_once_t key_once = PTHREAD_ONCE_INIT;

static void pthread_key_creator (void);
#else
static HevTaskSystemContext *default_context;
#endif

EXPORT_SYMBOL int
hev_task_system_init (void)
{
#ifdef ENABLE_MEMALLOC_SLICE
    HevMemoryAllocator *allocator;
#endif
#ifdef ENABLE_PTHREAD
    HevTaskSystemContext *default_context;
#endif

#ifdef ENABLE_MEMALLOC_SLICE
    allocator = hev_memory_allocator_slice_new ();
    if (allocator) {
        allocator = hev_memory_allocator_set_default (allocator);
        if (allocator)
            hev_memory_allocator_unref (allocator);
    }
#endif

#ifdef ENABLE_PTHREAD
    pthread_once (&key_once, pthread_key_creator);
    default_context = pthread_getspecific (key);
#endif

    if (default_context)
        return -1;

    default_context = hev_malloc0 (sizeof (HevTaskSystemContext));
    if (!default_context)
        return -2;

#ifdef ENABLE_PTHREAD
    pthread_setspecific (key, default_context);
#endif

    default_context->reactor = hev_task_io_reactor_new ();
    if (!default_context->reactor)
        return -3;

    default_context->timer = hev_task_timer_new ();
    if (!default_context->timer)
        return -4;

    return 0;
}

EXPORT_SYMBOL void
hev_task_system_fini (void)
{
#ifdef ENABLE_MEMALLOC_SLICE
    HevMemoryAllocator *allocator;
#endif
#ifdef ENABLE_PTHREAD
    HevTaskSystemContext *default_context = pthread_getspecific (key);
#endif

    hev_task_timer_destroy (default_context->timer);
    hev_task_io_reactor_destroy (default_context->reactor);
    hev_free (default_context);

#ifdef ENABLE_PTHREAD
    pthread_setspecific (key, NULL);
#else
    default_context = NULL;
#endif

#ifdef ENABLE_MEMALLOC_SLICE
    allocator = hev_memory_allocator_set_default (NULL);
    if (allocator)
        hev_memory_allocator_unref (allocator);
#endif
}

EXPORT_SYMBOL void
hev_task_system_run (void)
{
    hev_task_system_schedule (HEV_TASK_RUN_SCHEDULER);
}

HevTaskSystemContext *
hev_task_system_get_context (void)
{
#ifdef ENABLE_PTHREAD
    return pthread_getspecific (key);
#else
    return default_context;
#endif
}

#ifdef ENABLE_PTHREAD
static void
pthread_key_creator (void)
{
    pthread_key_create (&key, NULL);
}
#endif
