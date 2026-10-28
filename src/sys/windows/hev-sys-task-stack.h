/*
 ============================================================================
 Name        : hev-sys-task-stack.h
 Author      : Heiher <r@hev.cc>
 Copyright   : Copyright (c) 2026 everyone.
 Description : Task stack range in the TEB
 ============================================================================
 */

#ifndef __HEV_SYS_TASK_STACK_H__
#define __HEV_SYS_TASK_STACK_H__

#ifdef __cplusplus
extern "C" {
#endif

typedef struct _HevSysTaskStack HevSysTaskStack;

struct _HevSysTaskStack
{
    void *base;
    void *limit;
};

void hev_sys_task_stack_enter (HevSysTaskStack *self, void *base, void *limit);
void hev_sys_task_stack_leave (HevSysTaskStack *self);

#ifdef __cplusplus
}
#endif

#endif /* __HEV_SYS_TASK_STACK_H__ */
