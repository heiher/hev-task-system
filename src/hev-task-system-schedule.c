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

static inline void hev_task_system_insert_task (HevTaskSystemContext *ctx);
static inline void hev_task_system_remove_task (HevTaskSystemContext *ctx);
static inline void hev_task_system_update_task (HevTaskSystemContext *ctx);
static inline HevTask * hev_task_system_pick (HevTaskSystemContext *ctx);

/*
 * ring task list:
 *
 *   .--> task_nodes[0] --> task_nodes[1] ----.
 *   |         ^                              |
 *   |         |                              v
 * task[N]     |                 .--------> task[2]
 *   ^         \-- current_task  |            |
 *   |                           |            v
 * task[...]                   picked       task[3]
 *   ^                                        |
 *   |                                        |
 *   \--- task_nodes[5] <-- task_nodes[4] <---/
 */

void
hev_task_system_schedule (HevTaskYieldType type, HevTask *_new_task)
{
	HevTaskSystemContext *ctx = hev_task_system_get_context ();

	ctx->new_task = _new_task;

	if (ctx->current_task)
		goto save_task;

	if (type == HEV_TASK_YIELD_COUNT)
		if (setjmp (ctx->kernel_context) == 2)
			hev_task_system_remove_task (ctx);

	/* NOTE: in kernel context */
	if (ctx->new_task)
		goto new_task;

schedule:
	/* All tasks exited, Bye! */
	if (ctx->total_task_count == 0)
		return;

	/* pick a task */
	ctx->current_task = hev_task_system_pick (ctx);

	/* apply task's next priority */
	if (ctx->current_task->priority != ctx->current_task->next_priority)
		hev_task_system_update_task (ctx);

	/* switch to task */
	longjmp (ctx->current_task->context, 1);

save_task:
	/* NOTE: in task context */
	/* save current task context */
	if (setjmp (ctx->current_task->context))
		return; /* resume to task context */

	if (type == HEV_TASK_WAITIO) {
		ctx->running_task_count --;
		ctx->current_task->state = HEV_TASK_WAITING;
	}

	/* resume to kernel context */
	longjmp (ctx->kernel_context, 1);

new_task:
	/* NOTE: in kernel context */
	hev_task_execute (ctx->new_task, ctx->kernel_context, hev_task_executer);
	hev_task_system_insert_task (ctx);

	if (ctx->current_task)
		goto schedule;
}

void
hev_task_system_wakeup_task (HevTask *task)
{
	HevTaskSystemContext *ctx;

	/* skip to wakeup task that already in running or stopped */
	if (task->state == HEV_TASK_RUNNING || task->state == HEV_TASK_STOPPED)
		return;

	task->state = HEV_TASK_RUNNING;

	ctx = hev_task_system_get_context ();
	ctx->running_task_count ++;
}

void
hev_task_system_kill_current_task (jmp_buf kernel_context)
{
	longjmp (kernel_context, 2);
}

static inline void
hev_task_system_insert_task (HevTaskSystemContext *ctx)
{
	HevTask *task = ctx->new_task;
	int priority = task->priority;

	task->prev = &ctx->task_nodes[priority];
	task->next = ctx->task_nodes[priority].next;
	task->prev->next = task;
	task->next->prev = task;

	task->state = HEV_TASK_RUNNING;

	ctx->new_task = NULL;
	ctx->total_task_count ++;
	ctx->running_task_count ++;
}

static inline void
hev_task_system_remove_task (HevTaskSystemContext *ctx)
{
	HevTask *task = ctx->current_task;

	task->prev->next = task->next;
	task->next->prev = task->prev;

	task->state = HEV_TASK_STOPPED;

	ctx->current_task = task->prev;
	ctx->total_task_count --;
	ctx->running_task_count --;

	hev_task_unref (task);
}

static inline void
hev_task_system_update_task (HevTaskSystemContext *ctx)
{
	HevTask *task = ctx->current_task;
	int priority;

	/* remove from task list */
	task->prev->next = task->next;
	task->next->prev = task->prev;

	priority = task->next_priority;
	task->priority = priority;

	/* insert into task list */
	task->prev = &ctx->task_nodes[priority];
	task->next = ctx->task_nodes[priority].next;
	task->prev->next = task;
	task->next->prev = task;
}

static inline HevTask *
hev_task_system_pick (HevTaskSystemContext *ctx)
{
	HevTask *task;
	int i, count, timeout = 0;
	struct epoll_event events[128];

retry:
	/* io poll */
	count = epoll_wait (ctx->epoll_fd, events, 128, timeout);
	for (i=0; i<count; i++) {
		task = events[i].data.ptr;

		if (task->state == HEV_TASK_STOPPED)
			continue;
		hev_task_system_wakeup_task (task);
	}

	/* no task ready, retry */
	if (ctx->running_task_count == 0) {
		timeout = -1;
		goto retry;
	}

	/* pick a running task */
	task = ctx->current_task ? ctx->current_task->next :
		&ctx->task_nodes[HEV_TASK_PRIORITY_MIN];
	for (; task->state!=HEV_TASK_RUNNING;)
		task = task->next;

	return task;
}

