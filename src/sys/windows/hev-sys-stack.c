/* hev-win-stack.c - Task stack range in the TEB and the stack detector stub */

#define HEV_WIN_RAW
#include "hev-sys.h"
#include "lib/misc/hev-task-stack-detector.h"

/*
 * Coroutine stacks live on the heap, outside the range recorded in the TEB.
 * __chkstk compares the new stack pointer with TEB.StackLimit and probes every
 * page in between, which walks over unrelated memory for a heap stack.
 */
static __thread void *thread_stack_base;
static __thread void *thread_stack_limit;

void
hev_win_stack_enter (void *bottom, void *limit)
{
    NT_TIB *tib = (NT_TIB *)NtCurrentTeb ();

    thread_stack_base = tib->StackBase;
    thread_stack_limit = tib->StackLimit;

    tib->StackBase = bottom;
    tib->StackLimit = limit;
}

void
hev_win_stack_leave (void)
{
    NT_TIB *tib = (NT_TIB *)NtCurrentTeb ();

    if (!thread_stack_base)
        return;

    tib->StackBase = thread_stack_base;
    tib->StackLimit = thread_stack_limit;
}

void
hev_win_stack_swap (void *bottom, void *limit, void *saved[2])
{
    NT_TIB *tib = (NT_TIB *)NtCurrentTeb ();

    saved[0] = tib->StackBase;
    saved[1] = tib->StackLimit;

    tib->StackBase = bottom;
    tib->StackLimit = limit;
}

void
hev_win_stack_restore (void *saved[2])
{
    NT_TIB *tib = (NT_TIB *)NtCurrentTeb ();

    tib->StackBase = saved[0];
    tib->StackLimit = saved[1];
}

void
hev_task_stack_detector_destroy (HevTaskStackDetector *self)
{
    (void)self;
}
