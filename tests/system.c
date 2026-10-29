/*
 ============================================================================
 Name        : system.c
 Author      : Heiher <r@hev.cc>
 Copyright   : Copyright (c) 2018 - 2026 everyone.
 Description : System Test
 ============================================================================
 */

#include <stdio.h>
#include <fcntl.h>
#include <assert.h>

#include <hev-task.h>
#include <hev-task-io.h>
#include <hev-task-system.h>

#define LOOP_COUNT (100)
#define MAX_FD (1024)

static int
count_fds (void)
{
    int fd, count = 0;

    for (fd = 0; fd < MAX_FD; fd++)
        if (hev_task_io_fcntl (fd, F_GETFD) >= 0)
            count++;

    return count;
}

static int
count_maps (void)
{
#if defined(__linux__)
    FILE *fp;
    int c, count = 0;

    fp = fopen ("/proc/self/maps", "r");
    if (!fp)
        return 0;

    while ((c = fgetc (fp)) != EOF)
        if (c == '\n')
            count++;
    fclose (fp);

    return count;
#else
    return 0;
#endif
}

static void
task_entry (void *data)
{
    hev_task_yield (HEV_TASK_YIELD);
}

static void
test_init_fini (void)
{
    assert (hev_task_system_init () == 0);
    assert (hev_task_system_init () == -1);

    hev_task_system_run ();

    hev_task_system_fini ();
    hev_task_system_fini ();
}

static void
test_unfinished_tasks (void)
{
    HevTask *task;

    assert (hev_task_system_init () == 0);

    /* never run */
    task = hev_task_new (-1);
    assert (task);

    /* finished, but still referenced */
    task = hev_task_new (-1);
    assert (task);
    hev_task_ref (task);
    hev_task_run (task, task_entry, NULL);

    hev_task_system_run ();

    assert (hev_task_get_state (task) == HEV_TASK_STOPPED);

    hev_task_system_fini ();
}

int
main (int argc, char *argv[])
{
    int i, fds, maps;

    test_init_fini ();
    test_unfinished_tasks ();

    fds = count_fds ();
    maps = count_maps ();

    for (i = 0; i < LOOP_COUNT; i++) {
        test_init_fini ();
        test_unfinished_tasks ();
    }

    assert (count_fds () == fds);
    assert (count_maps () == maps);

    return 0;
}
