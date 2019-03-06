/*
 ============================================================================
 Name        : hev-task-io-reactor-kqueue.h
 Author      : Heiher <r@hev.cc>
 Copyright   : Copyright (c) 2018 everyone.
 Description : I/O Reactor KQueue
 ============================================================================
 */

#ifndef __HEV_TASK_IO_REACTOR_KQUEUE_H__
#define __HEV_TASK_IO_REACTOR_KQUEUE_H__

#include <poll.h>
#include <sys/types.h>
#include <sys/event.h>
#include <sys/time.h>

#ifndef EVFILT_EXCEPT
#define EVFILT_EXCEPT (0)
#endif

#define HEV_TASK_IO_REACTOR_EVENT_GEN_MAX (3)

typedef struct _HevTaskIOReactorKQueue HevTaskIOReactorKQueue;
typedef struct kevent HevTaskIOReactorSetupEvent;
typedef struct kevent HevTaskIOReactorWaitEvent;

struct _HevTaskIOReactorKQueue
{
    HevTaskIOReactor base;
};

enum _HevTaskIOReactorEvents
{
    HEV_TASK_IO_REACTOR_EV_RO = EVFILT_READ,
    HEV_TASK_IO_REACTOR_EV_WO = EVFILT_WRITE,
    HEV_TASK_IO_REACTOR_EV_ER = EVFILT_EXCEPT,
};

enum _HevTaskIOReactorOperation
{
    HEV_TASK_IO_REACTOR_OP_ADD = EV_ADD,
    HEV_TASK_IO_REACTOR_OP_MOD = EV_ADD,
    HEV_TASK_IO_REACTOR_OP_DEL = EV_DELETE,
};

static inline void
hev_task_io_reactor_setup_event_set (HevTaskIOReactorSetupEvent *event, int fd,
                                     HevTaskIOReactorOperation op,
                                     unsigned int events, void *data)
{
    EV_SET (event, fd, events, op | EV_CLEAR, 0, 0, data);
}

static inline int
hev_task_io_reactor_setup_event_gen (HevTaskIOReactorSetupEvent *events, int fd,
                                     HevTaskIOReactorOperation op,
                                     unsigned int poll_events, void *data)
{
    int count = 0;

    if (poll_events & POLLIN)
        hev_task_io_reactor_setup_event_set (&events[count++], fd, op,
                                             HEV_TASK_IO_REACTOR_EV_RO, data);
    if (poll_events & POLLOUT)
        hev_task_io_reactor_setup_event_set (&events[count++], fd, op,
                                             HEV_TASK_IO_REACTOR_EV_WO, data);
    if (poll_events & POLLERR)
        hev_task_io_reactor_setup_event_set (&events[count++], fd, op,
                                             HEV_TASK_IO_REACTOR_EV_ER, data);

    return count;
}

static inline unsigned int
hev_task_io_reactor_wait_event_get_events (HevTaskIOReactorWaitEvent *event)
{
    return event->filter;
}

static inline void *
hev_task_io_reactor_wait_event_get_data (HevTaskIOReactorWaitEvent *event)
{
    return event->udata;
}

#endif /* __HEV_TASK_IO_REACTOR_KQUEUE_H__ */
