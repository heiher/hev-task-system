/*
 ============================================================================
 Name        : hev-task-io-reactor-iocp.c
 Author      : hev <r@hev.cc>
 Copyright   : Copyright (c) 2025 hev
 Description : I/O Reactor IOCP
 ============================================================================
 */

#if defined(__MSYS__) || (defined(_WIN32) && !defined(__CYGWIN__))

#include <assert.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdatomic.h>

#include <hev-memory-allocator.h>

#include "lib/misc/hev-compiler.h"

#include "kern/io/hev-task-io-reactor.h"

typedef struct _HevTaskIOReactorIOCPNode HevTaskIOReactorIOCPNode;

struct _HevTaskIOReactorIOCPNode
{
    HevRBTreeNode base;

    int ref_count;
    int events;
    int queued;

    uintptr_t handle;
    void *data;

    void *ihandle;
    void *ehandle;
    void *whandle;
};

static HevTaskIOReactorIOCPNode *
hev_task_io_reactor_iocp_node_new (void)
{
    HevTaskIOReactorIOCPNode *node;
    atomic_int *rcp;

    node = calloc (1, sizeof (HevTaskIOReactorIOCPNode));
    if (!node)
        return NULL;

    rcp = (atomic_int *)&node->ref_count;
    atomic_store_explicit (rcp, 1, memory_order_relaxed);

    return node;
}

static void
hev_task_io_reactor_iocp_node_ref (HevTaskIOReactorIOCPNode *node)
{
    atomic_int *rcp = (atomic_int *)&node->ref_count;

    atomic_fetch_add_explicit (rcp, 1, memory_order_relaxed);
}

static void
hev_task_io_reactor_iocp_node_unref (HevTaskIOReactorIOCPNode *node)
{
    atomic_int *rcp = (atomic_int *)&node->ref_count;
    int rc;

    rc = atomic_fetch_sub_explicit (rcp, 1, memory_order_relaxed);
    if (rc > 1)
        return;

    assert (rc >= 1);
    free (node);
}

static int
hev_task_io_reactor_iocp_node_detach (HevTaskIOReactorIOCP *self,
                                      HevTaskIOReactorIOCPNode *node)
{
    if (node->whandle) {
        if (!UnregisterWaitEx (node->whandle, INVALID_HANDLE_VALUE))
            return -1;
        node->whandle = NULL;
    }

    if (node->ehandle && node->handle != (uintptr_t)node->ehandle) {
        uintptr_t sock = hev_sys_handle_to_socket (node->handle);

        /* The owner may have closed the socket without deleting it. */
        if (sock != HEV_SYS_INVALID_SOCKET)
            WSAEventSelect (sock, NULL, 0);
        WSACloseEvent (node->ehandle);
    }
    node->ehandle = NULL;

    hev_rbtree_erase (&self->tree, &node->base);
    hev_task_io_reactor_iocp_node_unref (node);

    return 0;
}

static int
hev_task_io_reactor_iocp_insert (HevTaskIOReactorIOCP *self,
                                 HevTaskIOReactorIOCPNode *node)
{
    HevRBTreeNode **new = &self->tree.root, *parent = NULL;

    while (*new) {
        HevTaskIOReactorIOCPNode *this;

        this = container_of (*new, HevTaskIOReactorIOCPNode, base);
        parent = *new;

        if (this->handle < node->handle)
            new = &((*new)->left);
        else if (this->handle > node->handle)
            new = &((*new)->right);
        else
            return -1;
    }

    hev_rbtree_node_link (&node->base, parent, new);
    hev_rbtree_insert_color (&self->tree, &node->base);

    return 0;
}

static HevTaskIOReactorIOCPNode *
hev_task_io_reactor_iocp_lookup (HevTaskIOReactorIOCP *self, uintptr_t handle)
{
    HevRBTreeNode *node = self->tree.root;

    while (node) {
        HevTaskIOReactorIOCPNode *this;

        this = container_of (node, HevTaskIOReactorIOCPNode, base);

        if (this->handle < handle)
            node = node->left;
        else if (this->handle > handle)
            node = node->right;
        else
            return this;
    }

    return NULL;
}

static VOID CALLBACK
hev_task_io_reactor_iocp_handler (void *data, BOOLEAN fired)
{
    HevTaskIOReactorIOCPNode *node = data;
    int res;

    if (fired)
        return;

    if (node->handle != (uintptr_t)node->ehandle) {
        WSANETWORKEVENTS events;
        uintptr_t sock = hev_sys_handle_to_socket (node->handle);

        if (sock == HEV_SYS_INVALID_SOCKET) {
            ResetEvent (node->ehandle);
            return;
        }

        res = WSAEnumNetworkEvents (sock, node->ehandle, &events);
        if (res || !events.lNetworkEvents)
            return;
        atomic_fetch_or_explicit ((atomic_int *)&node->events,
                                  events.lNetworkEvents, memory_order_acq_rel);
    }

    /* One pending completion per node, further events merge into it. */
    if (atomic_exchange_explicit ((atomic_int *)&node->queued, 1,
                                  memory_order_acq_rel))
        return;

    hev_task_io_reactor_iocp_node_ref (node);
    res = PostQueuedCompletionStatus (node->ihandle, 1, 0, (LPOVERLAPPED)node);
    if (!res) {
        atomic_store_explicit ((atomic_int *)&node->queued, 0,
                               memory_order_release);
        hev_task_io_reactor_iocp_node_unref (node);
    }
}

/*
 * A socket event is reset by WSAEnumNetworkEvents, so its wait is persistent
 * and fires once per edge. A waitable handle can stay signaled (a process, a
 * manual-reset event), so its wait is one-shot: it fires once and stays idle
 * until the handle is added again, or it would spin the reactor.
 */
static BOOL
hev_task_io_reactor_iocp_register (HevTaskIOReactorIOCPNode *node)
{
    ULONG flags = WT_EXECUTEINPERSISTENTTHREAD;

    if (node->handle == (uintptr_t)node->ehandle)
        flags |= WT_EXECUTEONLYONCE;

    return RegisterWaitForSingleObject (&node->whandle, node->ehandle,
                                        hev_task_io_reactor_iocp_handler, node,
                                        INFINITE, flags);
}

static int
hev_task_io_reactor_iocp_rearm (HevTaskIOReactorIOCPNode *node,
                                HevTaskIOReactorSetupEvent *event)
{
    if (node->handle != (uintptr_t)node->ehandle || event->events)
        return -1;

    if (node->whandle) {
        if (!UnregisterWaitEx (node->whandle, INVALID_HANDLE_VALUE))
            return -1;
        node->whandle = NULL;
    }

    node->data = event->data;
    if (!hev_task_io_reactor_iocp_register (node))
        return -1;

    return 0;
}

static int
hev_task_io_reactor_iocp_add (HevTaskIOReactorIOCP *self,
                              HevTaskIOReactorSetupEvent *event)
{
    HevTaskIOReactorIOCPNode *node;
    int res;

    node = hev_task_io_reactor_iocp_lookup (self, event->handle);
    if (node)
        return hev_task_io_reactor_iocp_rearm (node, event);

    node = hev_task_io_reactor_iocp_node_new ();
    if (!node)
        goto exit;

    if (event->events) {
        uintptr_t sock = hev_sys_handle_to_socket (event->handle);
        long events;

        if (sock == HEV_SYS_INVALID_SOCKET)
            goto free_node;

        node->ehandle = CreateEventA (NULL, TRUE, FALSE, NULL);
        if (!node->ehandle)
            goto free_node;

        events = event->events | HEV_TASK_IO_REACTOR_EV_ER;
        res = WSAEventSelect (sock, node->ehandle, events);
        if (res)
            goto free_event;
    } else {
        node->ehandle = (void *)event->handle;
    }

    node->data = event->data;
    node->handle = event->handle;
    node->ihandle = self->base.handle;

    res = hev_task_io_reactor_iocp_register (node);
    if (!res)
        goto free_event;

    hev_task_io_reactor_iocp_insert (self, node);

    return 0;

free_event:
    if (event->handle != (uintptr_t)node->ehandle) {
        WSAEventSelect (hev_sys_handle_to_socket (event->handle), NULL, 0);
        WSACloseEvent (node->ehandle);
    }
free_node:
    hev_task_io_reactor_iocp_node_unref (node);
exit:
    return -1;
}

static int
hev_task_io_reactor_iocp_mod (HevTaskIOReactorIOCP *self,
                              HevTaskIOReactorSetupEvent *event)
{
    HevTaskIOReactorIOCPNode *node;

    node = hev_task_io_reactor_iocp_lookup (self, event->handle);
    if (!node)
        return -1;

    if (node->handle != (uintptr_t)node->ehandle && event->events) {
        uintptr_t sock = hev_sys_handle_to_socket (node->handle);
        long events = event->events | HEV_TASK_IO_REACTOR_EV_ER;
        int res;

        if (sock == HEV_SYS_INVALID_SOCKET)
            return -1;

        res = WSAEventSelect (sock, node->ehandle, events);
        if (res)
            return -1;
    }

    return 0;
}

static int
hev_task_io_reactor_iocp_del (HevTaskIOReactorIOCP *self,
                              HevTaskIOReactorSetupEvent *event)
{
    HevTaskIOReactorIOCPNode *node;

    node = hev_task_io_reactor_iocp_lookup (self, event->handle);
    if (!node)
        return -1;

    return hev_task_io_reactor_iocp_node_detach (self, node);
}

HevTaskIOReactor *
hev_task_io_reactor_new (void)
{
    HevTaskIOReactorIOCP *self;
    HANDLE handle;

    self = hev_malloc0 (sizeof (HevTaskIOReactorIOCP));
    if (!self)
        return NULL;

    handle = CreateIoCompletionPort (INVALID_HANDLE_VALUE, NULL, 0, 0);
    if (!handle) {
        hev_free (self);
        return NULL;
    }

    hev_sys_mutex_init (&self->mutex);
    self->base.handle = handle;

    return &self->base;
}

void
hev_task_io_reactor_destroy (HevTaskIOReactor *_self)
{
    HevTaskIOReactorIOCP *self = (HevTaskIOReactorIOCP *)_self;

    hev_sys_mutex_lock (&self->mutex);
    while (self->tree.root) {
        HevTaskIOReactorIOCPNode *node;

        node = container_of (self->tree.root, HevTaskIOReactorIOCPNode, base);
        if (hev_task_io_reactor_iocp_node_detach (self, node) < 0) {
            hev_sys_mutex_unlock (&self->mutex);
            return;
        }
    }
    hev_sys_mutex_unlock (&self->mutex);

    for (;;) {
        OVERLAPPED *overlap = NULL;
        ULONG_PTR key = 0;
        DWORD bytes = 0;
        BOOL ok;

        ok = GetQueuedCompletionStatus (_self->handle, &bytes, &key, &overlap,
                                        0);
        if (overlap)
            hev_task_io_reactor_iocp_node_unref (
                (HevTaskIOReactorIOCPNode *)overlap);
        else if (!ok)
            break;
    }

    CloseHandle (_self->handle);
    hev_sys_mutex_fini (&self->mutex);
    hev_free (self);
}

int
hev_task_io_reactor_setup (HevTaskIOReactor *_self,
                           HevTaskIOReactorSetupEvent *events, int count)
{
    HevTaskIOReactorIOCP *self = (HevTaskIOReactorIOCP *)_self;
    int res = -1;

    assert (count <= HEV_TASK_IO_REACTOR_EVENT_GEN_MAX);

    if (!events || count <= 0)
        return res;

    hev_sys_mutex_lock (&self->mutex);

    switch (events->op) {
    case HEV_TASK_IO_REACTOR_OP_ADD:
        res = hev_task_io_reactor_iocp_add (self, events);
        break;
    case HEV_TASK_IO_REACTOR_OP_MOD:
        res = hev_task_io_reactor_iocp_mod (self, events);
        break;
    case HEV_TASK_IO_REACTOR_OP_DEL:
        res = hev_task_io_reactor_iocp_del (self, events);
    default:
        break;
    }

    hev_sys_mutex_unlock (&self->mutex);

    return res;
}

int
hev_task_io_reactor_wait (HevTaskIOReactor *self,
                          HevTaskIOReactorWaitEvent *events, int count,
                          int timeout)
{
    HevTaskIOReactorIOCPNode *node;
    OVERLAPPED *overlap = NULL;
    ULONG_PTR key = 0;
    DWORD bytes = 0;
    int res;

    if (count <= 0)
        return -1;

retry:
    res = GetQueuedCompletionStatus (self->handle, &bytes, &key, &overlap,
                                     timeout);
    if (!res) {
        if (GetLastError () == WAIT_TIMEOUT)
            return 0;
        return -1;
    }

    node = (HevTaskIOReactorIOCPNode *)overlap;
    atomic_store_explicit ((atomic_int *)&node->queued, 0,
                           memory_order_release);
    events->events = atomic_exchange_explicit ((atomic_int *)&node->events, 0,
                                               memory_order_acq_rel);
    events->data = node->data;
    res = node->ehandle && node->whandle;
    /* A socket node without events was merged into an earlier wakeup. */
    if (node->handle != (uintptr_t)node->ehandle && !events->events)
        res = 0;
    hev_task_io_reactor_iocp_node_unref (node);

    if (!res)
        goto retry;

    return 1;
}

#endif /* defined(__MSYS__) || (defined(_WIN32) && !defined(__CYGWIN__)) */
