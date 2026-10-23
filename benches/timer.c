/*
 ============================================================================
 Name        : timer.c
 Author      : Heiher <r@hev.cc>
 Copyright   : Copyright (c) 2026 everyone.
 Description : Timer sleep benchmark
 ============================================================================
 */

#include <stdio.h>
#include <time.h>

#include <hev-task-system.h>
#include <hev-task.h>

#define NTASKS (100)
#define NLOOPS (200)

static double
get_time (void)
{
    struct timespec ts;

    clock_gettime (CLOCK_MONOTONIC, &ts);
    return ts.tv_sec * 1000.0 + ts.tv_nsec / 1e6;
}

static void
sleep_entry (void *data)
{
    int i;

    for (i = 0; i < NLOOPS; i++)
        hev_task_sleep (1);
}

int
main (void)
{
    HevTask *tasks[NTASKS];
    double t0, t1;
    int i;

    if (hev_task_system_init () < 0)
        return 1;

    for (i = 0; i < NTASKS; i++) {
        tasks[i] = hev_task_new (-1);
        hev_task_run (tasks[i], sleep_entry, NULL);
    }

    t0 = get_time ();
    hev_task_system_run ();
    t1 = get_time ();

    /* Wall time is bounded below by NLOOPS ms: all tasks sleep in
     * parallel, so this measures timer overhead, not sleep latency. */
    printf ("%-22s: %9.0f sleeps/s (wall %.1f ms, lower bound %d ms)\n",
            "sleep(1) parallel", NTASKS * NLOOPS / ((t1 - t0) / 1000), t1 - t0,
            NLOOPS);

    for (i = 0; i < NTASKS; i++)
        hev_task_unref (tasks[i]);

    hev_task_system_fini ();
    return 0;
}
