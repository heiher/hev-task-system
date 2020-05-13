/*
 ============================================================================
 Name        : hev-task.c
 Author      : Heiher <r@hev.cc>
 Copyright   : Copyright (c) 2017 everyone.
 Description :
 ============================================================================
 */

#include <assert.h>
#include <stdlib.h>
#include <stdint.h>

#include "lib/misc/hev-compiler.h"
#include "kern/core/hev-task-system-private.h"
#include "mem/api/hev-memory-allocator-api.h"

#include "hev-task-private.h"

#include "hev-task.h"

#define HEV_TASK_STACK_SIZE (64 * 1024)

#ifdef ENABLE_STACK_OVERFLOW_DETECTION
#if CONFIG_STACK_OVERFLOW_DETECTION == 1
#define STACK_OVERFLOW_DETECTION_SIZE (4)
#elif CONFIG_STACK_OVERFLOW_DETECTION == 2
#define STACK_OVERFLOW_DETECTION_SIZE (4096)
#else
#define STACK_OVERFLOW_DETECTION_SIZE (8192)
#endif
#define STACK_OVERFLOW_DETECTION_TAG (0xdeadbeefu)
#endif

static inline void
hev_task_stack_detection_mark (HevTask *self)
{
#ifdef ENABLE_STACK_OVERFLOW_DETECTION
    const unsigned int tag = STACK_OVERFLOW_DETECTION_TAG;
    int i;

    for (i = 0; i < STACK_OVERFLOW_DETECTION_SIZE; i += sizeof (tag))
        *(unsigned int *)(self->stack + i) = tag;
#endif
}

static inline void
hev_task_stack_detection_check (HevTask *self)
{
#ifdef ENABLE_STACK_OVERFLOW_DETECTION
    const unsigned int tag = STACK_OVERFLOW_DETECTION_TAG;
    int i;

    for (i = 0; i < STACK_OVERFLOW_DETECTION_SIZE; i += sizeof (tag))
        assert (*(unsigned int *)(self->stack + i) == tag);
#endif
}

EXPORT_SYMBOL HevTask *
hev_task_new (int stack_size)
{
    HevTask *self;
    uintptr_t stack_addr;

    self = hev_malloc0 (sizeof (HevTask));
    if (!self)
        return NULL;

    self->ref_count = 1;
    self->next_priority = HEV_TASK_PRIORITY_DEFAULT;

    if (stack_size < 0)
        stack_size = HEV_TASK_STACK_SIZE;
    stack_size += STACK_OVERFLOW_DETECTION_SIZE;

    self->stack = hev_malloc (stack_size);
    if (!self->stack) {
        hev_free (self);
        return NULL;
    }

    hev_task_stack_detection_mark (self);

    stack_addr = (uintptr_t) (self->stack + stack_size);
    self->stack_top = (void *)ALIGN_DOWN (stack_addr, 16);
    self->sched_entity.task = self;

    return self;
}

EXPORT_SYMBOL HevTask *
hev_task_ref (HevTask *self)
{
    self->ref_count++;

    return self;
}

EXPORT_SYMBOL void
hev_task_unref (HevTask *self)
{
    self->ref_count--;
    if (self->ref_count)
        return;

    hev_task_stack_detection_check (self);

    hev_free (self->stack);
    hev_free (self);
}

EXPORT_SYMBOL HevTask *
hev_task_self (void)
{
    return hev_task_system_get_context ()->current_task;
}

EXPORT_SYMBOL HevTaskState
hev_task_get_state (HevTask *self)
{
    return self->state;
}

EXPORT_SYMBOL void
hev_task_set_priority (HevTask *self, int priority)
{
    if (priority < HEV_TASK_PRIORITY_MIN)
        priority = HEV_TASK_PRIORITY_MIN;
    else if (priority > HEV_TASK_PRIORITY_MAX)
        priority = HEV_TASK_PRIORITY_MAX;

    self->next_priority = priority;
}

EXPORT_SYMBOL int
hev_task_get_priority (HevTask *self)
{
    return self->next_priority;
}

EXPORT_SYMBOL int
hev_task_add_fd (HevTask *self, int fd, unsigned int events)
{
    HevTaskIOReactor *reactor;
    HevTaskIOReactorSetupEvent revents[HEV_TASK_IO_REACTOR_EVENT_GEN_MAX];
    int count;

    reactor = hev_task_system_get_context ()->reactor;
    count = hev_task_io_reactor_setup_event_gen (
        revents, fd, HEV_TASK_IO_REACTOR_OP_ADD, events, &self->sched_entity);
    return hev_task_io_reactor_setup (reactor, revents, count);
}

EXPORT_SYMBOL int
hev_task_mod_fd (HevTask *self, int fd, unsigned int events)
{
    HevTaskIOReactor *reactor;
    HevTaskIOReactorSetupEvent revents[HEV_TASK_IO_REACTOR_EVENT_GEN_MAX];
    int count;

    reactor = hev_task_system_get_context ()->reactor;
    count = hev_task_io_reactor_setup_event_gen (
        revents, fd, HEV_TASK_IO_REACTOR_OP_MOD, events, &self->sched_entity);
    return hev_task_io_reactor_setup (reactor, revents, count);
}

EXPORT_SYMBOL int
hev_task_del_fd (HevTask *self, int fd)
{
    HevTaskIOReactor *reactor;
    HevTaskIOReactorSetupEvent revents[HEV_TASK_IO_REACTOR_EVENT_GEN_MAX];
    int count;

    reactor = hev_task_system_get_context ()->reactor;
    count = hev_task_io_reactor_setup_event_gen (
        revents, fd, HEV_TASK_IO_REACTOR_OP_DEL, 0, NULL);
    return hev_task_io_reactor_setup (reactor, revents, count);
}

EXPORT_SYMBOL void
hev_task_wakeup (HevTask *task)
{
    hev_task_system_wakeup_task (task);
}

EXPORT_SYMBOL void
hev_task_yield (HevTaskYieldType type)
{
    hev_task_system_schedule (type);
}

EXPORT_SYMBOL unsigned int
hev_task_sleep (unsigned int milliseconds)
{
    return hev_task_usleep (milliseconds * 1000) / 1000;
}

EXPORT_SYMBOL unsigned int
hev_task_usleep (unsigned int microseconds)
{
    HevTaskSystemContext *ctx;

    if (microseconds == 0)
        return 0;

    ctx = hev_task_system_get_context ();
    return hev_task_timer_wait (ctx->timer, microseconds, ctx->current_task);
}

EXPORT_SYMBOL void
hev_task_run (HevTask *self, HevTaskEntry entry, void *data)
{
    /* Skip to run task that already running */
    if (self->state != HEV_TASK_STOPPED)
        return;

    self->entry = entry;
    self->data = data;
    self->priority = self->next_priority;
    self->sched_key = self->next_priority;

    hev_task_system_run_new_task (self);
}

EXPORT_SYMBOL void
hev_task_exit (void)
{
    hev_task_system_kill_current_task ();
}

EXPORT_SYMBOL void *
hev_task_get_data (HevTask *self)
{
    return self->data;
}
