/*
 ============================================================================
 Name        : channel.c
 Author      : Heiher <r@hev.cc>
 Copyright   : Copyright (c) 2026 everyone.
 Description : Channel round trip benchmark
 ============================================================================
 */

#include <stdio.h>
#include <time.h>

#include <hev-task-system.h>
#include <hev-task.h>
#include <hev-task-channel.h>

#define NOPS (200000)

static HevTaskChannel *chan_req;
static HevTaskChannel *chan_rep;

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
    printf ("%-22s: %9.0f rtt/s  (%8.1f ns/rtt)\n", name,
            n / ((t1 - t0) / 1000), (t1 - t0) * 1e6 / n);
}

static void
speaker_entry (void *data)
{
    long i, n = (long)data;
    long val = 0;
    ssize_t s;

    for (i = 0; i < n; i++) {
        s = hev_task_channel_write (chan_req, &val, sizeof (val));
        if (s < 0)
            return;
        s = hev_task_channel_read (chan_req, &val, sizeof (val));
        if (s < 0)
            return;
    }

    /* Wake the peer so its blocked read fails and it can exit. */
    hev_task_channel_destroy (chan_req);
    hev_task_channel_destroy (chan_rep);
}

static void
echo_entry (void *data)
{
    long val;
    ssize_t s;

    for (;;) {
        s = hev_task_channel_read (chan_rep, &val, sizeof (val));
        if (s <= 0)
            return;
        s = hev_task_channel_write (chan_rep, &val, sizeof (val));
        if (s < 0)
            return;
    }
}

static void
bench_channel (unsigned int buffers, const char *name)
{
    HevTask *tasks[2];
    double t0, t1;

    hev_task_channel_new_with_buffers (&chan_req, &chan_rep, sizeof (long),
                                       buffers);
    tasks[0] = hev_task_new (-1);
    hev_task_run (tasks[0], speaker_entry, (void *)(long)NOPS);
    tasks[1] = hev_task_new (-1);
    hev_task_run (tasks[1], echo_entry, NULL);

    t0 = get_time ();
    hev_task_system_run ();
    t1 = get_time ();
    report (name, NOPS, t0, t1);

    hev_task_unref (tasks[0]);
    hev_task_unref (tasks[1]);
}

int
main (void)
{
    if (hev_task_system_init () < 0)
        return 1;

    bench_channel (0, "channel rtt (unbuf)");
    bench_channel (8, "channel rtt (buf8)");

    hev_task_system_fini ();
    return 0;
}
