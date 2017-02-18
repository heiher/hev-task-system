/*
 ============================================================================
 Name        : hev-task.c
 Author      : Heiher <r@hev.cc>
 Copyright   : Copyright (c) 2017 everyone.
 Description :
 ============================================================================
 */

#include <stdlib.h>
#include <stdint.h>
#include <errno.h>
#include <unistd.h>
#include <sys/timerfd.h>

#include "hev-task.h"
#include "hev-task-private.h"
#include "hev-task-system-private.h"
#include "hev-memory-allocator.h"

#define HEV_TASK_STACK_SIZE	(64 * 1024)

#define ALIGN_DOWN(addr, align) \
	((addr) & ~((typeof (addr)) align - 1))

HevTask *
hev_task_new (int stack_size)
{
	HevTask *self;
	uintptr_t stack_addr;

	self = hev_malloc0 (sizeof (HevTask));
	if (!self)
		return NULL;

	self->timer_fd = -1;
	self->ref_count = 1;

	if (stack_size == -1) {
		stack_size = HEV_TASK_STACK_SIZE;
	}

	self->stack = hev_malloc0 (stack_size);
	if (!self->stack) {
		hev_free (self);
		return NULL;
	}

	stack_addr = (uintptr_t) (self->stack + stack_size);
	self->stack_top = (void *) ALIGN_DOWN (stack_addr, 16);
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

	if (self->timer_fd != -1)
		close (self->timer_fd);
	hev_free (self->stack);
	hev_free (self);
}

HevTask *
hev_task_self (void)
{
	return hev_task_system_get_current_task ();
}

HevTaskState
hev_task_get_state (HevTask *self)
{
	return self->state;
}

void
hev_task_set_priority (HevTask *self, int priority)
{
	if (priority < HEV_TASK_PRIORITY_MIN)
		priority = HEV_TASK_PRIORITY_MIN;
	else if (priority > HEV_TASK_PRIORITY_MAX)
		priority = HEV_TASK_PRIORITY_MAX;

	self->next_priority = priority;
}

int
hev_task_get_priority (HevTask *self)
{
	return self->next_priority;
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
hev_task_wakeup (HevTask *task)
{
	hev_task_system_wakeup_task (task);
}

void
hev_task_yield (HevTaskYieldType type)
{
	hev_task_system_schedule (type, NULL);
}

unsigned int
hev_task_sleep (unsigned int milliseconds)
{
	HevTask *self = hev_task_self ();
	struct itimerspec spec;
	ssize_t size;
	uint64_t time;

	if (self->timer_fd == -1) {
		self->timer_fd = timerfd_create (CLOCK_MONOTONIC, TFD_NONBLOCK);
		if (self->timer_fd == -1)
			return milliseconds;

		if (hev_task_add_fd (self, self->timer_fd, EPOLLIN) == -1)
			return milliseconds;
	}

	spec.it_interval.tv_sec = 0;
	spec.it_interval.tv_nsec = 0;
	spec.it_value.tv_sec = milliseconds / 1000;
	spec.it_value.tv_nsec = (milliseconds % 1000) * 1000 * 1000;
	if (timerfd_settime (self->timer_fd, 0, &spec, NULL) == -1)
		return milliseconds;

	size = read (self->timer_fd, &time, sizeof (time));
	if (size == -1) {
		if (errno == EAGAIN) {
			hev_task_yield (HEV_TASK_WAITIO);

			/* get the number of milliseconds left to sleep */
			if (timerfd_gettime (self->timer_fd, &spec) == -1)
				return milliseconds;
			if ((spec.it_value.tv_sec + spec.it_value.tv_nsec) == 0)
				return 0;
			return (spec.it_value.tv_sec * 1000) +
				(spec.it_value.tv_nsec / 1000 / 1000);
		}

		return milliseconds;
	}

	return 0;
}

void
hev_task_run (HevTask *self, HevTaskEntry entry, void *data)
{
	self->entry = entry;
	self->data = data;
	self->priority = self->next_priority;

	hev_task_system_schedule (HEV_TASK_YIELD, self);
}

