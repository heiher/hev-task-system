/*
 ============================================================================
 Name        : task-sleep.c
 Author      : Heiher <r@hev.cc>
 Copyright   : Copyright (c) 2018 - 2019 everyone.
 Description : Task Sleep Test
 ============================================================================
 */

#include <time.h>
#include <stddef.h>
#include <assert.h>

#include <hev-task.h>
#include <hev-task-system.h>

static int counter;

static long
time_diff (struct timespec *sp1, struct timespec *sp2)
{
    time_t sec;
    long nsec;

    sec = sp2->tv_sec - sp1->tv_sec;
    nsec = sp2->tv_nsec - sp1->tv_nsec;
    if (nsec < 0) {
        sec--;
        nsec += 1000000000L;
    }

    return sec * 1000000 + nsec / 1000;
}

static void
task_entry11 (void *data)
{
    struct timespec sp1, sp2;

    assert (counter == 2);
    counter++;
    clock_gettime (CLOCK_MONOTONIC, &sp1);
    assert (hev_task_usleep (300) == 0);
    clock_gettime (CLOCK_MONOTONIC, &sp2);
    assert (time_diff (&sp1, &sp2) >= 300);
}

static void
task_entry10 (void *data)
{
    struct timespec sp1, sp2;

    assert (counter == 1);
    counter++;
    clock_gettime (CLOCK_MONOTONIC, &sp1);
    assert (hev_task_usleep (200) == 0);
    clock_gettime (CLOCK_MONOTONIC, &sp2);
    assert (time_diff (&sp1, &sp2) >= 200);
}

static void
task_entry9 (void *data)
{
    struct timespec sp1, sp2;

    counter++;
    clock_gettime (CLOCK_MONOTONIC, &sp1);
    assert (hev_task_usleep (100) == 0);
    clock_gettime (CLOCK_MONOTONIC, &sp2);
    assert (time_diff (&sp1, &sp2) >= 100);
}

static void
task_entry8 (void *data)
{
    HevTask *task = data;

    assert (counter == 2);
    hev_task_wakeup (task);
}

static void
task_entry7 (void *data)
{
    struct timespec sp1, sp2;
    unsigned int remaining;
    HevTask *task;

    task = hev_task_new (-1);
    assert (task);
    hev_task_set_priority (task, 1);
    hev_task_run (task, task_entry8, hev_task_self ());

    assert (counter == 1);
    counter++;
    clock_gettime (CLOCK_MONOTONIC, &sp1);
    remaining = hev_task_usleep (200);
    assert (remaining >= 0);
    assert (remaining <= 200);
    assert (hev_task_usleep (remaining) == 0);
    clock_gettime (CLOCK_MONOTONIC, &sp2);
    assert (time_diff (&sp1, &sp2) >= 200);
}

static void
task_entry6 (void *data)
{
    struct timespec sp1, sp2;

    counter++;
    clock_gettime (CLOCK_MONOTONIC, &sp1);
    assert (hev_task_usleep (100) == 0);
    clock_gettime (CLOCK_MONOTONIC, &sp2);
    assert (time_diff (&sp1, &sp2) >= 100);
}

static void
task_entry5 (void *data)
{
    struct timespec sp1, sp2;

    counter++;
    clock_gettime (CLOCK_MONOTONIC, &sp1);
    assert (hev_task_usleep (200) == 0);
    clock_gettime (CLOCK_MONOTONIC, &sp2);
    assert (time_diff (&sp1, &sp2) >= 200);
}

static void
task_entry4 (void *data)
{
    struct timespec sp1, sp2;

    assert (counter == 1);
    clock_gettime (CLOCK_MONOTONIC, &sp1);
    assert (hev_task_usleep (100) == 0);
    clock_gettime (CLOCK_MONOTONIC, &sp2);
    assert (time_diff (&sp1, &sp2) >= 100);
}

static void
task_entry3 (void *data)
{
    struct timespec sp1, sp2;

    assert (counter == 1);
    clock_gettime (CLOCK_MONOTONIC, &sp1);
    assert (hev_task_usleep (200) == 0);
    clock_gettime (CLOCK_MONOTONIC, &sp2);
    assert (time_diff (&sp1, &sp2) >= 200);
}

static void
task_entry2 (void *data)
{
    struct timespec sp1, sp2;

    counter++;
    clock_gettime (CLOCK_MONOTONIC, &sp1);
    assert (hev_task_usleep (100) == 0);
    clock_gettime (CLOCK_MONOTONIC, &sp2);
    assert (time_diff (&sp1, &sp2) >= 100);
}

static void
task_entry1 (void *data)
{
    struct timespec sp1, sp2;

    clock_gettime (CLOCK_MONOTONIC, &sp1);
    assert (hev_task_usleep (100) == 0);
    clock_gettime (CLOCK_MONOTONIC, &sp2);
    assert (time_diff (&sp1, &sp2) >= 100);

    clock_gettime (CLOCK_MONOTONIC, &sp1);
    assert (hev_task_sleep (100) == 0);
    clock_gettime (CLOCK_MONOTONIC, &sp2);
    assert (time_diff (&sp1, &sp2) >= (100 * 1000));
}

int
main (int argc, char *argv[])
{
    HevTask *task;

    assert (hev_task_system_init () == 0);

    task = hev_task_new (-1);
    assert (task);
    hev_task_run (task, task_entry1, NULL);
    hev_task_system_run ();

    task = hev_task_new (-1);
    assert (task);
    hev_task_set_priority (task, 1);
    hev_task_run (task, task_entry2, NULL);
    task = hev_task_new (-1);
    assert (task);
    hev_task_set_priority (task, 2);
    hev_task_run (task, task_entry3, NULL);
    hev_task_system_run ();

    counter = 0;
    task = hev_task_new (-1);
    assert (task);
    hev_task_set_priority (task, 2);
    hev_task_run (task, task_entry4, NULL);
    task = hev_task_new (-1);
    assert (task);
    hev_task_set_priority (task, 1);
    hev_task_run (task, task_entry5, NULL);
    hev_task_system_run ();

    counter = 0;
    task = hev_task_new (-1);
    assert (task);
    hev_task_set_priority (task, 1);
    hev_task_run (task, task_entry6, NULL);
    task = hev_task_new (-1);
    assert (task);
    hev_task_set_priority (task, 2);
    hev_task_run (task, task_entry7, NULL);
    hev_task_system_run ();

    counter = 0;
    task = hev_task_new (-1);
    assert (task);
    hev_task_set_priority (task, 1);
    hev_task_run (task, task_entry9, NULL);
    task = hev_task_new (-1);
    assert (task);
    hev_task_set_priority (task, 2);
    hev_task_run (task, task_entry10, NULL);
    task = hev_task_new (-1);
    assert (task);
    hev_task_set_priority (task, 3);
    hev_task_run (task, task_entry11, NULL);
    hev_task_system_run ();

    hev_task_system_fini ();

    return 0;
}
