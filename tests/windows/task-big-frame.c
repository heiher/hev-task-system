/* task-big-frame.c - Large stack frame on a heap-allocated coroutine stack */

#include <assert.h>
#include <stdint.h>

#include <hev-task.h>
#include <hev-task-system.h>

/* A 64 KiB frame exceeds the __chkstk threshold; the stack probe must only
 * touch pages of the task's own heap-allocated stack. */

#define TASK_COUNT 4
#define FRAME_SIZE (64 * 1024)
#define PAGE_STRIDE 4096

static void
task_entry (void *data)
{
    volatile uint8_t frame[FRAME_SIZE];
    uintptr_t id = (uintptr_t)data;
    int i;

    for (i = 0; i < FRAME_SIZE; i += PAGE_STRIDE)
        frame[i] = (uint8_t)(i / PAGE_STRIDE + 1 + id);
    for (i = 0; i < FRAME_SIZE; i += PAGE_STRIDE)
        assert (frame[i] == (uint8_t)(i / PAGE_STRIDE + 1 + id));
}

int
main (void)
{
    hev_task_system_init ();

    for (uintptr_t i = 0; i < TASK_COUNT; i++) {
        HevTask *task = hev_task_new (FRAME_SIZE * 2);
        assert (task);
        hev_task_run (task, task_entry, (void *)i);
    }

    hev_task_system_run ();
    hev_task_system_fini ();

    return 0;
}
