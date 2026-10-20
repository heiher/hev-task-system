/*
 ============================================================================
 Name        : hev-task-io-reactor-iocp.h
 Author      : hev <r@hev.cc>
 Copyright   : Copyright (c) 2025 hev
 Description : I/O Reactor IOCP
 ============================================================================
 */

#ifndef __HEV_TASK_IO_REACTOR_IOCP_H__
#define __HEV_TASK_IO_REACTOR_IOCP_H__

#include <stdint.h>

#if defined(_MSC_VER)
#include <winsock2.h>
#include <ws2tcpip.h>
#include <windows.h>
/* The upstream API is int-fd-based; the native Win32 host must supply an
 * explicit translation to pointer-sized SOCKET values.  A cast is NOT safe. */
SOCKET hev_win_fd_to_socket (int fd);
#else
#include <io.h>
#include <poll.h>
#include <pthread.h>
#include "lib/misc/hev-windows-api.h"
#endif

#include "lib/rbtree/hev-rbtree.h"

#define HEV_TASK_IO_REACTOR_EVENT_GEN_MAX (1)

typedef struct _HevTaskIOReactorIOCP HevTaskIOReactorIOCP;
typedef struct _HevTaskIOReactorSetupEvent HevTaskIOReactorSetupEvent;
typedef struct _HevTaskIOReactorWaitEvent HevTaskIOReactorWaitEvent;

struct _HevTaskIOReactorIOCP
{
    HevTaskIOReactor base;

    HevRBTree tree;
#if defined(_MSC_VER)
    SRWLOCK mutex;
#else
    pthread_mutex_t mutex;
#endif
};

typedef enum _HevTaskIOReactorEvents
{
    HEV_TASK_IO_REACTOR_EV_RO = FD_READ | FD_ACCEPT,
    HEV_TASK_IO_REACTOR_EV_WO = FD_WRITE | FD_CONNECT,
    HEV_TASK_IO_REACTOR_EV_ER = FD_CLOSE,
} HevTaskIOReactorEvents;

typedef enum _HevTaskIOReactorOperation
{
    HEV_TASK_IO_REACTOR_OP_ADD,
    HEV_TASK_IO_REACTOR_OP_MOD,
    HEV_TASK_IO_REACTOR_OP_DEL,
} HevTaskIOReactorOperation;

/* handle values are pointer-sized: a WinSock SOCKET or a Win32 HANDLE */
struct _HevTaskIOReactorSetupEvent
{
    HevTaskIOReactorOperation op;

#if defined(_MSC_VER)
    uintptr_t handle;
#else
    long handle;
#endif
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
hev_task_io_reactor_setup_event_set (HevTaskIOReactorSetupEvent *event,
#if defined(_MSC_VER)
                                     uintptr_t handle,
#else
                                     long handle,
#endif
                                     HevTaskIOReactorOperation op,
                                     unsigned int events, void *data)
{
    event->op = op;
    event->handle = handle;
    event->events = events;
    event->data = data;
}

static inline int
hev_task_io_reactor_setup_event_fd_gen (HevTaskIOReactorSetupEvent *events,
#if defined(_MSC_VER)
                                        int fd, HevTaskIOReactorOperation op,
#else
                                        long fd, HevTaskIOReactorOperation op,
#endif
                                        unsigned int poll_events, void *data)
{
    HevTaskIOReactorEvents reactor_events = 0;
#if defined(_MSC_VER)
    uintptr_t handle;
#else
    long handle;
#endif

    if (poll_events & POLLIN)
        reactor_events |= HEV_TASK_IO_REACTOR_EV_RO;
    if (poll_events & POLLOUT)
        reactor_events |= HEV_TASK_IO_REACTOR_EV_WO;
    if (poll_events & POLLERR)
        reactor_events |= HEV_TASK_IO_REACTOR_EV_ER;

#ifdef _MSC_VER
    {
        SOCKET socket = hev_win_fd_to_socket (fd);

        if (socket == INVALID_SOCKET)
            return 0;
        handle = (uintptr_t)socket;
    }
#else
    handle = get_osfhandle (fd);
#endif

    hev_task_io_reactor_setup_event_set (events, handle, op, reactor_events,
                                         data);

    return 1;
}

static inline int
hev_task_io_reactor_setup_event_whandle_gen (HevTaskIOReactorSetupEvent *events,
                                             void *handle,
                                             HevTaskIOReactorOperation op,
                                             void *data)
{
#if defined(_MSC_VER)
    hev_task_io_reactor_setup_event_set (events, (uintptr_t)handle, op, 0,
                                         data);
#else
    hev_task_io_reactor_setup_event_set (events, (long)handle, op, 0, data);
#endif

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

#endif /* __HEV_TASK_IO_REACTOR_IOCP_H__ */
