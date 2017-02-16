/*
 ============================================================================
 Name        : echo-server.c
 Author      : Heiher <r@hev.cc>
 Copyright   : Copyright (c) 2017 everyone.
 Description :
 ============================================================================
 */

#include <stdio.h>
#include <errno.h>
#include <unistd.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include <hev-task.h>
#include <hev-task-system.h>

static int
task_socket_accept (int fd, struct sockaddr *restrict addr,
			socklen_t *restrict addr_len)
{
	int new_fd;
retry:
	new_fd = accept (fd, addr, addr_len);
	if (new_fd == -1 && errno == EAGAIN) {
		hev_task_yield (HEV_TASK_WAITIO);
		goto retry;
	}

	return new_fd;
}

static ssize_t
task_socket_recv (int fd, void *buf, size_t len, int flags)
{
	ssize_t size;

retry:
	size = recv (fd, buf, len, flags);
	if (size == -1 && errno == EAGAIN) {
		hev_task_yield (HEV_TASK_WAITIO);
		goto retry;
	}

	return size;
}

static ssize_t
task_socket_send (int fd, const void *buf, size_t len, int flags)
{
	ssize_t size;

retry:
	size = send (fd, buf, len, flags);
	if (size == -1 && errno == EAGAIN) {
		hev_task_yield (HEV_TASK_WAITIO);
		goto retry;
	}

	return size;
}

static void
task_client_entry (void *data)
{
	int fd = (intptr_t) data;
	char buf[2048];
	ssize_t size, s, c = 0;

	size = task_socket_recv (fd, buf, 2048, 0);
	if (size == -1) {
		printf ("Receive failed!\n");
		goto quit;
	}

retry:
	s = task_socket_send (fd, buf + c, size - c, 0);
	if (s == -1) {
		printf ("Send failed!\n");
		goto quit;
	} else {
		c += s;
		if (c < size)
			goto retry;
	}

quit:
	close (fd);
}

static void
task_listener_entry (void *data)
{
	HevTask *task = hev_task_self ();
	int fd, ret, nonblock = 1, reuseaddr = 1;
	struct sockaddr_in addr;

	fd = socket (AF_INET, SOCK_STREAM, 0);
	if (fd == -1) {
		fprintf (stderr, "Create socket failed!\n");
		return;
	}

	ret = setsockopt (fd, SOL_SOCKET, SO_REUSEADDR,
				&reuseaddr, sizeof (reuseaddr));
	if (ret == -1) {
		fprintf (stderr, "Set reuse address failed!\n");
		close (fd);
		return;
	}
	ret = ioctl (fd, FIONBIO, (char *) &nonblock);
	if (ret == -1) {
		fprintf (stderr, "Set non-blocking failed!\n");
		close (fd);
		return;
	}

	memset (&addr, 0, sizeof (addr));
	addr.sin_family = AF_INET;
	addr.sin_addr.s_addr = INADDR_ANY;
	addr.sin_port = htons (8000);
	ret = bind (fd, (struct sockaddr *) &addr, (socklen_t) sizeof (addr));
	if (ret == -1) {
		fprintf (stderr, "Bind address failed!\n");
		close (fd);
		return;
	}
	ret = listen (fd, 100);
	if (ret == -1) {
		fprintf (stderr, "Listen failed!\n");
		close (fd);
		return;
	}

	hev_task_add_fd (task, fd, EPOLLIN);

	while (1) {
		int client_fd;
		struct sockaddr *in_addr = (struct sockaddr *) &addr;
		socklen_t addr_len;

retry:
		addr_len = sizeof (addr);
		client_fd = task_socket_accept (fd, in_addr, &addr_len);
		if (-1 == client_fd) {
			printf ("Accept failed!\n");
			goto retry;
		}

		printf ("New client %d enter from %s:%u\n", client_fd,
					inet_ntoa (addr.sin_addr), ntohs (addr.sin_port));

		ret = ioctl (client_fd, FIONBIO, (char *) &nonblock);
		if (ret == -1) {
			fprintf (stderr, "Set non-blocking failed!\n");
			close (client_fd);
		}

		task = hev_task_new (-1);
		hev_task_set_priority (task, 1);
		hev_task_add_fd (task, client_fd, EPOLLIN | EPOLLOUT);
		hev_task_run (task, task_client_entry, (void *) (intptr_t) client_fd);
	}

	close (fd);
}

int
main (int argc, char *argv[])
{
	HevTask *task;

	hev_task_system_init ();

	task = hev_task_new (-1);
	hev_task_run (task, task_listener_entry, NULL);

	hev_task_system_run ();

	hev_task_system_fini ();

	return 0;
}

