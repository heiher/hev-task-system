/*
 ============================================================================
 Name        : timeout.c
 Author      : Heiher <r@hev.cc>
 Copyright   : Copyright (c) 2017 everyone.
 Description :
 ============================================================================
 */

#include <stdio.h>
#include <stdint.h>
#include <errno.h>
#include <unistd.h>
#include <sys/timerfd.h>

#include <hev-task.h>
#include <hev-task-system.h>

static void
task_entry (void *data)
{
	HevTask *task = hev_task_self ();
	int fd;
	struct itimerspec spec;
	const unsigned int interval = 1000;
	ssize_t size;
	uint64_t time;

	fd = timerfd_create (CLOCK_MONOTONIC, TFD_NONBLOCK);
	if (fd == -1) {
		fprintf (stderr, "Create timer fd failed!\n");
		return;
	}

	hev_task_add_fd (task, fd, EPOLLIN);

	spec.it_interval.tv_sec = 0;
	spec.it_interval.tv_nsec = 0;
	spec.it_value.tv_sec = interval / 1000;
	spec.it_value.tv_nsec = (interval % 1000) * 1000 * 1000;
	timerfd_settime (fd, 0, &spec, NULL);

	printf ("waitting for timeout %ums ...\n", interval);
retry:
	size = read (fd, &time, sizeof (time));
	if (size == -1) {
		if (errno == EAGAIN) {
			hev_task_system_yield (HEV_TASK_SYSTEM_WAITIO);
			goto retry;
		}

		close (fd);
		fprintf (stderr, "Read timer fd failed!\n");
		return;
	}

	printf ("timeout\n");

	close (fd);
}

int
main (int argc, char *argv[])
{
	HevTask *task;

	hev_task_system_init ();

	task = hev_task_new (-1);
	hev_task_run (task, task_entry, NULL);

	hev_task_system_run ();

	hev_task_system_fini ();

	return 0;
}

