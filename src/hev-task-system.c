/*
 ============================================================================
 Name        : hev-task-system.c
 Author      : Heiher <r@hev.cc>
 Copyright   : Copyright (c) 2017 everyone.
 Description :
 ============================================================================
 */

#include <stdlib.h>
#include <unistd.h>
#include <sys/epoll.h>

#include "hev-task-system.h"
#include "hev-task-system-private.h"
#include "hev-memory-allocator-slice.h"

static int epoll_fd;

int
hev_task_system_init (void)
{
	HevMemoryAllocator *allocator;

	allocator = hev_memory_allocator_slice_new ();
	if (allocator) {
		allocator = hev_memory_allocator_set_default (allocator);
		if (allocator)
			hev_memory_allocator_unref (allocator);
	}

	epoll_fd = epoll_create (128);
	if (-1 == epoll_fd)
		return -1;

	return 0;
}

void
hev_task_system_fini (void)
{
	HevMemoryAllocator *allocator;

	close (epoll_fd);

	allocator = hev_memory_allocator_set_default (NULL);
	if (allocator)
		hev_memory_allocator_unref (allocator);
}

void
hev_task_system_run (void)
{
	hev_task_system_schedule (HEV_TASK_YIELD_COUNT, NULL);
}

int
hev_task_system_get_epoll_fd (void)
{
	return epoll_fd;
}

