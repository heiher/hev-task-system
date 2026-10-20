/*
 ============================================================================
 Name        : hev-task-call.c
 Author      : Heiher <r@hev.cc>
 Copyright   : Copyright (c) 2020 everyone.
 Description : Task call
 ============================================================================
 */

#include <stdint.h>
#if defined(HEV_MSVC_ABI)
#include <windows.h>
#endif

#include "lib/misc/hev-compiler.h"
#include "mem/api/hev-memory-allocator-api.h"

#include "hev-task-call.h"

extern void hev_task_execute (HevTaskCall *self, HevTaskCallEntry entry);

EXPORT_SYMBOL HevTaskCall *
hev_task_call_new (int base_size, int stack_size)
{
    HevTaskCall *self;

    self = hev_malloc (base_size + stack_size);
    if (self) {
        uintptr_t stack_addr;
        stack_addr = (uintptr_t)(char *)self + base_size + stack_size;
        self->stack_top = (void *)ALIGN_DOWN (stack_addr, 16);
    }

    return self;
}

EXPORT_SYMBOL void
hev_task_call_destroy (HevTaskCall *self)
{
    hev_free (self);
}

EXPORT_SYMBOL void *
hev_task_call_jump (HevTaskCall *self, HevTaskCallEntry entry)
{
#if defined(HEV_MSVC_ABI)
    /* See hev-task-system-schedule.c: keep TEB.StackLimit consistent with the
     * stack the entry runs on, then restore the caller's range (which may be
     * a task stack itself). */
    NT_TIB *tib = (NT_TIB *)NtCurrentTeb ();
    void *saved_base = tib->StackBase;
    void *saved_limit = tib->StackLimit;

    tib->StackBase = self->stack_top;
    tib->StackLimit = (void *)self;
    hev_task_execute (self, entry);
    tib->StackBase = saved_base;
    tib->StackLimit = saved_limit;
#else
    hev_task_execute (self, entry);
#endif
    return self->retval;
}
