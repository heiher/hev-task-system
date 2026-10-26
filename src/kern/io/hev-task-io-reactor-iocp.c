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
#include <stdlib.h>
#include <stdatomic.h>

#include "lib/misc/hev-compiler.h"
#include "mem/api/hev-memory-allocator-api.h"

#include "kern/io/hev-task-io-reactor.h"

typedef struct _HevTaskIOReactorIOCPNode HevTaskIOReactorIOCPNode;

struct _HevTaskIOReactorIOCPNode
{
    HevRBTreeNode base;

    int ref_count;
    int events;

    HevIOCPHandle handle;
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

#if defined(HEV_IOCP_NATIVE)
static int
hev_task_io_reactor_iocp_node_detach (HevTaskIOReactorIOCP *self,
                                      HevTaskIOReactorIOCPNode *node)
{
    if (node->whandle) {
        if (!UnregisterWaitEx (node->whandle, INVALID_HANDLE_VALUE))
            return -1;
        node->whandle = NULL;
    }

    if (node->ehandle && node->handle != (HevIOCPHandle)node->ehandle) {
        WSAEventSelect (node->handle, NULL, 0);
        WSACloseEvent (node->ehandle);
    }
    node->ehandle = NULL;

    hev_rbtree_erase (&self->tree, &node->base);
    hev_task_io_reactor_iocp_node_unref (node);

    return 0;
}
#endif

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
hev_task_io_reactor_iocp_lookup (HevTaskIOReactorIOCP *self,
                              HevIOCPHandle handle)
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

    if (node->handle != (HevIOCPHandle)node->ehandle) {
        WSANETWORKEVENTS events;

        res = WSAEnumNetworkEvents (node->handle, node->ehandle, &events);
        if (res || !events.lNetworkEvents)
            return;
        node->events = events.lNetworkEvents;
    }

    hev_task_io_reactor_iocp_node_ref (node);
    res = PostQueuedCompletionStatus (node->ihandle, 1, 0, (LPOVERLAPPED)node);
    if (!res)
        hev_task_io_reactor_iocp_node_unref (node);
}

static int
hev_task_io_reactor_iocp_add (HevTaskIOReactorIOCP *self,
                              HevTaskIOReactorSetupEvent *event)
{
    HevTaskIOReactorIOCPNode *node;
    int res;

    node = hev_task_io_reactor_iocp_lookup (self, event->handle);
    if (node)
        goto exit;

    node = hev_task_io_reactor_iocp_node_new ();
    if (!node)
        goto exit;

    if (event->events) {
        long events;

#if defined(HEV_IOCP_NATIVE)
        node->ehandle = CreateEventA (NULL, TRUE, FALSE, NULL);
#else
        node->ehandle = CreateEventA (NULL, FALSE, FALSE, NULL);
#endif
        if (!node->ehandle)
            goto free_node;

        events = event->events | HEV_TASK_IO_REACTOR_EV_ER;
        res = WSAEventSelect (event->handle, node->ehandle, events);
        if (res)
            goto free_event;
    } else {
        node->ehandle = (void *)event->handle;
    }

    node->data = event->data;
    node->handle = event->handle;
    node->ihandle = self->base.handle;

    res = RegisterWaitForSingleObject (&node->whandle, node->ehandle,
                                       hev_task_io_reactor_iocp_handler, node,
                                       INFINITE, WT_EXECUTEINPERSISTENTTHREAD);
    if (!res)
        goto free_event;

    hev_task_io_reactor_iocp_insert (self, node);

    return 0;

free_event:
#if defined(HEV_IOCP_NATIVE)
    if (event->handle != (HevIOCPHandle)node->ehandle) {
        WSAEventSelect (event->handle, NULL, 0);
        WSACloseEvent (node->ehandle);
    }
#else
    if (event->handle != (HevIOCPHandle)node->ehandle)
        WSACloseEvent (node->ehandle);
#endif
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

    if (node->handle != (HevIOCPHandle)node->ehandle && event->events) {
        long events = event->events | HEV_TASK_IO_REACTOR_EV_ER;
        int res = WSAEventSelect (node->handle, node->ehandle, events);
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
#if !defined(HEV_IOCP_NATIVE)
    int res;
#endif

    node = hev_task_io_reactor_iocp_lookup (self, event->handle);
    if (!node)
        return -1;

#if defined(HEV_IOCP_NATIVE)
    return hev_task_io_reactor_iocp_node_detach (self, node);
#else
    res = UnregisterWaitEx (node->whandle, INVALID_HANDLE_VALUE);
    if (node->handle != (HevIOCPHandle)node->ehandle)
        res &= WSACloseEvent (node->ehandle);
    if (!res)
        return -1;

    node->ehandle = NULL;
    node->whandle = NULL;
    hev_rbtree_erase (&self->tree, &node->base);
    hev_task_io_reactor_iocp_node_unref (node);

    return 0;
#endif
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

    HEV_MUTEX_INIT (&self->mutex);
    self->base.handle = handle;

    return &self->base;
}

void
hev_task_io_reactor_destroy (HevTaskIOReactor *_self)
{
    HevTaskIOReactorIOCP *self = (HevTaskIOReactorIOCP *)_self;

#if defined(HEV_IOCP_NATIVE)
    HEV_MUTEX_LOCK (&self->mutex);
    while (self->tree.root) {
        HevTaskIOReactorIOCPNode *node;

        node = container_of (self->tree.root, HevTaskIOReactorIOCPNode, base);
        if (hev_task_io_reactor_iocp_node_detach (self, node) < 0) {
            HEV_MUTEX_UNLOCK (&self->mutex);
            return;
        }
    }
    HEV_MUTEX_UNLOCK (&self->mutex);

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
#endif

    CloseHandle (_self->handle);
    HEV_MUTEX_FINI (&self->mutex);
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

    HEV_MUTEX_LOCK (&self->mutex);

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

    HEV_MUTEX_UNLOCK (&self->mutex);

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
    events->events = node->events;
    events->data = node->data;
    res = node->ehandle && node->whandle;
    hev_task_io_reactor_iocp_node_unref (node);

    if (!res)
        goto retry;

    return 1;
}

#endif /* defined(__MSYS__) || (defined(_WIN32) && !defined(__CYGWIN__)) */
