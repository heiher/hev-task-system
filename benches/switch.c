/*
 ============================================================================
 Name        : switch.c
 Author      : Heiher <r@hev.cc>
 Copyright   : Copyright (c) 2026 everyone.
 Description : Context switch benchmark
 ============================================================================
 */

#include <stdio.h>
#include <time.h>

#include <hev-task-system.h>
#include <hev-task.h>

#define NCALLS (500000)

static HevTask *task_peer;

static double
get_time (void)
{
    struct timespec ts;

    clock_gettime (CLOCK_MONOTONIC, &ts);
    return ts.tv_sec * 1000.0 + ts.tv_nsec / 1e6;
}

static void
ping_entry (void *data)
{
    int i = (int)(long)data;

    for (; i > 0; i--) {
        hev_task_wakeup (task_peer);
        hev_task_yield (HEV_TASK_YIELD);
    }
}

static void
pong_entry (void *data)
{
    int i = (int)(long)data;

    for (; i > 0; i--) {
        hev_task_wakeup (task_peer);
        hev_task_yield (HEV_TASK_YIELD);
    }
}

int
main (void)
{
    HevTask *task;
    double t0, t1;

    if (hev_task_system_init () < 0)
        return 1;

    task = hev_task_new (-1);
    task_peer = hev_task_new (-1);
    hev_task_run (task, ping_entry, (void *)(long)NCALLS);
    hev_task_run (task_peer, pong_entry, (void *)(long)NCALLS);

    t0 = get_time ();
    hev_task_system_run ();
    t1 = get_time ();

    printf ("%-22s: %9.0f switches/s (%8.1f ns/pair)\n", "yield switch",
            NCALLS / ((t1 - t0) / 1000), (t1 - t0) * 1e6 / NCALLS);

    hev_task_unref (task);
    hev_task_unref (task_peer);

    hev_task_system_fini ();
    return 0;
}
