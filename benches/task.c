/*
 ============================================================================
 Name        : task.c
 Author      : Heiher <r@hev.cc>
 Copyright   : Copyright (c) 2026 everyone.
 Description : Task lifecycle benchmark
 ============================================================================
 */

#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include <hev-task-system.h>
#include <hev-task.h>

#define NOPS (20000)

static double
get_time (void)
{
    struct timespec ts;

    clock_gettime (CLOCK_MONOTONIC, &ts);
    return ts.tv_sec * 1000.0 + ts.tv_nsec / 1e6;
}

static void
report (const char *name, double n, double t0, double t1)
{
    printf ("%-22s: %9.0f ops/s  (%8.1f ns/op)\n", name, n / ((t1 - t0) / 1000),
            (t1 - t0) * 1e6 / n);
}

static void
bench_new_free (int stack_size, const char *name)
{
    double t0, t1;
    int i;

    t0 = get_time ();
    for (i = 0; i < NOPS; i++) {
        HevTask *task = hev_task_new (stack_size);
        hev_task_unref (task);
    }
    t1 = get_time ();
    report (name, NOPS, t0, t1);
}

static void
exit_entry (void *data)
{
    hev_task_exit ();
}

static void
bench_run_exit (int count)
{
    HevTask **tasks;
    double t0, t1;
    char name[64];
    int i;

    tasks = malloc (sizeof (HevTask *) * count);
    for (i = 0; i < count; i++)
        tasks[i] = hev_task_new (-1);

    t0 = get_time ();
    for (i = 0; i < count; i++)
        hev_task_run (tasks[i], exit_entry, NULL);
    hev_task_system_run ();
    t1 = get_time ();

    snprintf (name, sizeof (name), "run+exit x%d", count);
    report (name, count, t0, t1);

    for (i = 0; i < count; i++)
        hev_task_unref (tasks[i]);
    free (tasks);
}

int
main (void)
{
    if (hev_task_system_init () < 0)
        return 1;

    bench_new_free (-1, "task new+free 64k");
    bench_new_free (8 * 1024, "task new+free 8k");
    bench_run_exit (100);
    bench_run_exit (10000);

    hev_task_system_fini ();
    return 0;
}
