/*
 ============================================================================
 Name        : hev-task-call.c
 Author      : hev <r@hev.cc>
 Copyright   : Copyright (c) 2020 - 2026 hev
 Description : Task call
 ============================================================================
 */

#include <stdint.h>

#include "sys/hev-sys-private.h"
#include <hev-task-call.h>
#include <hev-memory-allocator.h>

#include "lib/misc/hev-compiler.h"

extern void hev_task_execute (void *self, void *entry);

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
    hev_sys_task_execute (self, self->stack_top, self, entry);
    return self->retval;
}
