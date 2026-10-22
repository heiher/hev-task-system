/*
 ============================================================================
 Name        : task-cond-timedwait.c
 Author      : Heiher <r@hev.cc>
 Copyright   : Copyright (c) 2025 everyone.
 Description : Task Condition Timedwait Test
 ============================================================================
 */

#include <stddef.h>
#include <assert.h>

#include <hev-task.h>
#include <hev-task-cond.h>
#include <hev-task-mutex.h>
#include <hev-task-system.h>

static HevTaskMutex mutex;
static HevTaskCond cond;

static void
task_late_entry (void *data)
{
    /* Queued behind the early waiter. Must be woken by its signal
     * rather than running into the timeout. */
    assert (hev_task_mutex_lock (&mutex) == 0);
    assert (hev_task_cond_timedwait (&cond, &mutex, 2000) == 0);
    assert (hev_task_mutex_unlock (&mutex) == 0);
}

static void
task_early_entry (void *data)
{
    /* Head of the waiter queue; times out first. */
    assert (hev_task_mutex_lock (&mutex) == 0);
    assert (hev_task_cond_timedwait (&cond, &mutex, 50) == -1);
    assert (hev_task_mutex_unlock (&mutex) == 0);

    assert (hev_task_mutex_lock (&mutex) == 0);
    assert (hev_task_cond_signal (&cond) == 0);
    assert (hev_task_mutex_unlock (&mutex) == 0);
}

int
main (int argc, char *argv[])
{
    HevTask *task_late, *task_early;

    assert (hev_task_system_init () == 0);
    assert (hev_task_mutex_init (&mutex) == 0);
    assert (hev_task_cond_init (&cond) == 0);

    task_late = hev_task_new (-1);
    assert (task_late);
    hev_task_run (task_late, task_late_entry, NULL);

    task_early = hev_task_new (-1);
    assert (task_early);
    hev_task_run (task_early, task_early_entry, NULL);

    hev_task_system_run ();

    hev_task_unref (task_late);
    hev_task_unref (task_early);

    hev_task_system_fini ();
    return 0;
}
