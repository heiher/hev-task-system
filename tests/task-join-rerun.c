/*
 ============================================================================
 Name        : task-join-rerun.c
 Author      : Heiher <r@hev.cc>
 Copyright   : Copyright (c) 2025 everyone.
 Description : Task Join Rerun Test
 ============================================================================
 */

#include <stddef.h>
#include <assert.h>

#include <hev-task.h>
#include <hev-task-system.h>

static int count;

static void
task1_entry (void *data)
{
    count++;
}

static void
task2_entry (void *data)
{
    HevTask *task1 = data;

    /* task1 ran first and is already stopped */
    assert (hev_task_get_state (task1) == HEV_TASK_STOPPED);
    assert (hev_task_join (task1) == 0);
}

int
main (int argc, char *argv[])
{
    HevTask *task1, *task2;

    assert (hev_task_system_init () == 0);

    task1 = hev_task_new (-1);
    assert (task1);
    task2 = hev_task_new (-1);
    assert (task2);

    hev_task_ref (task1);
    hev_task_ref (task2);

    /* task1 is scheduled before task2, so it exits first */
    hev_task_run (task1, task1_entry, NULL);
    hev_task_run (task2, task2_entry, task1);

    hev_task_system_run ();

    assert (count == 1);

    /* Re-run task1. The stale joiner from task2 must not be woken
     * now that task2 has stopped. */
    hev_task_run (task1, task1_entry, NULL);
    hev_task_system_run ();

    assert (count == 2);

    hev_task_unref (task1);
    hev_task_unref (task2);

    hev_task_system_fini ();
    return 0;
}
