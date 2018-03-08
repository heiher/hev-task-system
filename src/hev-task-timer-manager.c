/*
 ============================================================================
 Name        : hev-task_timer-manager.h
 Author      : Heiher <r@hev.cc>
 Copyright   : Copyright (c) 2018 everyone.
 Description :
 ============================================================================
 */

#include <fcntl.h>
#include <unistd.h>
#include <sys/timerfd.h>

#include "hev-task-timer-manager.h"
#include "hev-task-private.h"
#include "hev-task-system-private.h"
#include "hev-memory-allocator.h"

#define MAX_CACHED_TIMER_COUNT	CONFIG_TASK_TIMER_MAX_COUNT

struct _HevTaskTimer
{
	HevTaskTimer *next;

	HevTaskSchedEntity sched_entity;

	int fd;
};

struct _HevTaskTimerManager
{
	HevTaskTimer *cached_timers;

	unsigned int cached_count;
};

HevTaskTimerManager *
hev_task_timer_manager_new (void)
{
	HevTaskTimerManager *self;

	self = hev_malloc0 (sizeof (HevTaskTimerManager));
	if (!self)
		return NULL;

	return self;
}

void
hev_task_timer_manager_destroy (HevTaskTimerManager *self)
{
	HevTaskTimer *iter = self->cached_timers;

	while (iter) {
		HevTaskTimer *next = iter->next;
		close (iter->fd);
		hev_free (iter);
		iter = next;
	}

	hev_free (self);
}

HevTaskTimer *
hev_task_timer_manager_alloc (HevTaskTimerManager *self)
{
	HevTaskTimer *timer;
	int epoll_fd, fd, flags;
	struct epoll_event event;

	if (self->cached_timers) {
		timer = self->cached_timers;

		self->cached_timers = timer->next;
		self->cached_count --;

		return timer;
	}

	fd = timerfd_create (CLOCK_MONOTONIC, 0);
	if (fd == -1)
		return NULL;

	if (fcntl (fd, F_SETFL, O_NONBLOCK) == -1)
		return NULL;

	flags = fcntl (fd, F_GETFD);
	if (flags == -1)
		return NULL;

	flags |= FD_CLOEXEC;
	if (fcntl (fd, F_SETFD, flags) == -1)
		return NULL;
retry:
	timer = hev_malloc0 (sizeof (HevTaskTimer));
	if (!timer) {
		hev_task_yield (HEV_TASK_YIELD);
		goto retry;
	}

	epoll_fd = hev_task_system_get_context ()->epoll_fd;
	event.events = EPOLLET | EPOLLIN;
	event.data.ptr = &timer->sched_entity;
	if (epoll_ctl (epoll_fd, EPOLL_CTL_ADD, fd, &event) == -1) {
		close (fd);
		hev_free (timer);
		return NULL;
	}

	timer->fd = fd;

	return timer;
}

void
hev_task_timer_manager_free (HevTaskTimerManager *self, HevTaskTimer *timer)
{
	if (self->cached_count >= MAX_CACHED_TIMER_COUNT) {
		close (timer->fd);
		hev_free (timer);
		return;
	}

	timer->next = self->cached_timers;
	self->cached_timers = timer;
	self->cached_count ++;
}

int
hev_task_timer_get_fd (HevTaskTimer *timer)
{
	return timer->fd;
}

