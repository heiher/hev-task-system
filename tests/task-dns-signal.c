/*
 ============================================================================
 Name        : task-dns-signal.c
 Author      : Heiher <r@hev.cc>
 Copyright   : Copyright (c) 2026 everyone.
 Description : Task DNS Signal Test
 ============================================================================
 */

#include <assert.h>
#include <signal.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <sys/time.h>

#include <hev-sys.h>
#include <hev-task.h>
#include <hev-task-dns.h>
#include <hev-task-system.h>

#define TASK_COUNT (8)
#define CALL_COUNT (1000)

static void
signal_handler (int signo)
{
}

static void
getaddrinfo_entry (void *data)
{
    int base = (intptr_t)data;
    int i;

    /* Every reply must belong to its own call, even when interrupted. */
    for (i = 0; i < CALL_COUNT; i++) {
        struct addrinfo hints = { 0 };
        struct addrinfo *result = NULL;
        struct sockaddr_in *addr;
        char service[16];
        int port;

        port = base + i;
        snprintf (service, sizeof (service), "%d", port);

        hints.ai_family = AF_INET;
        hints.ai_socktype = SOCK_STREAM;
        hints.ai_flags = AI_NUMERICHOST | AI_NUMERICSERV;

        assert (hev_task_dns_getaddrinfo ("127.0.0.1", service, &hints,
                                          &result) == 0);
        assert (result != NULL);
        addr = (struct sockaddr_in *)result->ai_addr;
        assert (addr->sin_port == htons (port));
        freeaddrinfo (result);
    }
}

int
main (int argc, char *argv[])
{
    struct itimerval timer = { { 0, 100 }, { 0, 100 } };
    struct sigaction sa = { 0 };
    int i;

    /* Interrupt system calls without restarting them. */
    sa.sa_handler = signal_handler;
    sigemptyset (&sa.sa_mask);
    assert (sigaction (SIGALRM, &sa, NULL) == 0);
    assert (setitimer (ITIMER_REAL, &timer, NULL) == 0);

    assert (hev_task_system_init () == 0);

    for (i = 0; i < TASK_COUNT; i++) {
        HevTask *task = hev_task_new (-1);
        assert (task);
        hev_task_run (task, getaddrinfo_entry,
                      (void *)(intptr_t)(1024 + i * CALL_COUNT));
    }

    hev_task_system_run ();

    hev_task_system_fini ();

    memset (&timer, 0, sizeof (timer));
    assert (setitimer (ITIMER_REAL, &timer, NULL) == 0);

    return 0;
}
