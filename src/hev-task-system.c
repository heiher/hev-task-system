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

static HevTaskSystemContext *default_context;

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

	if (default_context)
		return -1;

	default_context = hev_malloc0 (sizeof (HevTaskSystemContext));
	if (!default_context)
		return -2;

	default_context->epoll_fd = epoll_create (128);
	if (-1 == default_context->epoll_fd)
		return -3;

	return 0;
}

void
hev_task_system_fini (void)
{
	HevMemoryAllocator *allocator;

	close (default_context->epoll_fd);
	hev_free (default_context);
	default_context = NULL;

	allocator = hev_memory_allocator_set_default (NULL);
	if (allocator)
		hev_memory_allocator_unref (allocator);
}

void
hev_task_system_run (void)
{
	hev_task_system_schedule (HEV_TASK_YIELD_COUNT, NULL);
}

HevTaskSystemContext *
hev_task_system_get_context (void)
{
	return default_context;
}

