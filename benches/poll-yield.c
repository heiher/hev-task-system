/*
 ============================================================================
 Name        : poll-yield.c
 Author      : Heiher <r@hev.cc>
 Copyright   : Copyright (c) 2026 everyone.
 Description : I/O poll yield benchmark
 ============================================================================
 */

#include <stdio.h>
#include <time.h>

#include <hev-sys.h>
#include <hev-task-system.h>
#include <hev-task.h>
#include <hev-task-io.h>
#include <hev-task-io-socket.h>

#define NYIELD_SOLO (20000000L)
#define NYIELD_WAITER (2000000L)

/* The waiter holds a never-readable socket, so every schedule pass
 * wants a non-blocking reactor poll. The "timer" variant additionally
 * parks a 10s task timer: throttled polls must still fire ahead of
 * timer expiries, keeping timer wakeups precise. */
enum
{
    MODE_SOLO,
    MODE_WAITER,
    MODE_WAITER_TIMER,
};

static int sk[2];
static long nyield;
static int mode;

static double
get_time (void)
{
    struct timespec ts;

    timespec_get (&ts, TIME_UTC);
    return ts.tv_sec * 1000.0 + ts.tv_nsec / 1e6;
}

/* Waits like hev-socks5: bounded sleep on every EAGAIN. */
static int
long_yielder (HevTaskYieldType type, void *data)
{
    if (type == HEV_TASK_WAITIO)
        hev_task_sleep (10000);
    else
        hev_task_yield (type);
    return 0;
}

static void
waiter_entry (void *data)
{
    char buf[1];

    hev_task_add_fd (hev_task_self (), sk[0], POLLIN);
    if (mode == MODE_WAITER_TIMER)
        hev_task_io_read (sk[0], buf, 1, long_yielder, NULL);
    else
        hev_task_io_read (sk[0], buf, 1, NULL, NULL);
    hev_task_del_fd (hev_task_self (), sk[0]);
}

static void
compute_entry (void *data)
{
    long i;

    for (i = 0; i < nyield; i++)
        hev_task_yield (HEV_TASK_YIELD);

    /* release the waiter */
    if (mode != MODE_SOLO && write (sk[1], "x", 1) < 0)
        _exit (1);
}

static void
bench (const char *name, int m)
{
    HevTask *compute, *waiter = NULL;
    double t0, t1;

    if (hev_task_io_socket_socketpair (PF_LOCAL, SOCK_STREAM, 0, sk) < 0)
        return;

    mode = m;
    nyield = (m == MODE_SOLO) ? NYIELD_SOLO : NYIELD_WAITER;

    if (m != MODE_SOLO) {
        waiter = hev_task_new (-1);
        hev_task_run (waiter, waiter_entry, NULL);
    }

    compute = hev_task_new (-1);
    hev_task_run (compute, compute_entry, NULL);

    t0 = get_time ();
    hev_task_system_run ();
    t1 = get_time ();

    printf ("%-22s: %10.0f yields/s (%7.1f ns/yield)\n", name,
            nyield / ((t1 - t0) / 1000), (t1 - t0) * 1e6 / nyield);

    hev_task_io_close (sk[0]);
    hev_task_io_close (sk[1]);
    hev_task_unref (compute);
    if (waiter)
        hev_task_unref (waiter);
}

int
main (void)
{
    if (hev_task_system_init () < 0)
        return 1;

    bench ("yield (solo)", MODE_SOLO);
    bench ("yield (fd waiter)", MODE_WAITER);
    bench ("yield (waiter+timer)", MODE_WAITER_TIMER);

    hev_task_system_fini ();
    return 0;
}
