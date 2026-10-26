/* hev-sys-call.c - Task call stack range in the TEB */

#define HEV_WIN_RAW
#include "hev-sys-private.h"
#include "kern/task/hev-task-call.h"

extern void hev_task_execute (HevTaskCall *self, HevTaskCallEntry entry);

void
hev_sys_call_execute (HevTaskCall *call, HevTaskCallEntry entry)
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
