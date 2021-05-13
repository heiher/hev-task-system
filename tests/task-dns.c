/*
 ============================================================================
 Name        : task-dns.c
 Author      : Heiher <r@hev.cc>
 Copyright   : Copyright (c) 2021 everyone.
 Description : Task DNS Test
 ============================================================================
 */

#include <stddef.h>
#include <assert.h>

#include <hev-task.h>
#include <hev-task-dns.h>
#include <hev-task-system.h>

static void
task1_entry (void *data)
{
    struct addrinfo hints = { 0 };
    struct addrinfo *result = NULL;
    ;

    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_DGRAM;
    hints.ai_flags = AI_PASSIVE;
    hints.ai_protocol = 0;

    assert (hev_task_dns_getaddrinfo (NULL, "80", &hints, &result) == 0);
    assert (result != NULL);
    freeaddrinfo (result);
}

int
main (int argc, char *argv[])
{
    HevTask *task;

    assert (hev_task_system_init () == 0);

    task = hev_task_new (-1);
    assert (task);
    hev_task_run (task, task1_entry, NULL);

    hev_task_system_run ();

    hev_task_system_fini ();

    return 0;
}
