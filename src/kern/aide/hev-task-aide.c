/*
 ============================================================================
 Name        : hev-task-aide.c
 Author      : Heiher <r@hev.cc>
 Copyright   : Copyright (c) 2022 - 2024 everyone.
 Description : Aide
 ============================================================================
 */

#include <poll.h>
#include <errno.h>

#if defined(_MSC_VER) && defined(_WIN32)
#include <winsock2.h>
#include <windows.h>
#include <process.h>
#else
#include <unistd.h>
#include <pthread.h>
#endif

#include "kern/task/hev-task-private.h"
#include "kern/io/hev-task-io-reactor.h"
#include "lib/io/basic/hev-task-io.h"

#include "hev-task-aide.h"

#if defined(_MSC_VER) && defined(_WIN32)

static SRWLOCK mutex = SRWLOCK_INIT;
static HevTaskIOReactor *reactor;
static HANDLE thread;
static HANDLE wake_event;
static volatile LONG exit_flag;
static HevTaskAideWork wake_work;

static void
hev_task_aide_wake_handler (unsigned int revents, void *data)
{
    (void)revents;
    (void)data;
}

static unsigned int __stdcall
hev_task_aide_entry (void *data)
{
    (void)data;

    for (;;) {
        HevTaskIOReactorWaitEvent events[256];
        int i, count;

        count = hev_task_io_reactor_wait (reactor, events, 256, -1);

        for (i = 0; i < count; i++) {
            HevTaskAideWork *work;
            unsigned int revents;

            work = hev_task_io_reactor_wait_event_get_data (&events[i]);
            revents = hev_task_io_reactor_wait_event_get_events (&events[i]);

            work->handler (revents, work->data);
        }

        if (exit_flag)
            break;
    }

    return 0;
}

int
hev_task_aide_init (void)
{
    int res = 0;
    HevTaskIOReactorSetupEvent revents[HEV_TASK_IO_REACTOR_EVENT_GEN_MAX];
    int count;

    AcquireSRWLockExclusive (&mutex);

    if (reactor)
        goto exit;

    reactor = hev_task_io_reactor_new ();
    if (!reactor) {
        res = -1;
        goto exit;
    }

    wake_event = CreateEventA (NULL, FALSE, FALSE, NULL);
    if (!wake_event) {
        hev_task_io_reactor_destroy (reactor);
        reactor = NULL;
        res = -1;
        goto exit;
    }

    /* register the wake event in the aide reactor so that signaling it
     * wakes the waiting loop */
    wake_work.handler = hev_task_aide_wake_handler;
    wake_work.data = NULL;
    count = hev_task_io_reactor_setup_event_whandle_gen (revents, wake_event,
                                                         HEV_TASK_IO_REACTOR_OP_ADD,
                                                         &wake_work);
    if (hev_task_io_reactor_setup (reactor, revents, count) < 0) {
        CloseHandle (wake_event);
        wake_event = NULL;
        hev_task_io_reactor_destroy (reactor);
        reactor = NULL;
        res = -1;
        goto exit;
    }

    exit_flag = 0;
    thread = (HANDLE)_beginthreadex (NULL, 0, hev_task_aide_entry, NULL, 0,
                                     NULL);
    if (!thread) {
        CloseHandle (wake_event);
        wake_event = NULL;
        hev_task_io_reactor_destroy (reactor);
        reactor = NULL;
        res = -1;
        goto exit;
    }

exit:
    ReleaseSRWLockExclusive (&mutex);

    return res;
}

int
hev_task_aide_add (HevTaskAideWork *work)
{
    HevTaskIOReactorSetupEvent revents[HEV_TASK_IO_REACTOR_EVENT_GEN_MAX];
    int count;

    count = hev_task_io_reactor_setup_event_fd_gen (
        revents, work->fd, HEV_TASK_IO_REACTOR_OP_ADD, work->events, work);

    return hev_task_io_reactor_setup (reactor, revents, count);
}

int
hev_task_aide_del (HevTaskAideWork *work)
{
    HevTaskIOReactorSetupEvent revents[HEV_TASK_IO_REACTOR_EVENT_GEN_MAX];
    int count;

    count = hev_task_io_reactor_setup_event_fd_gen (
        revents, work->fd, HEV_TASK_IO_REACTOR_OP_DEL, 0, NULL);

    return hev_task_io_reactor_setup (reactor, revents, count);
}

void
hev_task_aide_fini (void)
{
    HANDLE t;

    AcquireSRWLockExclusive (&mutex);

    if (!reactor) {
        ReleaseSRWLockExclusive (&mutex);
        return;
    }

    t = thread;
    thread = NULL;

    ReleaseSRWLockExclusive (&mutex);

    if (t) {
        InterlockedExchange (&exit_flag, 1);
        SetEvent (wake_event);
        WaitForSingleObject (t, INFINITE);
        CloseHandle (t);
    }

    AcquireSRWLockExclusive (&mutex);
    if (wake_event) {
        HevTaskIOReactorSetupEvent revents[HEV_TASK_IO_REACTOR_EVENT_GEN_MAX];
        int count;

        count = hev_task_io_reactor_setup_event_whandle_gen (
            revents, wake_event, HEV_TASK_IO_REACTOR_OP_DEL, NULL);
        (void)hev_task_io_reactor_setup (reactor, revents, count);
        CloseHandle (wake_event);
        wake_event = NULL;
    }
    hev_task_io_reactor_destroy (reactor);
    reactor = NULL;
    ReleaseSRWLockExclusive (&mutex);
}

#else /* POSIX pthread */

static pthread_t thread;
static pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
static HevTaskIOReactor *reactor;

static void *
hev_task_aide_entry (void *data)
{
    for (;;) {
        HevTaskIOReactorWaitEvent events[256];
        int i, count;

        count = hev_task_io_reactor_wait (reactor, events, 256, -1);

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

int
hev_task_aide_init (void)
{
    int res = 0;

    if (reactor)
        return 0;

    pthread_mutex_lock (&mutex);
    if (!reactor) {
        reactor = hev_task_io_reactor_new ();
        if (!reactor) {
            res = -1;
        } else {
            res = pthread_create (&thread, NULL, hev_task_aide_entry, NULL);
            if (res != 0) {
                hev_task_io_reactor_destroy (reactor);
                reactor = NULL;
                res = -1;
            }
        }
    }
    pthread_mutex_unlock (&mutex);

    return res;
}

int
hev_task_aide_add (HevTaskAideWork *work)
{
    HevTaskIOReactorSetupEvent revents[HEV_TASK_IO_REACTOR_EVENT_GEN_MAX];
    int count;

    count = hev_task_io_reactor_setup_event_fd_gen (
        revents, work->fd, HEV_TASK_IO_REACTOR_OP_ADD, work->events, work);

    return hev_task_io_reactor_setup (reactor, revents, count);
}

int
hev_task_aide_del (HevTaskAideWork *work)
{
    HevTaskIOReactorSetupEvent revents[HEV_TASK_IO_REACTOR_EVENT_GEN_MAX];
    int count;

    count = hev_task_io_reactor_setup_event_fd_gen (
        revents, work->fd, HEV_TASK_IO_REACTOR_OP_DEL, 0, NULL);

    return hev_task_io_reactor_setup (reactor, revents, count);
}

void
hev_task_aide_fini (void)
{
    /* no explicit lifecycle on POSIX platforms: pthread peers keep the
     * existing upstream behavior */
}

#endif /* _WIN32 */
