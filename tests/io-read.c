/*
 ============================================================================
 Name        : io-read.c
 Author      : Heiher <r@hev.cc>
 Copyright   : Copyright (c) 2018 everyone.
 Description : IO Read Test
 ============================================================================
 */

#include <stdlib.h>
#include <assert.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include <hev-task.h>
#include <hev-task-system.h>
#include <hev-task-io.h>

static void
task_entry (void *data)
{
    HevTask *task = hev_task_self ();
    int fd;
    char buf[4];
    ssize_t size;

    fd = hev_task_io_open ("/dev/random", O_RDONLY);
    assert (fd >= 0);

    assert (hev_task_add_fd (task, fd, POLLIN) == 0);

    size = hev_task_io_read (fd, buf, 4, NULL, NULL);
    assert (size >= 0);

    close (fd);
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
