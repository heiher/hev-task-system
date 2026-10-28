/*
 ============================================================================
 Name        : hev-sys-task-execute.c
 Author      : Heiher <r@hev.cc>
 Copyright   : Copyright (c) 2026 everyone.
 Description : Task stack range in the TEB
 ============================================================================
 */

#define HEV_SYS_RAW
#include "sys/hev-sys-private.h"
#include "hev-sys-task-execute.h"

extern void hev_task_execute (void *self, void *executer);

void
hev_sys_task_execute (void *ctx, void *top, void *limit, void *entry)
{
    NT_TIB *tib = (NT_TIB *)NtCurrentTeb ();
    void *saved_base = tib->StackBase;
    void *saved_limit = tib->StackLimit;

    tib->StackBase = top;
    tib->StackLimit = limit;

    hev_task_execute (ctx, entry);

    tib->StackBase = saved_base;
    tib->StackLimit = saved_limit;
}
