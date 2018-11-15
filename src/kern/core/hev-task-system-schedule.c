/*
 ============================================================================
 Name        : hev-task-system-schedule.c
 Author      : Heiher <r@hev.cc>
 Copyright   : Copyright (c) 2017 everyone.
 Description :
 ============================================================================
 */

/* Disable signal stack check in longjmp */
#ifdef _FORTIFY_SOURCE
#undef _FORTIFY_SOURCE
#endif

#include <stdlib.h>
#include <sys/epoll.h>

#include "hev-task-system.h"
#include "hev-task-system-private.h"
#include "kern/task/hev-task-private.h"
#include "kern/task/hev-task-executer.h"

static inline void
hev_task_system_update_sched_time (HevTaskSystemContext *ctx);
static inline void hev_task_system_update_sched_key (HevTaskSystemContext *ctx);
static inline void
hev_task_system_wakeup_task_with_context (HevTaskSystemContext *ctx,
                                          HevTask *task);
static inline void hev_task_system_insert_task (HevTaskSystemContext *ctx,
                                                HevTask *task);
static inline void
hev_task_system_remove_current_task (HevTaskSystemContext *ctx,
                                     HevTaskState state);
static inline void
hev_task_system_reinsert_current_task (HevTaskSystemContext *ctx);
static inline void
hev_task_system_pick_current_task (HevTaskSystemContext *ctx);

void
hev_task_system_schedule (HevTaskYieldType type)
{
    HevTaskSystemContext *ctx = hev_task_system_get_context ();

    if (ctx->current_task)
        goto save_task;

    if (type == HEV_TASK_RUN_SCHEDULER) {
        switch (setjmp (ctx->kernel_context)) {
        case 1:
            hev_task_system_update_sched_key (ctx);
            hev_task_system_reinsert_current_task (ctx);
            break;
        case 2:
            hev_task_system_remove_current_task (ctx, HEV_TASK_STOPPED);
            break;
        case 3:
            hev_task_system_update_sched_key (ctx);
            hev_task_system_remove_current_task (ctx, HEV_TASK_WAITING);
            break;
        }
    }

    /* NOTE: in kernel context */
    /* All tasks exited, Bye! */
    if (ctx->total_task_count == 0) {
        ctx->current_task = NULL;
        return;
    }

    /* pick a task */
    hev_task_system_pick_current_task (ctx);

    /* update schedule time */
    hev_task_system_update_sched_time (ctx);

    /* switch to task */
    longjmp (ctx->current_task->context, 1);

save_task:
    /* NOTE: in task context */
    /* save current task context */
    if (setjmp (ctx->current_task->context))
        return; /* resume to task context */

    if (type == HEV_TASK_WAITIO)
        longjmp (ctx->kernel_context, 3);

    /* resume to kernel context */
    longjmp (ctx->kernel_context, 1);
}

void
hev_task_system_wakeup_task (HevTask *task)
{
    HevTaskSystemContext *ctx;

    ctx = hev_task_system_get_context ();
    hev_task_system_wakeup_task_with_context (ctx, task);
}

void
hev_task_system_run_new_task (HevTask *task)
{
    HevTaskSystemContext *ctx = hev_task_system_get_context ();

    hev_task_execute (task, hev_task_executer);

    /* Copy current task's schedule key */
    if (ctx->current_task)
        task->sched_key += ctx->current_task->sched_key;

    hev_task_system_insert_task (ctx, task);
    ctx->total_task_count++;
}

void
hev_task_system_kill_current_task (void)
{
    HevTaskSystemContext *ctx = hev_task_system_get_context ();

    /* NOTE: remove current task in kernel context, because current
     * task stack may be freed. */
    longjmp (ctx->kernel_context, 2);
}

static inline void
hev_task_system_get_clock_time (struct timespec *ts)
{
    if (-1 == clock_gettime (CONFIG_SCHED_CLOCK, ts))
        abort ();
}

static inline void
hev_task_system_update_sched_time (HevTaskSystemContext *ctx)
{
    hev_task_system_get_clock_time (&ctx->sched_time);
}

static inline void
hev_task_system_update_sched_key (HevTaskSystemContext *ctx)
{
    HevTask *curr_task = ctx->current_task;
    struct timespec curr_time;
    uint64_t runtime;
    time_t sec;
    long nsec;

    hev_task_system_get_clock_time (&curr_time);

    sec = curr_time.tv_sec - ctx->sched_time.tv_sec;
    nsec = curr_time.tv_nsec - ctx->sched_time.tv_nsec;
    if (nsec < 0) {
        sec--;
        nsec += 1000000000L;
    }

    runtime = (uint64_t)sec * 1000000000UL + nsec;
    curr_task->sched_key += runtime * curr_task->priority;
}

static inline void
hev_task_system_wakeup_task_with_context (HevTaskSystemContext *ctx,
                                          HevTask *task)
{
    /* skip to wakeup task that already in running or stopped */
    if (task->state == HEV_TASK_RUNNING || task->state == HEV_TASK_STOPPED)
        return;

    hev_task_system_insert_task (ctx, task);
}

static inline void
_hev_task_system_insert_task (HevRBTreeCached *tree, HevTask *task)
{
    HevRBTreeNode **new = &tree->base.root, *parent = NULL;
    int leftmost = 1;

    while (*new) {
        HevTask *this = container_of (*new, HevTask, node);

        parent = *new;
        if (task->sched_key < this->sched_key) {
            new = &((*new)->left);
        } else if (task->sched_key > this->sched_key) {
            new = &((*new)->right);
            leftmost = 0;
        } else {
            if (task < this) {
                new = &((*new)->left);
            } else {
                new = &((*new)->right);
                leftmost = 0;
            }
        }
    }

    hev_rbtree_node_link (&task->node, parent, new);
    hev_rbtree_cached_insert_color (tree, &task->node, leftmost);
}

static inline void
hev_task_system_insert_task (HevTaskSystemContext *ctx, HevTask *task)
{
    task->state = HEV_TASK_RUNNING;
    task->priority = task->next_priority;

    _hev_task_system_insert_task (&ctx->running_tasks, task);

    ctx->running_task_count++;
}

static inline void
hev_task_system_remove_current_task (HevTaskSystemContext *ctx,
                                     HevTaskState state)
{
    HevTask *task = ctx->current_task;

    task->state = state;

    hev_rbtree_cached_erase (&ctx->running_tasks, &task->node);

    ctx->running_task_count--;

    if (HEV_TASK_STOPPED == state) {
        ctx->total_task_count--;
        hev_task_unref (task);
    }
}

static inline void
hev_task_system_reinsert_current_task (HevTaskSystemContext *ctx)
{
    HevTask *task = ctx->current_task;

    task->priority = task->next_priority;

    hev_rbtree_cached_erase (&ctx->running_tasks, &task->node);
    _hev_task_system_insert_task (&ctx->running_tasks, task);
}

static inline void
hev_task_system_io_poll (HevTaskSystemContext *ctx, int timeout)
{
    int i, count;
    struct epoll_event events[128];

    count = epoll_wait (ctx->epoll_fd, events, 128, timeout);
    for (i = 0; i < count; i++) {
        HevTaskSchedEntity *sched_entity;

        sched_entity = events[i].data.ptr;
        hev_task_system_wakeup_task_with_context (ctx, sched_entity->task);
    }
}

static inline void
hev_task_system_pick_current_task (HevTaskSystemContext *ctx)
{
    HevRBTreeNode *node;

    if (ctx->running_task_count < ctx->total_task_count) {
        if (ctx->running_task_count) {
            hev_task_system_io_poll (ctx, 0);
        } else {
            do {
                hev_task_system_io_poll (ctx, -1);
            } while (!ctx->running_task_count);
        }
    }

    node = hev_rbtree_cached_first (&ctx->running_tasks);
    ctx->current_task = container_of (node, HevTask, node);
}
