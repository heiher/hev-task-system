/*
 ============================================================================
 Name        : io.c
 Author      : Heiher <r@hev.cc>
 Copyright   : Copyright (c) 2026 everyone.
 Description : Task I/O benchmark
 ============================================================================
 */

#include <fcntl.h>
#include <stdio.h>
#include <time.h>
#include <unistd.h>

#include <hev-task-system.h>
#include <hev-task.h>
#include <hev-task-io.h>

#define NOPS (200000)

static int pipe_fds[2];
static HevTaskIOYielder bench_yielder;

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

/* Waits like hev-socks5: bounded sleep on every EAGAIN. */
static int
sleep_yielder (HevTaskYieldType type, void *data)
{
    if (type == HEV_TASK_WAITIO) {
        hev_task_sleep (300000);
        return 0;
    }
    hev_task_yield (type);
    return 0;
}

static void
writer_entry (void *data)
{
    long i, n = (long)data;
    char buf[64];

    for (i = 0; i < n; i++)
        hev_task_io_write (pipe_fds[1], buf, sizeof (buf), bench_yielder, NULL);
}

static void
reader_entry (void *data)
{
    long i, n = (long)data;
    char buf[64];

    for (i = 0; i < n; i++)
        hev_task_io_read (pipe_fds[0], buf, sizeof (buf), bench_yielder, NULL);
}

static void
mod_entry (void *data)
{
    long i, n = (long)data;

    for (i = 0; i < n; i++)
        hev_task_mod_fd (hev_task_self (), pipe_fds[0], POLLIN);
}

static void
bench_pipe_io (HevTaskIOYielder yielder, const char *name)
{
    HevTask *tasks[2];
    double t0, t1;

    bench_yielder = yielder;
    if (pipe (pipe_fds) < 0)
        return;
    fcntl (pipe_fds[0], F_SETFL, O_NONBLOCK);
    fcntl (pipe_fds[1], F_SETFL, O_NONBLOCK);

    tasks[0] = hev_task_new (-1);
    hev_task_add_fd (tasks[0], pipe_fds[0], POLLIN);
    hev_task_run (tasks[0], reader_entry, (void *)(long)NOPS);
    tasks[1] = hev_task_new (-1);
    hev_task_add_fd (tasks[1], pipe_fds[1], POLLOUT);
    hev_task_run (tasks[1], writer_entry, (void *)(long)NOPS);

    t0 = get_time ();
    hev_task_system_run ();
    t1 = get_time ();
    report (name, NOPS, t0, t1);

    hev_task_del_fd (tasks[0], pipe_fds[0]);
    hev_task_del_fd (tasks[1], pipe_fds[1]);
    close (pipe_fds[0]);
    close (pipe_fds[1]);
    hev_task_unref (tasks[0]);
    hev_task_unref (tasks[1]);
}

static void
bench_fd_mod (void)
{
    HevTask *task;
    double t0, t1;

    if (pipe (pipe_fds) < 0)
        return;
    fcntl (pipe_fds[0], F_SETFL, O_NONBLOCK);

    task = hev_task_new (-1);
    hev_task_add_fd (task, pipe_fds[0], POLLIN);
    hev_task_run (task, mod_entry, (void *)(long)NOPS);

    t0 = get_time ();
    hev_task_system_run ();
    t1 = get_time ();
    report ("fd mod (epoll_ctl)", NOPS, t0, t1);

    hev_task_del_fd (task, pipe_fds[0]);
    close (pipe_fds[0]);
    close (pipe_fds[1]);
    hev_task_unref (task);
}

int
main (void)
{
    if (hev_task_system_init () < 0)
        return 1;

    bench_pipe_io (NULL, "pipe io rw (64B)");
    bench_pipe_io (sleep_yielder, "pipe io rw (sleep)");
    bench_fd_mod ();

    hev_task_system_fini ();
    return 0;
}
