/*
 ============================================================================
 Name        : hev-task-io-reactor-wsa.c
 Author      : hev <r@hev.cc>
 Copyright   : Copyright (c) 2025 hev
 Description : I/O Reactor WSA
 ============================================================================
 */

#if defined(__MSYS__)

#include <assert.h>
#include <stdlib.h>
#include <stdatomic.h>

#include "lib/misc/hev-compiler.h"
#include "mem/api/hev-memory-allocator-api.h"

#include "kern/io/hev-task-io-reactor.h"

typedef struct _HevTaskIOReactorWSANode HevTaskIOReactorWSANode;

struct _HevTaskIOReactorWSANode
{
    HevRBTreeNode base;

    int ref_count;
    int events;

    long handle;
    void *data;

    void *ihandle;
    void *ehandle;
    void *whandle;
};

static HevTaskIOReactorWSANode *
hev_task_io_reactor_wsa_node_new (void)
{
    HevTaskIOReactorWSANode *node;
    atomic_int *rcp;

    node = calloc (1, sizeof (HevTaskIOReactorWSANode));
    if (!node)
        return NULL;

    rcp = (atomic_int *)&node->ref_count;
    atomic_store_explicit (rcp, 1, memory_order_relaxed);

    return node;
}

static void
hev_task_io_reactor_wsa_node_ref (HevTaskIOReactorWSANode *node)
{
    atomic_int *rcp = (atomic_int *)&node->ref_count;

    atomic_fetch_add_explicit (rcp, 1, memory_order_relaxed);
}

static void
hev_task_io_reactor_wsa_node_unref (HevTaskIOReactorWSANode *node)
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
hev_task_io_reactor_wsa_insert (HevTaskIOReactorWSA *self,
                                HevTaskIOReactorWSANode *node)
{
    HevRBTreeNode **new = &self->tree.root, *parent = NULL;

    while (*new) {
        HevTaskIOReactorWSANode *this;

        this = container_of (*new, HevTaskIOReactorWSANode, base);
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

static HevTaskIOReactorWSANode *
hev_task_io_reactor_wsa_lookup (HevTaskIOReactorWSA *self, long handle)
{
    HevRBTreeNode *node = self->tree.root;

    while (node) {
        HevTaskIOReactorWSANode *this;

        this = container_of (node, HevTaskIOReactorWSANode, base);

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
hev_task_io_reactor_wsa_handler (void *data, BOOLEAN fired)
{
    HevTaskIOReactorWSANode *node = data;
    WSANETWORKEVENTS events;
    int res;

    if (fired)
        return;

    res = WSAEnumNetworkEvents (node->handle, node->ehandle, &events);
    if (res || !events.lNetworkEvents)
        return;

    hev_task_io_reactor_wsa_node_ref (node);
    node->events = events.lNetworkEvents;

    res = PostQueuedCompletionStatus (node->ihandle, 1, 0, (LPOVERLAPPED)node);
    if (!res)
        hev_task_io_reactor_wsa_node_unref (node);
}

static int
hev_task_io_reactor_wsa_add (HevTaskIOReactorWSA *self,
                             HevTaskIOReactorSetupEvent *event)
{
    HevTaskIOReactorWSANode *node;
    long events;
    int res;

    node = hev_task_io_reactor_wsa_lookup (self, event->handle);
    if (node)
        goto exit;

    node = hev_task_io_reactor_wsa_node_new ();
    if (!node)
        goto exit;

    node->ehandle = CreateEventA (NULL, FALSE, FALSE, NULL);
    if (!node->ehandle)
        goto free_node;

    events = event->events | HEV_TASK_IO_REACTOR_EV_ER;
    res = WSAEventSelect (event->handle, node->ehandle, events);
    if (res)
        goto free_event;

    res = RegisterWaitForSingleObject (&node->whandle, node->ehandle,
                                       hev_task_io_reactor_wsa_handler, node,
                                       INFINITE, 0);
    if (!res)
        goto free_event;

    node->handle = event->handle;
    node->data = event->data;
    node->ihandle = self->base.handle;
    hev_task_io_reactor_wsa_insert (self, node);

    return 0;

free_event:
    WSACloseEvent (node->ehandle);
free_node:
    hev_task_io_reactor_wsa_node_unref (node);
exit:
    return -1;
}

static int
hev_task_io_reactor_wsa_mod (HevTaskIOReactorWSA *self,
                             HevTaskIOReactorSetupEvent *event)
{
    HevTaskIOReactorWSANode *node;
    long events;
    int res;

    node = hev_task_io_reactor_wsa_lookup (self, event->handle);
    if (!node)
        return -1;

    events = event->events | HEV_TASK_IO_REACTOR_EV_ER;
    res = WSAEventSelect (node->handle, node->ehandle, events);
    if (res)
        return -1;

    return 0;
}

static int
hev_task_io_reactor_wsa_del (HevTaskIOReactorWSA *self,
                             HevTaskIOReactorSetupEvent *event)
{
    HevTaskIOReactorWSANode *node;
    int res;

    node = hev_task_io_reactor_wsa_lookup (self, event->handle);
    if (!node)
        return -1;

    res = UnregisterWaitEx (node->whandle, INVALID_HANDLE_VALUE);
    res &= WSACloseEvent (node->ehandle);
    if (!res)
        return -1;

    node->ehandle = NULL;
    node->whandle = NULL;
    hev_rbtree_erase (&self->tree, &node->base);
    hev_task_io_reactor_wsa_node_unref (node);

    return 0;
}

HevTaskIOReactor *
hev_task_io_reactor_new (void)
{
    HevTaskIOReactorWSA *self;
    HANDLE handle;

    self = hev_malloc0 (sizeof (HevTaskIOReactorWSA));
    if (!self)
        return NULL;

    handle = CreateIoCompletionPort (INVALID_HANDLE_VALUE, NULL, 0, 0);
    if (!handle) {
        hev_free (self);
        return NULL;
    }

    pthread_mutex_init (&self->mutex, NULL);
    self->base.handle = handle;

    return &self->base;
}

void
hev_task_io_reactor_destroy (HevTaskIOReactor *_self)
{
    HevTaskIOReactorWSA *self = (HevTaskIOReactorWSA *)_self;

    CloseHandle (_self->handle);
    pthread_mutex_destroy (&self->mutex);
    hev_free (self);
}

int
hev_task_io_reactor_setup (HevTaskIOReactor *_self,
                           HevTaskIOReactorSetupEvent *events, int count)
{
    HevTaskIOReactorWSA *self = (HevTaskIOReactorWSA *)_self;
    int res = -1;

    assert (count <= HEV_TASK_IO_REACTOR_EVENT_GEN_MAX);

    if (!events || count <= 0)
        return res;

    pthread_mutex_lock (&self->mutex);

    switch (events->op) {
    case HEV_TASK_IO_REACTOR_OP_ADD:
        res = hev_task_io_reactor_wsa_add (self, events);
        break;
    case HEV_TASK_IO_REACTOR_OP_MOD:
        res = hev_task_io_reactor_wsa_mod (self, events);
        break;
    case HEV_TASK_IO_REACTOR_OP_DEL:
        res = hev_task_io_reactor_wsa_del (self, events);
    default:
        break;
    }

    pthread_mutex_unlock (&self->mutex);

    return res;
}

int
hev_task_io_reactor_wait (HevTaskIOReactor *self,
                          HevTaskIOReactorWaitEvent *events, int count,
                          int timeout)
{
    HevTaskIOReactorWSANode *node;
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

    node = (HevTaskIOReactorWSANode *)overlap;
    events->events = node->events;
    events->data = node->data;
    res = node->ehandle && node->whandle;
    hev_task_io_reactor_wsa_node_unref (node);

    if (!res)
        goto retry;

    return 1;
}

#endif /* defined(__MSYS__) */
