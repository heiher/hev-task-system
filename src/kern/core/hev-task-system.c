/*
 ============================================================================
 Name        : hev-task-system.c
 Author      : Heiher <r@hev.cc>
 Copyright   : Copyright (c) 2017 - 2025 everyone.
 Description :
 ============================================================================
 */

#include <stdlib.h>

#include "lib/misc/hev-compiler.h"
#include "lib/misc/hev-task-stack-detector.h"
#include "mem/api/hev-memory-allocator-api.h"
#include "mem/slice/hev-memory-allocator-slice.h"

#include "hev-task-system-private.h"

#include "hev-task-system.h"

__thread HevTaskSystemContext *hev_task_system_tls_context;

static inline void
hev_task_system_set_context (HevTaskSystemContext *context)
{
    hev_task_system_tls_context = context;
}

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

    if (hev_task_system_get_context ())
        goto exit;

    context = hev_malloc0 (sizeof (HevTaskSystemContext));
    if (!context)
        goto exit;

    context->reactor = hev_task_io_reactor_new ();
    if (!context->reactor)
        goto free_context;

    context->timer = hev_task_timer_new (context);
    if (!context->timer)
        goto free_reactor;

#ifdef ENABLE_STACK_OVERFLOW_DETECTOR
    context->stack_detector = hev_task_stack_detector_new ();
    if (!context->stack_detector)
        goto free_timer;
#endif

    hev_task_system_set_context (context);

    return 0;

#ifdef ENABLE_STACK_OVERFLOW_DETECTOR
free_timer:
    hev_task_timer_destroy (context->timer);
#endif
free_reactor:
    hev_task_io_reactor_destroy (context->reactor);
free_context:
    hev_free (context);
exit:
    return -1;
}

EXPORT_SYMBOL void
hev_task_system_fini (void)
{
    HevMemoryAllocator *allocator;
    HevTaskSystemContext *context;

    context = hev_task_system_get_context ();
    if (!context)
        return;

    if (context->dns_proxy)
        hev_task_dns_proxy_destroy (context->dns_proxy);
    if (context->stack_detector)
        hev_task_stack_detector_destroy (context->stack_detector);
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
