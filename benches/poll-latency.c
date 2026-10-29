/*
 ============================================================================
 Name        : poll-latency.c
 Author      : Heiher <r@hev.cc>
 Copyright   : Copyright (c) 2026 everyone.
 Description : I/O wakeup latency benchmark
 ============================================================================
 */

#include <fcntl.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include <hev-sys.h>
#include <hev-task-system.h>
#include <hev-task.h>
#include <hev-task-io.h>
#include <hev-task-io-pipe.h>

/* A compute task spins to keep the scheduler busy with runnable work
 * while a peer thread writes to a pipe every few ms. The reported
 * write()->read() latency is the price of deferring non-blocking I/O
 * polls: it is bounded by the poll interval. */
#define NSAMPLES 300
#define WRITE_PERIOD_US 3000
#define SPIN_BURST 1000

static int pipe_fds[2];
static volatile int done;
static double t_write[NSAMPLES];
static double lat_us[NSAMPLES];

static double
get_time (void)
{
    struct timespec ts;

    timespec_get (&ts, TIME_UTC);
    return ts.tv_sec * 1000.0 + ts.tv_nsec / 1e6;
}

static void *
writer_thread (void *data)
{
    int i;

    for (i = 0; i < NSAMPLES && !done; i++) {
        t_write[i] = get_time ();
        if (write (pipe_fds[1], "x", 1) < 0)
            _exit (1);
        usleep (WRITE_PERIOD_US);
    }
    return NULL;
}

static void
waiter_entry (void *data)
{
    char buf[1];
    int i;

    for (i = 0; i < NSAMPLES; i++) {
        if (hev_task_io_read (pipe_fds[0], buf, 1, NULL, NULL) != 1)
            _exit (1);
        lat_us[i] = (get_time () - t_write[i]) * 1000.0;
    }

    done = 1;
}

static void
spin_entry (void *data)
{
    while (!done) {
        int i;

        for (i = 0; i < SPIN_BURST; i++)
            hev_task_yield (HEV_TASK_YIELD);
    }
}

static int
cmp (const void *a, const void *b)
{
    double x = *(const double *)a, y = *(const double *)b;

    return (x > y) - (x < y);
}

int
main (void)
{
    HevTask *waiter, *spin;
    double sorted[NSAMPLES], sum = 0;
    pthread_t tid;
    int i;

    if (hev_task_system_init () < 0)
        return 1;

    if (hev_task_io_pipe_pipe (pipe_fds) < 0)
        return 1;

    waiter = hev_task_new (-1);
    hev_task_add_fd (waiter, pipe_fds[0], POLLIN);
    hev_task_run (waiter, waiter_entry, NULL);

    spin = hev_task_new (-1);
    hev_task_run (spin, spin_entry, NULL);

    pthread_create (&tid, NULL, writer_thread, NULL);
    pthread_detach (tid);

    hev_task_system_run ();

    memcpy (sorted, lat_us, sizeof (lat_us));
    qsort (sorted, NSAMPLES, sizeof (double), cmp);
    for (i = 0; i < NSAMPLES; i++)
        sum += lat_us[i];

    printf ("%-22s: min %5.1f us, avg %5.1f us, p50 %5.1f us, "
            "p99 %5.1f us, max %5.1f us\n",
            "wake latency", sorted[0], sum / NSAMPLES,
            sorted[(NSAMPLES * 50) / 100], sorted[(NSAMPLES * 99) / 100],
            sorted[NSAMPLES - 1]);

    hev_task_io_close (pipe_fds[0]);
    hev_task_io_close (pipe_fds[1]);
    hev_task_unref (waiter);
    hev_task_unref (spin);

    hev_task_system_fini ();
    return 0;
}
