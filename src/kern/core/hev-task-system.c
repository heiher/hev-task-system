/*
 ============================================================================
 Name        : hev-task-system.c
 Author      : Heiher <r@hev.cc>
 Copyright   : Copyright (c) 2017 - 2025 everyone.
 Description :
 ============================================================================
 */

#include <stdlib.h>
#if defined(_MSC_VER) && defined(_WIN32)
#include <windows.h>
#else
#include <pthread.h>
#endif

#include "lib/misc/hev-compiler.h"
#include "lib/misc/hev-task-stack-detector.h"
#include "mem/api/hev-memory-allocator-api.h"
#include "mem/base/hev-memory-allocator.h"
#include "mem/slice/hev-memory-allocator-slice.h"

#include "hev-task-system-private.h"

#include "hev-task-system.h"
#include "kern/aide/hev-task-aide.h"

#if defined(_MSC_VER) && defined(_WIN32)

static SRWLOCK key_lock = SRWLOCK_INIT;
static DWORD key = FLS_OUT_OF_INDEXES;

static DWORD
hev_task_system_get_key (void)
{
    DWORD result;

    AcquireSRWLockExclusive (&key_lock);
    if (key == FLS_OUT_OF_INDEXES)
        key = FlsAlloc (NULL);
    result = key;
    ReleaseSRWLockExclusive (&key_lock);

    return result;
}

HevTaskSystemContext *
hev_task_system_get_context (void)
{
    DWORD tls = hev_task_system_get_key ();

    if (tls == FLS_OUT_OF_INDEXES)
        return NULL;
    return (HevTaskSystemContext *)FlsGetValue (tls);
}

static inline int
hev_task_system_set_context (HevTaskSystemContext *context)
{
    DWORD tls = hev_task_system_get_key ();

    if (tls == FLS_OUT_OF_INDEXES)
        return -1;
    return FlsSetValue (tls, context) ? 0 : -1;
}

#else /* POSIX pthread */

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

#endif /* _WIN32 */

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

#if !defined(_MSC_VER) || !defined(_WIN32)
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

    context->timer = hev_task_timer_new (context);
    if (!context->timer)
        goto free_reactor;

#ifdef ENABLE_STACK_OVERFLOW_DETECTOR
    context->stack_detector = hev_task_stack_detector_new ();
    if (!context->stack_detector)
        goto free_timer;
#endif

    return 0;

#ifdef ENABLE_STACK_OVERFLOW_DETECTOR
free_timer:
    hev_task_timer_destroy (context->timer);
#endif
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

    if (!context)
        return;

    if (context->dns_proxy)
        hev_task_dns_proxy_destroy (context->dns_proxy);
#ifdef ENABLE_STACK_OVERFLOW_DETECTOR
    if (context->stack_detector)
        hev_task_stack_detector_destroy (context->stack_detector);
#endif
    hev_task_timer_destroy (context->timer);
    hev_task_io_reactor_destroy (context->reactor);
    hev_free (context);
    hev_task_system_set_context (NULL);

#if defined(_MSC_VER) && defined(_WIN32)
    hev_task_aide_fini ();
#endif

    allocator = hev_memory_allocator_set_default (NULL);
    if (allocator)
        hev_memory_allocator_unref (allocator);

#if defined(_MSC_VER) && defined(_WIN32)
    /* FLS indices are process-wide, not thread-local resources.  Do not
     * free them while another thread can still be using its context. */
#endif
}

EXPORT_SYMBOL void
hev_task_system_run (void)
{
    hev_task_system_schedule (HEV_TASK_RUN_SCHEDULER);
}
