/*
 ============================================================================
 Name        : hev-task.c
 Author      : Heiher <r@hev.cc>
 Copyright   : Copyright (c) 2017 everyone.
 Description :
 ============================================================================
 */

#include <stdlib.h>
#include <unistd.h>
#include <sys/mman.h>

#include "hev-task.h"
#include "hev-task-private.h"
#include "hev-task-system-private.h"
#include "hev-memory-allocator.h"

#define HEV_TASK_STACK_SIZE	(512 * 1024)

HevTask *
hev_task_new (int stack_size)
{
	HevTask *self;

	self = hev_malloc0 (sizeof (HevTask));
	if (!self)
		return NULL;

	self->ref_count = 1;

	if (stack_size == -1) {
		stack_size = HEV_TASK_STACK_SIZE;
	} else {
		static long page_size;

		if (!page_size)
			page_size = sysconf (_SC_PAGESIZE);

		stack_size = (stack_size + page_size - 1) & ~(page_size - 1);
	}

	self->stack = mmap (NULL, stack_size,
				PROT_READ | PROT_WRITE,
				MAP_PRIVATE | MAP_ANONYMOUS,
				-1, 0);
	if (self->stack == MAP_FAILED) {
		hev_free (self);
		return NULL;
	}
	self->stack += stack_size;
	self->stack_size = stack_size;

	return self;
}

HevTask *
hev_task_ref (HevTask *self)
{
	self->ref_count ++;

	return self;
}

void
hev_task_unref (HevTask *self)
{
	self->ref_count --;
	if (self->ref_count)
		return;

	munmap (self->stack - self->stack_size, self->stack_size);
	hev_free (self);
}

void
hev_task_set_priority (HevTask *self, int priority)
{
	if (priority < HEV_TASK_PRIORITY_MIN)
		priority = HEV_TASK_PRIORITY_MIN;
	else if (priority > HEV_TASK_PRIORITY_MAX)
		priority = HEV_TASK_PRIORITY_MAX;

	self->priority = priority;
}

int
hev_task_get_priority (HevTask *self)
{
	return self->priority;
}

int
hev_task_add_fd (HevTask *self, int fd, unsigned int events)
{
	int epoll_fd;
	struct epoll_event event;

	epoll_fd = hev_task_system_get_epoll_fd ();

	event.events = EPOLLET | events;
	event.data.ptr = self;
	return epoll_ctl (epoll_fd, EPOLL_CTL_ADD, fd, &event);
}

int
hev_task_mod_fd (HevTask *self, int fd, unsigned int events)
{
	int epoll_fd;
	struct epoll_event event;

	epoll_fd = hev_task_system_get_epoll_fd ();

	event.events = EPOLLET | events;
	event.data.ptr = self;
	return epoll_ctl (epoll_fd, EPOLL_CTL_MOD, fd, &event);
}

int
hev_task_del_fd (HevTask *self, int fd)
{
	int epoll_fd;

	epoll_fd = hev_task_system_get_epoll_fd ();

	return epoll_ctl (epoll_fd, EPOLL_CTL_DEL, fd, NULL);
}

void
hev_task_run (HevTask *self, HevTaskEntry entry, void *data)
{
	self->entry = entry;
	self->data = data;

	hev_task_system_schedule (HEV_TASK_SYSTEM_YIELD, self);
}

