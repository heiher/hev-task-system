/*
 ============================================================================
 Name        : hev-task-io-reactor.c
 Author      : Heiher <r@hev.cc>
 Copyright   : Copyright (c) 2018 everyone.
 Description : I/O Reactor
 ============================================================================
 */

#include "hev-task-io-reactor.h"

int
hev_task_io_reactor_get_fd (HevTaskIOReactor *self)
{
    return self->fd;
}
