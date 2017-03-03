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

void
hev_task_system_schedule (HevTaskYieldType type, HevTask *_new_task)
{
	HevTaskSystemContext *ctx = hev_task_system_get_context ();
	int i, count, timeout, priority;
	struct epoll_event events[128];

	ctx->new_task = _new_task;

	if (ctx->current_task) {
		/* save current task context */
		if (setjmp (ctx->current_task->context))
			return; /* resume to task context */

		/* insert current task to waiting list by type */
		priority = hev_task_get_priority (ctx->current_task);
		ctx->current_task->prev = NULL;
		ctx->current_task->next = ctx->waiting_lists[priority][type];
		if (ctx->waiting_lists[priority][type])
			ctx->waiting_lists[priority][type]->prev = ctx->current_task;
		ctx->waiting_lists[priority][type] = ctx->current_task;

		/* set task state = WAITING */
		ctx->current_task->state = HEV_TASK_WAITING;

		/* resume to kernel context */
		longjmp (ctx->kernel_context, 1);
	}

	if (type == HEV_TASK_YIELD_COUNT)
		if (setjmp (ctx->kernel_context))
			ctx = hev_task_system_get_context ();

	/* NOTE: in kernel context */
	if (ctx->new_task) {
		priority = hev_task_get_priority (ctx->new_task);

		/* insert new task to running list */
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

			/* set task state = STOPPED */
			ctx->current_task->state = HEV_TASK_STOPPED;

			hev_task_unref (ctx->current_task);
			ctx->task_count --;

			longjmp (ctx->kernel_context, 1);
		}

		ctx->task_count ++;
		ctx->new_task = NULL;

		/* for task_run before task_system_run */
		if (!ctx->current_task)
			return;
	}

	/* All tasks exited, Bye! */
	if (!ctx->task_count)
		return;

	/* schedule */
	timeout = 0;
retry:
	/* io poll */
	count = epoll_wait (ctx->epoll_fd, events, 128, timeout);
	for (i=0; i<count; i++) {
		HevTask *task = events[i].data.ptr;

		hev_task_system_wakeup_task (task);
	}

	/* get a ready task */
	ctx->current_task = NULL;
	for (i=HEV_TASK_PRIORITY_MIN; i<=HEV_TASK_PRIORITY_MAX; i++) {
		if (!ctx->running_lists[i])
			continue;

		ctx->current_task = ctx->running_lists[i];
		break;
	}

	/* no task ready */
	if (!ctx->current_task) {
		/* move tasks from yield watting list to running list */
		for (i=HEV_TASK_PRIORITY_MIN; i<=HEV_TASK_PRIORITY_MAX; i++) {
			HevTask *task;

			ctx->running_lists[i] = ctx->waiting_lists[i][HEV_TASK_YIELD];
			ctx->waiting_lists[i][HEV_TASK_YIELD] = NULL;

			/* set task state = RUNNING */
			for (task=ctx->running_lists[i]; task; task=task->next)
				task->state = HEV_TASK_RUNNING;
		}

		/* get a ready task again */
		for (i=HEV_TASK_PRIORITY_MIN; i<=HEV_TASK_PRIORITY_MAX; i++) {
			if (!ctx->running_lists[i])
				continue;

			ctx->current_task = ctx->running_lists[i];
			break;
		}
	}

	/* no task ready again, retry */
	if (!ctx->current_task) {
		timeout = -1;
		goto retry;
	}

	/* remove task from running list */
	if (ctx->current_task->prev) {
		ctx->current_task->prev->next = ctx->current_task->next;
	} else {
		priority = hev_task_get_priority (ctx->current_task);
		ctx->running_lists[priority] = ctx->current_task->next;
	}
	if (ctx->current_task->next) {
		ctx->current_task->next->prev = ctx->current_task->prev;
	}

	/* apply task's next_priority */
	ctx->current_task->priority = ctx->current_task->next_priority;

	/* switch to task */
	longjmp (ctx->current_task->context, 1);
}

void
hev_task_system_wakeup_task (HevTask *task)
{
	HevTaskSystemContext *ctx = hev_task_system_get_context ();
	int priority;

	/* skip to wakeup task that already in running */
	if (task->state == HEV_TASK_RUNNING)
		return;

	priority = hev_task_get_priority (task);
	/* remove task from waiting list */
	if (task->prev) {
		task->prev->next = task->next;
	} else {
		int j;

		for (j=HEV_TASK_YIELD; j<HEV_TASK_YIELD_COUNT; j++)
			if (ctx->waiting_lists[priority][j] == task) {
				ctx->waiting_lists[priority][j] = task->next;
				break;
			}
	}
	if (task->next) {
		task->next->prev = task->prev;
	}

	/* insert task to running list */
	task->prev = NULL;
	task->next = ctx->running_lists[priority];
	if (ctx->running_lists[priority])
		ctx->running_lists[priority]->prev = task;
	ctx->running_lists[priority] = task;

	/* set task state = RUNNING */
	task->state = HEV_TASK_RUNNING;
}

