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
# undef _FORTIFY_SOURCE
#endif

#include <stdlib.h>
#include <sys/epoll.h>

#include "hev-task-system.h"
#include "hev-task-system-private.h"
#include "hev-task-private.h"
#include "hev-task-executer.h"

static inline void hev_task_system_wakeup_task_with_context (HevTaskSystemContext *ctx,
			HevTask *task);
static inline void hev_task_system_append_task (HevTaskSystemContext *ctx,
			HevTask *task);
static inline void hev_task_system_remove_current_task (HevTaskSystemContext *ctx,
			HevTaskState state);
static inline void hev_task_system_reappend_current_task (HevTaskSystemContext *ctx);
static inline void hev_task_system_pick_current_task (HevTaskSystemContext *ctx);

void
hev_task_system_schedule (HevTaskYieldType type)
{
	HevTaskSystemContext *ctx = hev_task_system_get_context ();

	if (ctx->current_task)
		goto save_task;

	if (type == HEV_TASK_RUN_SCHEDULER) {
		switch (setjmp (ctx->kernel_context)) {
		case 1:
			hev_task_system_reappend_current_task (ctx);
			break;
		case 2:
			hev_task_system_remove_current_task (ctx, HEV_TASK_STOPPED);
			break;
		case 3:
			hev_task_system_remove_current_task (ctx, HEV_TASK_WAITING);
			break;
		}
	}

	/* NOTE: in kernel context */
	/* All tasks exited, Bye! */
	if (ctx->total_task_count == 0)
		return;

	/* pick a task */
	hev_task_system_pick_current_task (ctx);

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
	hev_task_system_wakeup_task_with_context (NULL, task);
}

void
hev_task_system_run_new_task (HevTask *task)
{
	HevTaskSystemContext *ctx = hev_task_system_get_context ();

	hev_task_execute (task, hev_task_executer);

	hev_task_system_append_task (ctx, task);
	ctx->total_task_count ++;
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
hev_task_system_wakeup_task_with_context (HevTaskSystemContext *ctx, HevTask *task)
{
	/* skip to wakeup task that already in running or stopped */
	if (task->state == HEV_TASK_RUNNING || task->state == HEV_TASK_STOPPED)
		return;

	task->state = HEV_TASK_RUNNING;

	if (!ctx)
		ctx = hev_task_system_get_context ();
	hev_task_system_append_task (ctx, task);
}

static inline void
hev_task_system_append_task (HevTaskSystemContext *ctx, HevTask *task)
{
	HevTask **running_tasks;
	HevTask **running_tasks_tail;

	task->state = HEV_TASK_RUNNING;
	task->priority = task->next_priority;

	running_tasks = &ctx->running_tasks[task->priority];
	running_tasks_tail = &ctx->running_tasks_tail[task->priority];

	task->next = NULL;
	if (*running_tasks_tail) {
		task->prev = *running_tasks_tail;
		(*running_tasks_tail)->next = task;
	} else {
		task->prev = NULL;
	}
	ctx->running_tasks_bitmap |= (1U << task->priority);

	if (!*running_tasks)
		*running_tasks = task;

	*running_tasks_tail = task;
}

static inline void
hev_task_system_remove_current_task (HevTaskSystemContext *ctx, HevTaskState state)
{
	HevTask *task = ctx->current_task;
	HevTask **running_tasks;
	HevTask **running_tasks_tail;

	task->state = state;

	running_tasks = &ctx->running_tasks[task->priority];
	running_tasks_tail = &ctx->running_tasks_tail[task->priority];

	*running_tasks = task->next;
	if (*running_tasks) {
		(*running_tasks)->prev = NULL;
	} else {
		*running_tasks_tail = NULL;
		ctx->running_tasks_bitmap ^= (1U << task->priority);
	}

	ctx->current_task = NULL;

	if (HEV_TASK_STOPPED == state) {
		ctx->total_task_count --;
		hev_task_unref (task);
	}
}

static inline void
hev_task_system_reappend_current_task (HevTaskSystemContext *ctx)
{
	HevTask *task = ctx->current_task;
	HevTask **running_tasks;
	HevTask **running_tasks_tail;

	ctx->current_task = NULL;

	if (task->priority == task->next_priority) {
		if (!task->next)
			return;

		running_tasks = &ctx->running_tasks[task->priority];
		running_tasks_tail = &ctx->running_tasks_tail[task->priority];

		*running_tasks = task->next;

		task->next = NULL;
		task->prev = *running_tasks_tail;

		(*running_tasks)->prev = NULL;
		(*running_tasks_tail)->next = task;
		*running_tasks_tail = task;
		return;
	}

	/* remove */
	running_tasks = &ctx->running_tasks[task->priority];
	running_tasks_tail = &ctx->running_tasks_tail[task->priority];

	*running_tasks = task->next;
	if (*running_tasks) {
		(*running_tasks)->prev = NULL;
	} else {
		*running_tasks_tail = NULL;
		ctx->running_tasks_bitmap ^= (1U << task->priority);
	}

	/* append */
	task->priority = task->next_priority;
	running_tasks = &ctx->running_tasks[task->priority];
	running_tasks_tail = &ctx->running_tasks_tail[task->priority];

	task->next = NULL;
	if (*running_tasks_tail) {
		task->prev = *running_tasks_tail;
		(*running_tasks_tail)->next = task;
	} else {
		task->prev = NULL;
	}
	ctx->running_tasks_bitmap |= (1U << task->priority);

	if (!*running_tasks)
		*running_tasks = task;

	*running_tasks_tail = task;
}

static inline void
hev_task_system_pick_current_task (HevTaskSystemContext *ctx)
{
	int i, count, timeout = 0;
	struct epoll_event events[128];

retry:
	/* io poll */
	count = epoll_wait (ctx->epoll_fd, events, 128, timeout);
	for (i=0; i<count; i++) {
		HevTaskSchedEntity *sched_entity;

		sched_entity = events[i].data.ptr;
		hev_task_system_wakeup_task_with_context (ctx, sched_entity->task);
	}

	/* no task ready, retry */
	if (!ctx->running_tasks_bitmap) {
		timeout = -1;
		goto retry;
	}

	for (i=HEV_TASK_PRIORITY_MIN; i<=HEV_TASK_PRIORITY_MAX; i++) {
		if (ctx->running_tasks[i]) {
			ctx->current_task = ctx->running_tasks[i];
			return;
		}
	}
}

