/*
 ============================================================================
 Name        : hev-task-system.c
 Author      : Heiher <r@hev.cc>
 Copyright   : Copyright (c) 2017 everyone.
 Description :
 ============================================================================
 */

#include <fcntl.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/epoll.h>

#ifdef ENABLE_PTHREAD
# include <pthread.h>
#endif

#include "hev-task-system.h"
#include "hev-task-system-private.h"
#include "hev-memory-allocator-slice.h"

#ifdef ENABLE_PTHREAD
static pthread_key_t key;
static pthread_once_t key_once = PTHREAD_ONCE_INIT;

static void pthread_key_creator (void);
#else
static HevTaskSystemContext *default_context;
#endif

int
hev_task_system_init (void)
{
	HevTask *task_nodes;
	const int priority_min = HEV_TASK_PRIORITY_MIN;
	const int priority_max = HEV_TASK_PRIORITY_MAX;
	int i, flags;

#ifdef ENABLE_MEMALLOC_SLICE
	HevMemoryAllocator *allocator;
#endif
#ifdef ENABLE_PTHREAD
	HevTaskSystemContext *default_context;
#endif

#ifdef ENABLE_MEMALLOC_SLICE
	allocator = hev_memory_allocator_slice_new ();
	if (allocator) {
		allocator = hev_memory_allocator_set_default (allocator);
		if (allocator)
			hev_memory_allocator_unref (allocator);
	}
#endif

#ifdef ENABLE_PTHREAD
	pthread_once (&key_once, pthread_key_creator);
	default_context = pthread_getspecific (key);
#endif

	if (default_context)
		return -1;

	default_context = hev_malloc0 (sizeof (HevTaskSystemContext));
	if (!default_context)
		return -2;

#ifdef ENABLE_PTHREAD
	pthread_setspecific (key, default_context);
#endif

	default_context->epoll_fd = epoll_create (128);
	if (-1 == default_context->epoll_fd)
		return -3;

	flags = fcntl (default_context->epoll_fd, F_GETFD);
	if (-1 == flags)
		return -4;

	flags |= FD_CLOEXEC;
	if (-1 == fcntl (default_context->epoll_fd, F_SETFD, flags))
		return -5;

	/*
         * .--> task_nodes[0] <--> task_nodes[1] <--.
         * |                                        |
         * \--> task_nodes[N] <--> task_nodes[2] <--/
	 */
	task_nodes = default_context->task_nodes;
	task_nodes[priority_min].prev = &task_nodes[priority_max];
	task_nodes[priority_min].next = &task_nodes[priority_min + 1];

	for (i=priority_min+1; i<priority_max; i++) {
		task_nodes[i].prev = &task_nodes[i - 1];
		task_nodes[i].next = &task_nodes[i + 1];
	}

	task_nodes[priority_max].prev = &task_nodes[priority_max - 1];
	task_nodes[priority_max].next = &task_nodes[priority_min];

	return 0;
}

void
hev_task_system_fini (void)
{
#ifdef ENABLE_MEMALLOC_SLICE
	HevMemoryAllocator *allocator;
#endif
#ifdef ENABLE_PTHREAD
	HevTaskSystemContext *default_context = pthread_getspecific (key);
#endif

	close (default_context->epoll_fd);
	hev_free (default_context);

#ifdef ENABLE_PTHREAD
	pthread_setspecific (key, NULL);
#else
	default_context = NULL;
#endif

#ifdef ENABLE_MEMALLOC_SLICE
	allocator = hev_memory_allocator_set_default (NULL);
	if (allocator)
		hev_memory_allocator_unref (allocator);
#endif
}

void
hev_task_system_run (void)
{
	hev_task_system_schedule (HEV_TASK_RUN_SCHEDULER, NULL);
}

HevTaskSystemContext *
hev_task_system_get_context (void)
{
#ifdef ENABLE_PTHREAD
	return pthread_getspecific (key);
#else
	return default_context;
#endif
}

#ifdef ENABLE_PTHREAD
static void
pthread_key_creator (void)
{
	pthread_key_create (&key, NULL);
}
#endif

