/*
 ============================================================================
 Name        : hev-task-aide.c
 Author      : hev <r@hev.cc>
 Copyright   : Copyright (c) 2022 - 2026 hev
 Description : Aide
 ============================================================================
 */

#include <stdlib.h>
#include <errno.h>

#include "hev-sys-private.h"
#include <hev-task.h>
#include <hev-task-io.h>
#include <hev-task-io-socket.h>
#include <hev-memory-allocator.h>
#include "kern/task/hev-task-private.h"
#include "kern/io/hev-task-io-reactor.h"
#include "mem/simple/hev-memory-allocator-simple.h"

#include "hev-task-aide.h"

struct _HevTaskAide
{
    HevSysThread thread;
    HevTaskIOReactor *reactor;
    HevMemoryAllocator *allocator;
    HevTaskAideWork ctrl_work;
    int ctrl_fd;
    unsigned int quit;
    unsigned int orphan;
    unsigned int ref_count;
};

/* The aide is shared by all task systems in the process, it is created by
 * the first user and destroyed by the last one, which may run on different
 * threads. So its memory comes from the system heap instead of the per-thread
 * default allocator.
 */
static HevTaskAide *aide;
static HevSysOnce once = HEV_SYS_ONCE_INITIALIZER;
static HevSysMutex mutex = HEV_SYS_MUTEX_INITIALIZER;

static void
hev_task_aide_quit_handler (unsigned int revents, void *data)
{
    HevTaskAide *self = data;

    for (;;) {
        char buf[64];
        int res;

        res = read (self->ctrl_work.fd, buf, sizeof (buf));
        if (0 > res) {
            if (EINTR == errno)
                continue;
            break;
        }
        if (0 == res)
            break;

        self->quit = 1;
    }
}

static void *
hev_task_aide_entry (void *data)
{
    HevTaskAide *self = data;

    while (!self->quit) {
        HevTaskIOReactorWaitEvent events[256];
        int i, count;

        count = hev_task_io_reactor_wait (self->reactor, events, 256, -1);

        for (i = 0; i < count; i++) {
            HevTaskAideWork *work;
            unsigned int revents;

            work = hev_task_io_reactor_wait_event_get_data (&events[i]);
            revents = hev_task_io_reactor_wait_event_get_events (&events[i]);

            work->handler (revents, work->data);
        }
    }

    return NULL;
}

static HevTaskIOReactor *
hev_task_aide_reactor_new (HevTaskAide *self)
{
    HevMemoryAllocator *allocator;
    HevTaskIOReactor *reactor;

    allocator = hev_memory_allocator_set_default (self->allocator);
    reactor = hev_task_io_reactor_new ();
    hev_memory_allocator_set_default (allocator);

    return reactor;
}

static void
hev_task_aide_reactor_destroy (HevTaskAide *self)
{
    HevMemoryAllocator *allocator;

    allocator = hev_memory_allocator_set_default (self->allocator);
    hev_task_io_reactor_destroy (self->reactor);
    hev_memory_allocator_set_default (allocator);
}

static HevTaskAide *
hev_task_aide_new (void)
{
    HevTaskIOReactorSetupEvent revents[HEV_TASK_IO_REACTOR_EVENT_GEN_MAX];
    HevTaskAide *self;
    int fds[2];
    int count;
    int res;

    res = hev_task_io_socket_socketpair (PF_LOCAL, SOCK_STREAM, 0, fds);
    if (0 > res)
        return NULL;

    if ((0 > fcntl (fds[0], F_SETFD, FD_CLOEXEC)) ||
        (0 > fcntl (fds[1], F_SETFD, FD_CLOEXEC)))
        goto close;

    self = calloc (1, sizeof (HevTaskAide));
    if (!self)
        goto close;

    self->allocator = hev_memory_allocator_simple_new ();
    if (!self->allocator)
        goto free;

    self->reactor = hev_task_aide_reactor_new (self);
    if (!self->reactor)
        goto free_allocator;

    self->ctrl_work.fd = fds[0];
    self->ctrl_work.events = POLLIN;
    self->ctrl_work.handler = hev_task_aide_quit_handler;
    self->ctrl_work.data = self;
    self->ctrl_fd = fds[1];

    count = hev_task_io_reactor_setup_event_fd_gen (
        revents, fds[0], HEV_TASK_IO_REACTOR_OP_ADD, POLLIN, &self->ctrl_work);
    res = hev_task_io_reactor_setup (self->reactor, revents, count);
    if (0 > res)
        goto free_reactor;

    res = hev_sys_thread_create (&self->thread, hev_task_aide_entry, self);
    if (res != 0)
        goto free_reactor;

    return self;

free_reactor:
    hev_task_aide_reactor_destroy (self);
free_allocator:
    hev_memory_allocator_unref (self->allocator);
free:
    free (self);
close:
    close (fds[0]);
    close (fds[1]);

    return NULL;
}

static void
hev_task_aide_destroy (HevTaskAide *self)
{
    struct pollfd pfd;
    char quit = 'q';
    int res;

    /* An orphan has no thread to stop, and its sockets are shared with the
     * aide of the parent process, so only release the local resources.
     */
    for (; !self->orphan;) {
        res = write (self->ctrl_fd, &quit, sizeof (quit));
        if (0 <= res)
            break;
        if (EINTR == errno)
            continue;
        if (EAGAIN != errno)
            break;

        pfd.fd = self->ctrl_fd;
        pfd.events = POLLOUT;
        poll (&pfd, 1, -1);
    }

    if (!self->orphan)
        hev_sys_thread_join (self->thread);

    close (self->ctrl_fd);
    close (self->ctrl_work.fd);
    hev_task_aide_reactor_destroy (self);
    hev_memory_allocator_unref (self->allocator);
    free (self);
}

static void
hev_task_aide_fork_prepare (void)
{
    hev_sys_mutex_lock (&mutex);
}

static void
hev_task_aide_fork_parent (void)
{
    hev_sys_mutex_unlock (&mutex);
}

static void
hev_task_aide_fork_child (void)
{
    /* The aide thread is not inherited by the child, the aide becomes an
     * orphan, which is released by its remaining users, and the next user
     * creates a new one.
     */
    if (aide) {
        aide->orphan = 1;
        aide = NULL;
    }

    hev_sys_mutex_unlock (&mutex);
}

static void
hev_task_aide_once (void)
{
    hev_sys_atfork (hev_task_aide_fork_prepare, hev_task_aide_fork_parent,
                    hev_task_aide_fork_child);
}

HevTaskAide *
hev_task_aide_get (void)
{
    HevTaskAide *self;

    hev_sys_once (&once, hev_task_aide_once);

    hev_sys_mutex_lock (&mutex);
    if (!aide)
        aide = hev_task_aide_new ();
    self = aide;
    if (self)
        self->ref_count++;
    hev_sys_mutex_unlock (&mutex);

    return self;
}

void
hev_task_aide_put (HevTaskAide *self)
{
    unsigned int ref_count;

    /* Detach the last reference under the lock and stop the aide outside,
     * so that the join does not block other users, a new user creates a
     * new aide meanwhile.
     */
    hev_sys_mutex_lock (&mutex);
    ref_count = --self->ref_count;
    if ((0 == ref_count) && (aide == self))
        aide = NULL;
    hev_sys_mutex_unlock (&mutex);

    if (0 == ref_count)
        hev_task_aide_destroy (self);
}

int
hev_task_aide_is_orphan (HevTaskAide *self)
{
    return self->orphan;
}

int
hev_task_aide_add (HevTaskAide *self, HevTaskAideWork *work)
{
    HevTaskIOReactorSetupEvent revents[HEV_TASK_IO_REACTOR_EVENT_GEN_MAX];
    int count;

    count = hev_task_io_reactor_setup_event_fd_gen (
        revents, work->fd, HEV_TASK_IO_REACTOR_OP_ADD, work->events, work);

    return hev_task_io_reactor_setup (self->reactor, revents, count);
}

int
hev_task_aide_del (HevTaskAide *self, HevTaskAideWork *work)
{
    HevTaskIOReactorSetupEvent revents[HEV_TASK_IO_REACTOR_EVENT_GEN_MAX];
    int count;

    /* The reactor of an orphan is shared with the parent process, deleting
     * the work would remove it from the aide of the parent.
     */
    if (self->orphan)
        return 0;

    count = hev_task_io_reactor_setup_event_fd_gen (
        revents, work->fd, HEV_TASK_IO_REACTOR_OP_DEL, 0, NULL);

    return hev_task_io_reactor_setup (self->reactor, revents, count);
}
