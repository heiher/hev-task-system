/*
 ============================================================================
 Name        : hev-sys-task-stack.c
 Author      : Heiher <r@hev.cc>
 Copyright   : Copyright (c) 2026 everyone.
 Description : Task stack range in the TEB
 ============================================================================
 */

#define HEV_SYS_RAW
#include "hev-sys-private.h"

void
hev_sys_task_stack_enter (HevSysTaskStack *self, void *base, void *limit)
{
    NT_TIB *tib = (NT_TIB *)NtCurrentTeb ();

    self->base = tib->StackBase;
    self->limit = tib->StackLimit;

    tib->StackBase = base;
    tib->StackLimit = limit;
}

void
hev_sys_task_stack_leave (HevSysTaskStack *self)
{
    NT_TIB *tib = (NT_TIB *)NtCurrentTeb ();

    if (!self->base)
        return;

    tib->StackBase = self->base;
    tib->StackLimit = self->limit;
}
