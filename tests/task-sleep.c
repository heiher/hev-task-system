/*
 ============================================================================
 Name        : task-sleep.c
 Author      : Heiher <r@hev.cc>
 Copyright   : Copyright (c) 2018 everyone.
 Description : Task Sleep Test
 ============================================================================
 */

#include <time.h>
#include <stddef.h>
#include <assert.h>

#include <hev-task.h>
#include <hev-task-system.h>

static void
task_entry (void *data)
{
    struct timespec sp1, sp2;
    time_t sec;
    long nsec, usec, msec;

    clock_gettime (CLOCK_MONOTONIC, &sp1);
    hev_task_usleep (100);
    clock_gettime (CLOCK_MONOTONIC, &sp2);
    sec = sp2.tv_sec - sp1.tv_sec;
    nsec = sp2.tv_nsec - sp1.tv_nsec;
    if (nsec < 0) {
        sec--;
        nsec += 1000000000L;
    }
    usec = sec * 1000000 + nsec / 1000;
    assert (usec >= 100);

    clock_gettime (CLOCK_MONOTONIC, &sp1);
    hev_task_sleep (100);
    clock_gettime (CLOCK_MONOTONIC, &sp2);
    sec = sp2.tv_sec - sp1.tv_sec;
    nsec = sp2.tv_nsec - sp1.tv_nsec;
    if (nsec < 0) {
        sec--;
        nsec += 1000000000L;
    }
    msec = sec * 1000 + nsec / 1000000;
    assert (msec >= 100);
}

int
main (int argc, char *argv[])
{
    HevTask *task;

    assert (hev_task_system_init () == 0);

    task = hev_task_new (-1);
    assert (task);
    hev_task_run (task, task_entry, NULL);

    hev_task_system_run ();

    hev_task_system_fini ();

    return 0;
}
