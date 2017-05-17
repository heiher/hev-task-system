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

static inline HevTask * hev_task_system_pick (HevTaskSystemContext *ctx);

void
hev_task_system_schedule (HevTaskYieldType type, HevTask *_new_task)
{
	HevTaskSystemContext *ctx = hev_task_system_get_context ();
	int priority;

	ctx->new_task = _new_task;

	if (ctx->current_task)
		goto save_task;

	if (type == HEV_TASK_YIELD_COUNT)
		if (setjmp (ctx->kernel_context))
			ctx = hev_task_system_get_context ();

	/* NOTE: in kernel context */
	if (ctx->new_task)
		goto new_task;

schedule:
	/* All tasks exited, Bye! */
	if (!ctx->task_count)
		return;

	/* pick a task */
	ctx->current_task = hev_task_system_pick (ctx);

	/* remove task from running lists head */
	priority = ctx->current_task->priority;
	ctx->running_lists[priority] = ctx->current_task->next;
	if (ctx->current_task->next)
		ctx->current_task->next->prev = NULL;

	/* apply task's next_priority */
	ctx->current_task->priority = ctx->current_task->next_priority;

	/* switch to task */
	longjmp (ctx->current_task->context, 1);

save_task:
	/* NOTE: in task context */
	/* save current task context */
	if (setjmp (ctx->current_task->context))
		return; /* resume to task context */

	/* insert current task to waiting lists by type */
	ctx->current_task->prev = NULL;
	ctx->current_task->next = ctx->waiting_lists[type];
	if (ctx->waiting_lists[type])
		ctx->waiting_lists[type]->prev = ctx->current_task;
	ctx->waiting_lists[type] = ctx->current_task;

	/* set task state = WAITING */
	ctx->current_task->state = HEV_TASK_WAITING;

	/* resume to kernel context */
	longjmp (ctx->kernel_context, 1);

new_task:
	/* NOTE: in kernel context */
	priority = ctx->new_task->priority;

	/* insert new task to running lists */
	ctx->new_task->prev = NULL;
	ctx->new_task->next = ctx->running_lists[priority];
	if (ctx->running_lists[priority])
		ctx->running_lists[priority]->prev = ctx->new_task;
	ctx->running_lists[priority] = ctx->new_task;

	/* set task state = RUNNING */
	ctx->new_task->state = HEV_TASK_RUNNING;

	/* save context in new task */
	if (setjmp (ctx->new_task->context)) {
		/* NOTE: the new task will run at next schedule */
		ctx = hev_task_system_get_context ();

		/* execute new task entry */
		hev_task_execute (ctx->current_task);

		ctx = hev_task_system_get_context ();

		/* set task state = STOPPED */
		ctx->current_task->state = HEV_TASK_STOPPED;

		hev_task_unref (ctx->current_task);
		ctx->task_count --;

		longjmp (ctx->kernel_context, 1);
	}

	ctx->task_count ++;
	ctx->new_task = NULL;

	if (ctx->current_task)
		goto schedule;
}

void
hev_task_system_wakeup_task (HevTask *task)
{
	HevTaskSystemContext *ctx = hev_task_system_get_context ();
	int priority;

	/* skip to wakeup task that already in running */
	if (task->state == HEV_TASK_RUNNING)
		return;

	/* remove task from waiting lists */
	if (task->prev) {
		task->prev->next = task->next;
	} else {
		int i;

		for (i=HEV_TASK_YIELD; i<HEV_TASK_YIELD_COUNT; i++)
			if (ctx->waiting_lists[i] == task) {
				ctx->waiting_lists[i] = task->next;
				break;
			}
	}
	if (task->next)
		task->next->prev = task->prev;

	priority = task->priority;
	/* insert task to running lists */
	task->prev = NULL;
	task->next = ctx->running_lists[priority];
	if (ctx->running_lists[priority])
		ctx->running_lists[priority]->prev = task;
	ctx->running_lists[priority] = task;

	/* set task state = RUNNING */
	task->state = HEV_TASK_RUNNING;
}

static inline HevTask *
hev_task_system_running_lists_head (HevTask *running_lists[])
{
	int i;

	for (i=HEV_TASK_PRIORITY_MIN; i<=HEV_TASK_PRIORITY_MAX; i++) {
		if (!running_lists[i])
			continue;

		return running_lists[i];
	}

	return NULL;
}

static inline HevTask *
hev_task_system_pick (HevTaskSystemContext *ctx)
{
	HevTask *task = NULL;
	int i, count, timeout = 0;
	struct epoll_event events[128];

retry:
	/* io poll */
	count = epoll_wait (ctx->epoll_fd, events, 128, timeout);
	for (i=0; i<count; i++) {
		HevTask *task = events[i].data.ptr;

		if (task->state == HEV_TASK_STOPPED)
			continue;

		hev_task_system_wakeup_task (task);
	}

	/* get a task from running lists */
	task = hev_task_system_running_lists_head (ctx->running_lists);

	/* got a task */
	if (task)
		return task;

	/* move tasks from yield waiting lists to running lists */
	for (task=ctx->waiting_lists[HEV_TASK_YIELD]; task;) {
		HevTask *curr_task = task;
		int priority = curr_task->priority;

		task = task->next;
		curr_task->state = HEV_TASK_RUNNING;

		curr_task->prev = NULL;
		curr_task->next = ctx->running_lists[priority];
		ctx->running_lists[priority] = curr_task;
	}
	ctx->waiting_lists[HEV_TASK_YIELD] = NULL;

	/* get a task from running lists again */
	task = hev_task_system_running_lists_head (ctx->running_lists);

	/* no task ready again, retry */
	if (!task) {
		timeout = -1;
		goto retry;
	}

	return task;
}

