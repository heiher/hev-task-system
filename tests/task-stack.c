/*
 ============================================================================
 Name        : task-stack.c
 Author      : Heiher <r@hev.cc>
 Copyright   : Copyright (c) 2026 everyone.
 Description : Task Stack Test
 ============================================================================
 */

#include <stddef.h>
#include <stdint.h>
#include <string.h>
#include <assert.h>
#include <unistd.h>
#include <alloca.h>

#include <hev-task.h>
#include <hev-task-call.h>
#include <hev-task-system.h>

#define BUF_SIZE (32 * 1024)
#define CALL_STACK_SIZE (64 * 1024)

static unsigned char *buf_addr;
static HevTaskCall *call;
static HevTask *other;

static __attribute__ ((noinline)) void
touch_stack (int fill)
{
    volatile unsigned char *buf = alloca (BUF_SIZE);

    buf_addr = (unsigned char *)buf;
    memset ((void *)buf, fill, BUF_SIZE);
}

static __attribute__ ((noinline)) void
check_zero (void)
{
    uintptr_t page_size = getpagesize ();
    uintptr_t start = ((uintptr_t)buf_addr + page_size - 1) & ~(page_size - 1);
    uintptr_t end = (uintptr_t)buf_addr + BUF_SIZE / 2;
    volatile unsigned char *p;

    /* The lower half of the buffer is far below the release frame and the
     * frames used since, so it must be fully released. */
    for (p = (unsigned char *)start; (uintptr_t)p < end; p++)
        assert (*p == 0);
}

static void
call_entry (HevTaskCall *call)
{
    /* Not on the task's stack, must be a no-op. */
    hev_task_release_stack (hev_task_self ());
}

static void
task_entry (void *data)
{
    volatile unsigned char live[64];
    int i;

    /* Not the running task, must be a no-op. */
    hev_task_release_stack (other);

    for (i = 0; i < 3; i++) {
        touch_stack (0x5a);
        hev_task_release_stack (hev_task_self ());
        hev_task_yield (HEV_TASK_YIELD);
#ifdef __linux__
        check_zero ();
#endif
    }

    memset ((void *)live, 0xa5, sizeof (live));
    hev_task_call_jump (call, call_entry);
    for (i = 0; i < sizeof (live); i++)
        assert (live[i] == 0xa5);

    touch_stack (0xa5);
}

int
main (int argc, char *argv[])
{
    unsigned char call_stack[CALL_STACK_SIZE] __attribute__ ((aligned (16)));
    HevTaskCall call_obj;
    HevTask *task;

    assert (hev_task_system_init () == 0);

    /* Put the call stack on the main stack, above the task's stack, so a
     * release from it would cover the task's live frames. */
    call_obj.stack_top = call_stack + CALL_STACK_SIZE;
    call = &call_obj;

    other = hev_task_new (-1);
    assert (other);

    task = hev_task_new (-1);
    assert (task);
    hev_task_run (task, task_entry, NULL);
    hev_task_system_run ();

    hev_task_unref (other);
    hev_task_system_fini ();

    return 0;
}
