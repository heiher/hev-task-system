/*
 ============================================================================
 Name        : hev-task-io-pipe.c
 Author      : hev <r@hev.cc>
 Copyright   : Copyright (c) 2018 - 2026 hev
 Description : Task pipe I/O operations
 ============================================================================
 */

#include "sys/hev-sys-private.h"
#include <hev-task.h>
#include <hev-task-io.h>
#include <hev-task-io-pipe.h>

#include "lib/misc/hev-compiler.h"

EXPORT_SYMBOL int
hev_task_io_pipe_pipe (int pipefd[2])
{
    int nonblock = 1;

    if (0 > pipe (pipefd))
        return -1;

    if (0 > ioctl (pipefd[0], FIONBIO, (char *)&nonblock)) {
        hev_task_io_close (pipefd[0]);
        hev_task_io_close (pipefd[1]);
        return -2;
    }

    if (0 > ioctl (pipefd[1], FIONBIO, (char *)&nonblock)) {
        hev_task_io_close (pipefd[0]);
        hev_task_io_close (pipefd[1]);
        return -3;
    }

    return 0;
}
