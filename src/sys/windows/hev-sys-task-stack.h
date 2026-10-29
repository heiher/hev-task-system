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

static inline void
hev_sys_task_stack_enter (HevSysTaskStack *self, void *base, void *limit)
{
    NT_TIB *tib = (NT_TIB *)NtCurrentTeb ();

    self->base = tib->StackBase;
    self->limit = tib->StackLimit;

    tib->StackBase = base;
    tib->StackLimit = limit;
}

static inline void
hev_sys_task_stack_leave (HevSysTaskStack *self)
{
    NT_TIB *tib = (NT_TIB *)NtCurrentTeb ();

    if (!self->base)
        return;

    tib->StackBase = self->base;
    tib->StackLimit = self->limit;
}

#ifdef __cplusplus
}
#endif

#endif /* __HEV_SYS_TASK_STACK_H__ */
