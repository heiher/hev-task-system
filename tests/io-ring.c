/*
 ============================================================================
 Name        : io-ring.c
 Author      : Heiher <r@hev.cc>
 Copyright   : Copyright (c) 2025 everyone.
 Description : IO Ring Test
 ============================================================================
 */

#include <stdlib.h>
#include <assert.h>
#include <unistd.h>
#include <string.h>
#include <netinet/in.h>
#include <sys/socket.h>

#include <hev-task.h>
#include <hev-task-system.h>
#include <hev-task-io.h>
#include <hev-task-io-socket.h>
#include <hev-task-io-ring.h>

static int fds[2];

static void
task_udp_entry (void *data)
{
    struct sockaddr_in addr, raddr;
    socklen_t alen = sizeof (addr);
    HevTask *task = hev_task_self ();
    char buf[16];
    ssize_t s;
    int fd, sfd;

    fd = hev_task_io_socket_socket (PF_INET, SOCK_DGRAM, 0);
    assert (fd >= 0);
    assert (hev_task_add_fd (task, fd, POLLIN) == 0);

    sfd = hev_task_io_socket_socket (PF_INET, SOCK_DGRAM, 0);
    assert (sfd >= 0);

    memset (&addr, 0, sizeof (addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl (INADDR_LOOPBACK);
    assert (bind (fd, (struct sockaddr *)&addr, sizeof (addr)) == 0);
    assert (getsockname (fd, (struct sockaddr *)&addr, &alen) == 0);

    /* sendto/recvfrom round trip with address reporting */
    s = hev_task_io_socket_sendto (sfd, "udp", 3, 0,
                                   (struct sockaddr *)&addr, sizeof (addr),
                                   NULL, NULL);
    assert (s == 3);

    memset (&raddr, 0, sizeof (raddr));
    alen = sizeof (raddr);
    s = hev_task_io_socket_recvfrom (fd, buf, sizeof (buf), 0,
                                     (struct sockaddr *)&raddr, &alen,
                                     NULL, NULL);
    assert (s == 3);
    assert (memcmp (buf, "udp", 3) == 0);
    assert (alen == sizeof (raddr));
    assert (raddr.sin_family == AF_INET);

    close (sfd);
    close (fd);
}

static int
task_io_yielder (HevTaskYieldType type, void *data)
{
    assert (type == HEV_TASK_WAITIO);

    return -1;
}

static void
task_entry (void *data)
{
    HevTask *task;
    char buf[16];
    ssize_t s;

    assert (hev_task_io_ring_is_enabled () != 0);

    /* interrupt an in-flight read; the pending op must be cancelled */
    s = hev_task_io_read (fds[0], buf, sizeof (buf), task_io_yielder, NULL);
    assert (s == -2);

    /* a read after the peer write completes with data */
    assert (write (fds[1], "ping", 4) == 4);
    s = hev_task_io_read (fds[0], buf, sizeof (buf), NULL, NULL);
    assert (s == 4);
    assert (memcmp (buf, "ping", 4) == 0);

    /* MSG_WAITALL across short writes */
    assert (write (fds[1], "abcdefgh", 8) == 8);
    assert (write (fds[1], "ijkl", 4) == 4);

    task = hev_task_new (-1);
    assert (task);
    hev_task_run (task, task_udp_entry, NULL);

    s = hev_task_io_socket_recv (fds[0], buf, 8, MSG_WAITALL, NULL, NULL);
    assert (s == 8);
    assert (memcmp (buf, "abcdefgh", 8) == 0);

    close (fds[0]);
    close (fds[1]);
}

int
main (int argc, char *argv[])
{
    HevTask *task;

#if !defined(ENABLE_IO_RING)
    return 0;
#endif

    assert (hev_task_io_socket_socketpair (PF_LOCAL, SOCK_STREAM, 0, fds) == 0);

    assert (hev_task_system_init () == 0);

    task = hev_task_new (-1);
    assert (task);
    hev_task_run (task, task_entry, NULL);

    hev_task_system_run ();

    hev_task_system_fini ();

    close (fds[0]);
    close (fds[1]);

    return 0;
}
