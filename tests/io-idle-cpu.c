/*
 ============================================================================
 Name        : io-idle-cpu.c
 Author      : Heiher <r@hev.cc>
 Copyright   : Copyright (c) 2026 everyone.
 Description : IO Idle CPU Test, no spurious wakeups or busy loop
 ============================================================================
 */

#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <stddef.h>
#include <string.h>
#include <assert.h>
#include <fcntl.h>

#include <hev-sys.h>
#include <hev-task.h>
#include <hev-task-system.h>
#include <hev-task-io.h>
#include <hev-task-io-socket.h>

#ifdef _WIN32
#include <windows.h>
#else
#include <time.h>
#endif

#define IDLE_MS (400)
#define MAX_WAKEUPS (3)
#define MAX_CPU_MS (150)

static double
cpu_ms (void)
{
#ifdef _WIN32
    FILETIME c, e, k, u;

    GetProcessTimes (GetCurrentProcess (), &c, &e, &k, &u);
    return (((double)((((unsigned long long)k.dwHighDateTime) << 32) |
                      k.dwLowDateTime)) +
            ((double)((((unsigned long long)u.dwHighDateTime) << 32) |
                      u.dwLowDateTime))) /
           10000.0;
#else
    struct timespec ts;

    clock_gettime (CLOCK_PROCESS_CPUTIME_ID, &ts);
    return ts.tv_sec * 1000.0 + ts.tv_nsec / 1000000.0;
#endif
}

/* Sleep for IDLE_MS, count how many times the task was woken up early. */
static int
idle (const char *name)
{
    unsigned int left = IDLE_MS;
    int wakeups = 0;
    double cpu = cpu_ms ();

    while (left) {
        left = hev_task_sleep (left);
        if (left)
            wakeups++;
    }

    cpu = cpu_ms () - cpu;
    if (wakeups > MAX_WAKEUPS || cpu >= MAX_CPU_MS) {
        fprintf (stderr, "%s: wakeups=%d cpu=%.1fms\n", name, wakeups, cpu);
        abort ();
    }

    return wakeups;
}

static void
make_pair (int fds[2])
{
    HevTask *task = hev_task_self ();

    assert (hev_task_io_socket_socketpair (PF_LOCAL, SOCK_STREAM, 0, fds) == 0);
    assert (hev_task_add_fd (task, fds[0], POLLIN | POLLOUT) == 0);
    assert (hev_task_add_fd (task, fds[1], POLLIN | POLLOUT) == 0);
    /* let the initial writable edge pass */
    hev_task_sleep (20);
}

static void
test_empty (void)
{
    int fds[2];
    char buf[16];

    make_pair (fds);
    idle ("empty socket");

    /* still works after the idle period */
    assert (write (fds[1], "x", 1) == 1);
    assert (hev_task_io_read (fds[0], buf, sizeof (buf), NULL, NULL) == 1);

    hev_task_io_close (fds[0]);
    hev_task_io_close (fds[1]);
}

static void
test_unread (void)
{
    int fds[2];
    char buf[16];

    make_pair (fds);
    assert (write (fds[1], "hello", 5) == 5);
    hev_task_sleep (20);
    /* data is pending and never read: edge-triggered, wake once at most */
    idle ("unread data");

    assert (hev_task_io_read (fds[0], buf, sizeof (buf), NULL, NULL) == 5);
    hev_task_io_close (fds[0]);
    hev_task_io_close (fds[1]);
}

static void
test_partial (void)
{
    int fds[2];
    char buf[16];

    make_pair (fds);
    assert (write (fds[1], "hello world", 11) == 11);
    hev_task_sleep (20);
    assert (read (fds[0], buf, 5) == 5);
    /* some data left behind a partial read */
    idle ("partially read data");
    assert (read (fds[0], buf, sizeof (buf)) == 6);

    hev_task_io_close (fds[0]);
    hev_task_io_close (fds[1]);
}

static void
test_closed (void)
{
    int fds[2];
    char buf[16];

    make_pair (fds);
    hev_task_io_close (fds[1]);
    hev_task_sleep (20);
    idle ("peer closed (EOF unread)");
    assert (read (fds[0], buf, sizeof (buf)) == 0);
    idle ("peer closed (EOF read)");

    hev_task_io_close (fds[0]);
}

static void
test_full (void)
{
    int fds[2];
    static char buf[65536];
    ssize_t s;

    make_pair (fds);
    memset (buf, 'a', sizeof (buf));
    for (;;) {
        s = write (fds[0], buf, sizeof (buf));
        if (s < 0)
            break;
    }
    assert (errno == EAGAIN);
    hev_task_sleep (20);
    idle ("send buffer full");

    /* drain, writable edge must come back */
    while (read (fds[1], buf, sizeof (buf)) > 0)
        ;
    s = hev_task_io_write (fds[0], buf, sizeof (buf), NULL, NULL);
    assert (s > 0);

    hev_task_io_close (fds[0]);
    hev_task_io_close (fds[1]);
}

static void
test_accept (void)
{
    struct sockaddr_in addr;
    int lfd, cfd, afd, one = 1;
    char buf[16];

    lfd = hev_task_io_socket_socket (AF_INET, SOCK_STREAM, 0);
    assert (lfd >= 0);
    memset (&addr, 0, sizeof (addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl (INADDR_LOOPBACK);
    addr.sin_port = htons (38211);
    assert (hev_task_io_socket_setsockopt (lfd, SOL_SOCKET, SO_REUSEADDR,
                                           (void *)&one, sizeof (one)) == 0);
    assert (hev_task_io_socket_bind (lfd, (struct sockaddr *)&addr,
                                     sizeof (addr)) == 0);
    assert (hev_task_io_socket_listen (lfd, 4) == 0);
    assert (hev_task_add_fd (hev_task_self (), lfd, POLLIN) == 0);
    hev_task_sleep (20);

    idle ("idle listener");

    cfd = hev_task_io_socket_socket (AF_INET, SOCK_STREAM, 0);
    assert (cfd >= 0);
    assert (hev_task_add_fd (hev_task_self (), cfd, POLLIN | POLLOUT) == 0);
    assert (hev_task_io_socket_connect (cfd, (struct sockaddr *)&addr,
                                        sizeof (addr), NULL, NULL) == 0);
    afd = hev_task_io_socket_accept (lfd, NULL, NULL, NULL, NULL);
    assert (afd >= 0);
    assert (hev_task_add_fd (hev_task_self (), afd, POLLIN | POLLOUT) == 0);
    hev_task_sleep (20);

    idle ("connected tcp, both idle");
    assert (write (cfd, "x", 1) == 1);
    assert (hev_task_io_read (afd, buf, sizeof (buf), NULL, NULL) == 1);

    hev_task_io_close (afd);
    hev_task_io_close (cfd);
    hev_task_io_close (lfd);
}

#ifdef _WIN32
static void
test_whandle (void)
{
    HANDLE ev = CreateEventA (NULL, TRUE, FALSE, NULL);

    assert (ev);
    assert (hev_task_add_whandle (hev_task_self (), ev) == 0);
    idle ("whandle, unsignaled");

    SetEvent (ev);
    idle ("whandle, stays signaled");

    hev_task_del_whandle (hev_task_self (), ev);
    CloseHandle (ev);
}
#endif

static void
task_entry (void *data)
{
    test_empty ();
    test_unread ();
    test_partial ();
    test_closed ();
    test_full ();
    test_accept ();
#ifdef _WIN32
    test_whandle ();
#endif
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
