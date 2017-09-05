/*
 ============================================================================
 Name        : curl.c
 Author      : Heiher <r@hev.cc>
 Copyright   : Copyright (c) 2017 everyone.
 Description :
 ============================================================================
 */

#include <stdio.h>
#include <stdlib.h>

#include <hev-task.h>
#include <hev-task-system.h>

#include <curl/curl.h>

static CURLM *curl_handle;

static void
add_download (const char *url, int num)
{
	char filename[50];
	FILE *file;
	CURL *handle;

	snprintf (filename, 50, "%d.download", num);

	file = fopen (filename, "wb");
	if (!file) {
		fprintf (stderr, "Error opening %s\n", filename);
		return;
	}

	handle = curl_easy_init ();
	curl_easy_setopt (handle, CURLOPT_WRITEDATA, file);
	curl_easy_setopt (handle, CURLOPT_PRIVATE, file);
	curl_easy_setopt (handle, CURLOPT_URL, url);
	curl_multi_add_handle (curl_handle, handle);
	fprintf (stderr, "Added download %s -> %s\n", url, filename);
}

static void
check_multi_info (void)
{
	char *done_url;
	CURLMsg *message;
	int pending;
	CURL *easy_handle;
	FILE *file;

	while ((message = curl_multi_info_read (curl_handle, &pending))) {
		switch (message->msg) {
		case CURLMSG_DONE:
			easy_handle = message->easy_handle;

			curl_easy_getinfo (easy_handle, CURLINFO_EFFECTIVE_URL, &done_url);
			curl_easy_getinfo (easy_handle, CURLINFO_PRIVATE, &file);
			printf ("%s DONE\n", done_url);

			curl_multi_remove_handle (curl_handle, easy_handle);
			curl_easy_cleanup (easy_handle);
			if (file)
				fclose (file);
			break;

		default:
			fprintf (stderr, "CURLMSG default\n");
			break;
		}
	}
}

static void
task_socket_entry (void *data)
{
	int running_handles;

	curl_multi_perform (curl_handle, &running_handles);
	if (!running_handles)
		return;

	do {
		long timeout_ms;

		curl_multi_timeout (curl_handle, &timeout_ms);

		if (timeout_ms < 0) {
			hev_task_yield (HEV_TASK_WAITIO);
			curl_multi_perform (curl_handle, &running_handles);
		} else {
			hev_task_sleep (timeout_ms);
			curl_multi_socket_action (curl_handle, CURL_SOCKET_TIMEOUT,
						0, &running_handles);
		}

		check_multi_info ();
	} while (running_handles);
}

static int
handle_socket (CURL *easy, curl_socket_t s, int action, void *userp, void *socketp)
{
	HevTask *task = userp;
	int events = 0;

	switch (action) {
	case CURL_POLL_IN:
		events = EPOLLIN;
		goto poll_apply;
	case CURL_POLL_OUT:
		events = EPOLLOUT;
		goto poll_apply;
	case CURL_POLL_INOUT:
		events = EPOLLIN | EPOLLOUT;

poll_apply:
		if (socketp) {
			hev_task_mod_fd (task, s, events);
			break;
		}

		hev_task_add_fd (task, s, events);
		curl_multi_assign (curl_handle, s, task);
		break;
	case CURL_POLL_REMOVE:
		if (!socketp)
			break;

		hev_task_del_fd (task, s);
		curl_multi_assign (curl_handle, s, NULL);
		break;
	}

	return 0;
}

int
main (int argc, char **argv)
{
	HevTask *task;

	if (argc <= 1)
		return 0;

	hev_task_system_init ();

	if (curl_global_init (CURL_GLOBAL_ALL)) {
		fprintf (stderr, "Could not init curl\n");
		return 1;
	}

	curl_handle = curl_multi_init ();

	task = hev_task_new (-1);
	hev_task_run (task, task_socket_entry, NULL);

	curl_multi_setopt (curl_handle, CURLMOPT_SOCKETDATA, task);
	curl_multi_setopt (curl_handle, CURLMOPT_SOCKETFUNCTION, handle_socket);

	while (argc-- > 1)
		add_download (argv[argc], argc);

	hev_task_system_run ();

	curl_multi_cleanup (curl_handle);

	hev_task_system_fini ();

	return 0;
}

