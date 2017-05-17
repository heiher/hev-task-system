/*
 ============================================================================
 Name        : hev-task-system-private.h
 Author      : Heiher <r@hev.cc>
 Copyright   : Copyright (c) 2017 everyone.
 Description :
 ============================================================================
 */

#ifndef __HEV_TASK_SYSTEM_PRIVATE_H__
#define __HEV_TASK_SYSTEM_PRIVATE_H__

#include <setjmp.h>

#include "hev-task.h"
#include "hev-task-system.h"

#define PRIORITY_COUNT	(HEV_TASK_PRIORITY_MAX - HEV_TASK_PRIORITY_MIN + 1)

typedef struct _HevTaskSystemContext HevTaskSystemContext;

struct _HevTaskSystemContext
{
	int epoll_fd;
	unsigned int task_count;

	HevTask *running_lists[PRIORITY_COUNT];
	HevTask *waiting_lists[HEV_TASK_YIELD_COUNT];
	HevTask *current_task;
	HevTask *new_task;

	jmp_buf kernel_context;
};

void hev_task_system_schedule (HevTaskYieldType type, HevTask *new_task);
void hev_task_system_wakeup_task (HevTask *task);

HevTaskSystemContext * hev_task_system_get_context (void);

#endif /* __HEV_TASK_SYSTEM_PRIVATE_H__ */

