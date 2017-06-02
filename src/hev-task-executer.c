/*
 ============================================================================
 Name        : hev-task-executer.c
 Author      : Heiher <r@hev.cc>
 Copyright   : Copyright (c) 2017 everyone.
 Description :
 ============================================================================
 */

#include "hev-task-executer.h"

void
hev_task_executer (HevTask *task, jmp_buf kernel_context)
{
	if (setjmp (task->context) == 0)
		return;

	task->entry (task->data);

	longjmp (kernel_context, 2);
}

