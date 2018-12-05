/*
 ============================================================================
 Name        : hev-task-io-shared.h
 Author      : Heiher <r@hev.cc>
 Copyright   : Copyright (c) 2018 everyone.
 Description : Task I/O operations
 ============================================================================
 */

#ifndef __HEV_TASK_IO_SHARED_H__
#define __HEV_TASK_IO_SHARED_H__

#include <stdlib.h>

#include "kern/io/hev-task-io-reactor.h"
#include "kern/core/hev-task-system-private.h"

static inline void
hev_task_io_res_fd (int fd, HevTaskIOReactorEvents events)
{
    HevTaskSystemContext *ctx;
    HevTaskIOReactorSetupEvent event;

    if (!hev_task_io_reactor_get_oneshot ())
        return;

    ctx = hev_task_system_get_context ();
    hev_task_io_reactor_setup_event_set (&event, fd, HEV_TASK_IO_REACTOR_OP_ADD,
                                         events,
                                         &ctx->current_task->sched_entity);
    if (hev_task_io_reactor_setup (ctx->reactor, &event, 1) == -1)
        abort ();
}

static inline void
hev_task_io_res_fd2 (int fd1, HevTaskIOReactorEvents events1, int fd2,
                     HevTaskIOReactorEvents events2)
{
    HevTaskSystemContext *ctx;
    HevTaskIOReactorSetupEvent events[2];

    if (!hev_task_io_reactor_get_oneshot ())
        return;

    ctx = hev_task_system_get_context ();
    hev_task_io_reactor_setup_event_set (&events[0], fd1,
                                         HEV_TASK_IO_REACTOR_OP_ADD, events1,
                                         &ctx->current_task->sched_entity);
    hev_task_io_reactor_setup_event_set (&events[1], fd2,
                                         HEV_TASK_IO_REACTOR_OP_ADD, events2,
                                         &ctx->current_task->sched_entity);
    if (hev_task_io_reactor_setup (ctx->reactor, events, 2) == -1)
        abort ();
}

static inline void
hev_task_io_res_fd3 (int fd1, HevTaskIOReactorEvents events1, int fd2,
                     HevTaskIOReactorEvents events2, int fd3,
                     HevTaskIOReactorEvents events3)
{
    HevTaskSystemContext *ctx;
    HevTaskIOReactorSetupEvent events[3];

    if (!hev_task_io_reactor_get_oneshot ())
        return;

    ctx = hev_task_system_get_context ();
    hev_task_io_reactor_setup_event_set (&events[0], fd1,
                                         HEV_TASK_IO_REACTOR_OP_ADD, events1,
                                         &ctx->current_task->sched_entity);
    hev_task_io_reactor_setup_event_set (&events[1], fd2,
                                         HEV_TASK_IO_REACTOR_OP_ADD, events2,
                                         &ctx->current_task->sched_entity);
    hev_task_io_reactor_setup_event_set (&events[2], fd3,
                                         HEV_TASK_IO_REACTOR_OP_ADD, events3,
                                         &ctx->current_task->sched_entity);
    if (hev_task_io_reactor_setup (ctx->reactor, events, 3) == -1)
        abort ();
}

static inline void
hev_task_io_res_fd4 (int fd1, HevTaskIOReactorEvents events1, int fd2,
                     HevTaskIOReactorEvents events2, int fd3,
                     HevTaskIOReactorEvents events3, int fd4,
                     HevTaskIOReactorEvents events4)
{
    HevTaskSystemContext *ctx;
    HevTaskIOReactorSetupEvent events[4];

    if (!hev_task_io_reactor_get_oneshot ())
        return;

    ctx = hev_task_system_get_context ();
    hev_task_io_reactor_setup_event_set (&events[0], fd1,
                                         HEV_TASK_IO_REACTOR_OP_ADD, events1,
                                         &ctx->current_task->sched_entity);
    hev_task_io_reactor_setup_event_set (&events[1], fd2,
                                         HEV_TASK_IO_REACTOR_OP_ADD, events2,
                                         &ctx->current_task->sched_entity);
    hev_task_io_reactor_setup_event_set (&events[2], fd3,
                                         HEV_TASK_IO_REACTOR_OP_ADD, events3,
                                         &ctx->current_task->sched_entity);
    hev_task_io_reactor_setup_event_set (&events[3], fd4,
                                         HEV_TASK_IO_REACTOR_OP_ADD, events4,
                                         &ctx->current_task->sched_entity);
    if (hev_task_io_reactor_setup (ctx->reactor, events, 4) == -1)
        abort ();
}

#endif /* __HEV_TASK_IO_SHARED_H__ */
