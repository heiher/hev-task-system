/*
 ============================================================================
 Name        : hev-sys-task-execute.c
 Author      : Heiher <r@hev.cc>
 Copyright   : Copyright (c) 2026 everyone.
 Description : Task call stack range in the TEB
 ============================================================================
 */

#define HEV_SYS_RAW
#include "hev-sys-private.h"
#include "hev-sys-task-execute.h"
#include "kern/task/hev-task-call.h"

extern void hev_task_execute (HevTaskCall *self, HevTaskCallEntry entry);

void
hev_sys_task_execute (HevTaskCall *call, HevTaskCallEntry entry)
{
    NT_TIB *tib = (NT_TIB *)NtCurrentTeb ();
    void *base = tib->StackBase;
    void *limit = tib->StackLimit;

    tib->StackBase = call->stack_top;
    tib->StackLimit = call;

    hev_task_execute (call, entry);

    tib->StackBase = base;
    tib->StackLimit = limit;
}
