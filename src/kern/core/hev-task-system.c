/*
 ============================================================================
 Name        : hev-task-system.c
 Author      : Heiher <r@hev.cc>
 Copyright   : Copyright (c) 2017 - 2020 everyone.
 Description :
 ============================================================================
 */

#include <stdlib.h>

#include "lib/misc/hev-compiler.h"
#include "mem/api/hev-memory-allocator-api.h"
#include "mem/slice/hev-memory-allocator-slice.h"

#include "hev-task-system-private.h"

#include "hev-task-system.h"

#ifdef ENABLE_PTHREAD

#include <pthread.h>

static pthread_key_t key;
static pthread_once_t key_once = PTHREAD_ONCE_INIT;

static void
pthread_key_creator (void)
{
    pthread_key_create (&key, NULL);
}

HevTaskSystemContext *
hev_task_system_get_context (void)
{
    return pthread_getspecific (key);
}

static inline int
hev_task_system_set_context (HevTaskSystemContext *context)
{
    return pthread_setspecific (key, context);
}

#else /* !ENABLE_PTHREAD */

static HevTaskSystemContext *default_context;

HevTaskSystemContext *
hev_task_system_get_context (void)
{
    return default_context;
}

static inline int
hev_task_system_set_context (HevTaskSystemContext *context)
{
    default_context = context;
    return 0;
}

#endif /* !ENABLE_PTHREAD */

EXPORT_SYMBOL int
hev_task_system_init (void)
{
    HevMemoryAllocator *allocator = NULL;
    HevTaskSystemContext *context;

#ifdef ENABLE_MEMALLOC_SLICE
    allocator = hev_memory_allocator_slice_new ();
#endif
    allocator = hev_memory_allocator_set_default (allocator);
    if (allocator)
        hev_memory_allocator_unref (allocator);

#ifdef ENABLE_PTHREAD
    pthread_once (&key_once, pthread_key_creator);
#endif

    if (hev_task_system_get_context ())
        goto exit;

    context = hev_malloc0 (sizeof (HevTaskSystemContext));
    if (!context)
        goto exit;

    if (hev_task_system_set_context (context) < 0)
        goto free_context;

    context->reactor = hev_task_io_reactor_new ();
    if (!context->reactor)
        goto rest_context;

    context->timer = hev_task_timer_new ();
    if (!context->timer)
        goto free_reactor;

    return 0;

free_reactor:
    hev_task_io_reactor_destroy (context->reactor);
rest_context:
    hev_task_system_set_context (NULL);
free_context:
    hev_free (context);
exit:
    return -1;
}

EXPORT_SYMBOL void
hev_task_system_fini (void)
{
    HevMemoryAllocator *allocator;
    HevTaskSystemContext *context = hev_task_system_get_context ();

    if (context->dns_proxy)
        hev_task_dns_proxy_destroy (context->dns_proxy);
    hev_task_timer_destroy (context->timer);
    hev_task_io_reactor_destroy (context->reactor);
    hev_free (context);
    hev_task_system_set_context (NULL);

    allocator = hev_memory_allocator_set_default (NULL);
    if (allocator)
        hev_memory_allocator_unref (allocator);
}

EXPORT_SYMBOL void
hev_task_system_run (void)
{
    hev_task_system_schedule (HEV_TASK_RUN_SCHEDULER);
}
