/*
 ============================================================================
 Name        : hev-sys-task-stack.h
 Author      : Heiher <r@hev.cc>
 Copyright   : Copyright (c) 2026 everyone.
 Description : Task stack range (no-op on POSIX)
 ============================================================================
 */

#ifndef __HEV_SYS_UNIX_TASK_STACK_H__
#define __HEV_SYS_UNIX_TASK_STACK_H__

typedef struct _HevSysTaskStack HevSysTaskStack;

struct _HevSysTaskStack
{
};

#define hev_sys_task_stack_enter(self, base, limit) ((void)0)
#define hev_sys_task_stack_leave(self) ((void)0)

#endif /* __HEV_SYS_UNIX_TASK_STACK_H__ */
