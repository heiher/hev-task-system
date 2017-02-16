/*
 ============================================================================
 Name        : hev-task-private.h
 Author      : Heiher <r@hev.cc>
 Copyright   : Copyright (c) 2017 everyone.
 Description :
 ============================================================================
 */

#ifndef __HEV_TASK_PRIVATE_H__
#define __HEV_TASK_PRIVATE_H__

#include <setjmp.h>

#include "hev-task.h"

struct _HevTask
{
	void *stack;
	HevTaskEntry entry;
	void *data;

	HevTask *prev;
	HevTask *next;

	int ref_count;
	int priority;
	int next_priority;
	int stack_size;

	jmp_buf context;
};

extern void hev_task_execute (HevTask *self);

#endif /* __HEV_TASK_PRIVATE_H__ */

