/*
 ============================================================================
 Name        : hev-task-io-reactor-wsa.h
 Author      : hev <r@hev.cc>
 Copyright   : Copyright (c) 2025 hev
 Description : I/O Reactor WSA
 ============================================================================
 */

#ifndef __HEV_TASK_IO_REACTOR_WSA_H__
#define __HEV_TASK_IO_REACTOR_WSA_H__

#include <io.h>
#include <poll.h>
#include <pthread.h>

#include "lib/rbtree/hev-rbtree.h"
#include "lib/misc/hev-windows-api.h"

#define HEV_TASK_IO_REACTOR_EVENT_GEN_MAX (1)

typedef struct _HevTaskIOReactorWSA HevTaskIOReactorWSA;
typedef struct _HevTaskIOReactorSetupEvent HevTaskIOReactorSetupEvent;
typedef struct _HevTaskIOReactorWaitEvent HevTaskIOReactorWaitEvent;

struct _HevTaskIOReactorWSA
{
    HevTaskIOReactor base;

    HevRBTree tree;
    pthread_mutex_t mutex;
};

enum _HevTaskIOReactorEvents
{
    HEV_TASK_IO_REACTOR_EV_RO = FD_READ | FD_ACCEPT,
    HEV_TASK_IO_REACTOR_EV_WO = FD_WRITE | FD_CONNECT,
    HEV_TASK_IO_REACTOR_EV_ER = FD_CLOSE,
};

enum _HevTaskIOReactorOperation
{
    HEV_TASK_IO_REACTOR_OP_ADD,
    HEV_TASK_IO_REACTOR_OP_MOD,
    HEV_TASK_IO_REACTOR_OP_DEL,
};

struct _HevTaskIOReactorSetupEvent
{
    HevTaskIOReactorOperation op;

    long handle;
    int events;
    void *data;
};

struct _HevTaskIOReactorWaitEvent
{
    int events;
    void *data;
};

int hev_task_io_reactor_wait (HevTaskIOReactor *self,
                              HevTaskIOReactorWaitEvent *events, int count,
                              int timeout);

static inline void
hev_task_io_reactor_setup_event_set (HevTaskIOReactorSetupEvent *event, int fd,
                                     HevTaskIOReactorOperation op,
                                     unsigned int events, void *data)
{
    event->op = op;
    event->handle = get_osfhandle (fd);
    event->events = events;
    event->data = data;
}

static inline int
hev_task_io_reactor_setup_event_gen (HevTaskIOReactorSetupEvent *events, int fd,
                                     HevTaskIOReactorOperation op,
                                     unsigned int poll_events, void *data)
{
    HevTaskIOReactorEvents reactor_events = 0;

    if (poll_events & POLLIN)
        reactor_events |= HEV_TASK_IO_REACTOR_EV_RO;
    if (poll_events & POLLOUT)
        reactor_events |= HEV_TASK_IO_REACTOR_EV_WO;
    if (poll_events & POLLERR)
        reactor_events |= HEV_TASK_IO_REACTOR_EV_ER;

    hev_task_io_reactor_setup_event_set (events, fd, op, reactor_events, data);

    return 1;
}

static inline unsigned int
hev_task_io_reactor_wait_event_get_events (HevTaskIOReactorWaitEvent *event)
{
    unsigned int events = 0;

    if (event->events & HEV_TASK_IO_REACTOR_EV_RO)
        events |= POLLIN;
    if (event->events & HEV_TASK_IO_REACTOR_EV_WO)
        events |= POLLOUT;
    if (event->events & HEV_TASK_IO_REACTOR_EV_ER)
        events |= POLLERR;

    return events;
}

static inline void *
hev_task_io_reactor_wait_event_get_data (HevTaskIOReactorWaitEvent *event)
{
    return event->data;
}

#endif /* __HEV_TASK_IO_REACTOR_WSA_H__ */
