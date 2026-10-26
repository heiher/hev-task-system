/*
 ============================================================================
 Name        : task-dns-fork.c
 Author      : Heiher <r@hev.cc>
 Copyright   : Copyright (c) 2026 everyone.
 Description : Task DNS Fork Test
 ============================================================================
 */

#include <assert.h>
#include <stddef.h>
#include <string.h>
#include <pthread.h>
#include <sys/wait.h>

#include <hev-sys.h>
#include <hev-task.h>
#include <hev-task-dns.h>
#include <hev-task-system.h>

#define FORK_COUNT (50)

static volatile int quit;

static void
getaddrinfo_entry (void *data)
{
    struct addrinfo hints = { 0 };
    struct addrinfo *result = NULL;
    struct sockaddr_in *addr;

    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_NUMERICHOST | AI_NUMERICSERV;

    assert (hev_task_dns_getaddrinfo ("127.0.0.1", "80", &hints, &result) == 0);
    assert (result != NULL);
    addr = (struct sockaddr_in *)result->ai_addr;
    assert (addr->sin_port == htons (80));
    freeaddrinfo (result);
}

static void
resolve (void)
{
    int i;

    for (i = 0; i < 4; i++) {
        HevTask *task = hev_task_new (-1);
        assert (task);
        hev_task_run (task, getaddrinfo_entry, NULL);
    }

    hev_task_system_run ();
}

static void *
busy_entry (void *data)
{
    while (!quit) {
        assert (hev_task_system_init () == 0);
        resolve ();
        hev_task_system_fini ();
    }

    return NULL;
}

int
main (int argc, char *argv[])
{
    pthread_t thread;
    int i;

    /* Keep the shared aide busy and restarting while forking. */
    assert (pthread_create (&thread, NULL, busy_entry, NULL) == 0);

    assert (hev_task_system_init () == 0);
    resolve ();

    for (i = 0; i < FORK_COUNT; i++) {
        pid_t pid;
        int status;

        pid = fork ();
        assert (pid >= 0);
        if (pid == 0) {
            /* The inherited aide must be released without disturbing the
             * parent, and a new one serves the child.
             */
            hev_task_system_fini ();
            assert (hev_task_system_init () == 0);
            resolve ();
            hev_task_system_fini ();
            _exit (0);
        }

        assert (waitpid (pid, &status, 0) == pid);
        assert (WIFEXITED (status) && (WEXITSTATUS (status) == 0));

        /* The aide of the parent still serves. */
        resolve ();
    }

    hev_task_system_fini ();

    quit = 1;
    pthread_join (thread, NULL);

    return 0;
}
