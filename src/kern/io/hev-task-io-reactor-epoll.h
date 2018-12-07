/*
 ============================================================================
 Name        : hev-task-io-reactor-epoll.h
 Author      : Heiher <r@hev.cc>
 Copyright   : Copyright (c) 2018 everyone.
 Description : I/O Reactor EPoll
 ============================================================================
 */

#ifndef __HEV_TASK_IO_REACTOR_EPOLL_H__
#define __HEV_TASK_IO_REACTOR_EPOLL_H__

#include <sys/epoll.h>

typedef struct _HevTaskIOReactorSetupEvent HevTaskIOReactorSetupEvent;
typedef struct epoll_event HevTaskIOReactorWaitEvent;

enum _HevTaskIOReactorEvents
{
    HEV_TASK_IO_REACTOR_EV_RO = EPOLLIN,
    HEV_TASK_IO_REACTOR_EV_WO = EPOLLOUT,
    HEV_TASK_IO_REACTOR_EV_ER = EPOLLERR,
    HEV_TASK_IO_REACTOR_EV_RW = EPOLLIN | EPOLLOUT,
};

enum _HevTaskIOReactorOperation
{
    HEV_TASK_IO_REACTOR_OP_ADD = EPOLL_CTL_ADD,
    HEV_TASK_IO_REACTOR_OP_MOD = EPOLL_CTL_MOD,
    HEV_TASK_IO_REACTOR_OP_DEL = EPOLL_CTL_DEL,
};

struct _HevTaskIOReactorSetupEvent
{
    HevTaskIOReactorOperation op;

    int fd;
    struct epoll_event event;
};

static inline int
hev_task_io_reactor_get_oneshot (void)
{
    return 0;
}

static inline void
hev_task_io_reactor_setup_event_set (HevTaskIOReactorSetupEvent *event, int fd,
                                     HevTaskIOReactorOperation op,
                                     unsigned int events, void *data)
{
    event->op = op;
    event->fd = fd;
    event->event.events = events;
    event->event.data.ptr = data;
}

static inline unsigned int
hev_task_io_reactor_wait_event_get_events (HevTaskIOReactorWaitEvent *event)
{
    return event->events;
}

static inline void *
hev_task_io_reactor_wait_event_get_data (HevTaskIOReactorWaitEvent *event)
{
    return event->data.ptr;
}

#endif /* __HEV_TASK_IO_REACTOR_EPOLL_H__ */
