/*
 ============================================================================
 Name        : hev-task-poll.c
 Author      : Heiher <r@hev.cc>
 Copyright   : Copyright (c) 2017 everyone.
 Description :
 ============================================================================
 */

#include "hev-task-poll.h"
#include "hev-task.h"

int
hev_task_poll (HevTaskPollFD fds[], unsigned int nfds, int timeout)
{
	HevTask *task = hev_task_self ();
	int i, ret;

	ret = poll (fds, nfds, 0);
	if ((ret > 0) || (timeout == 0))
		return ret;

	for (i=0; i<nfds; i++)
		hev_task_add_fd (task, fds[i].fd, fds[i].events);

	if (timeout > 0) {
retry_sleep:
		timeout = hev_task_sleep (timeout);
		ret = poll (fds, nfds, 0);
		if (timeout > 0 && ret == 0)
			goto retry_sleep;

		goto quit;
	}

retry:
	ret = poll (fds, nfds, 0);
	if (ret == 0) {
		hev_task_yield (HEV_TASK_WAITIO);
		goto retry;
	}

quit:
	for (i=0; i<nfds; i++)
		hev_task_del_fd (task, fds[i].fd);

	return ret;
}

