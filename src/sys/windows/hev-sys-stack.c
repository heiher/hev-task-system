/*
 ============================================================================
 Name        : hev-sys-stack.c
 Author      : Heiher <r@hev.cc>
 Copyright   : Copyright (c) 2026 everyone.
 Description : Task stack range in the TEB and the stack detector stub
 ============================================================================
 */

#define HEV_SYS_RAW
#include "sys/hev-sys-private.h"
#include "kern/task/hev-task-private.h"
#include "kern/task/hev-task-stack.h"
#include "lib/misc/hev-task-stack-detector.h"

/*
 * Coroutine stacks live on the heap, outside the range recorded in the TEB.
 * __chkstk compares the new stack pointer with TEB.StackLimit and probes every
 * page in between, which walks over unrelated memory for a heap stack.
 */
static __thread void *thread_stack_base;
static __thread void *thread_stack_limit;

void
hev_sys_stack_enter (HevTask *task)
{
    NT_TIB *tib = (NT_TIB *)NtCurrentTeb ();

    thread_stack_base = tib->StackBase;
    thread_stack_limit = tib->StackLimit;

    tib->StackBase = task->stack_bottom;
    tib->StackLimit = hev_task_stack_get_base (task->stack);
}

void
hev_sys_stack_leave (void)
{
    NT_TIB *tib = (NT_TIB *)NtCurrentTeb ();

    if (!thread_stack_base)
        return;

    tib->StackBase = thread_stack_base;
    tib->StackLimit = thread_stack_limit;
}

void
hev_task_stack_detector_destroy (HevTaskStackDetector *self)
{
    (void)self;
}
