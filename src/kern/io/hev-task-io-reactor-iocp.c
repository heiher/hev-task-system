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

#include <hev-memory-allocator.h>

#include "lib/misc/hev-compiler.h"

#include "kern/io/hev-task-io-reactor.h"

typedef struct _HevTaskIOReactorIOCPNode HevTaskIOReactorIOCPNode;

/*
 * Every node, a socket or a waitable handle, waits on an event through a wait
 * completion packet (Windows 8+): the signal is queued to the completion port
 * directly, so nothing runs outside the reactor thread. A packet completes
 * once and stays idle until it is armed again.
 *
 * A socket event is reset by WSAEnumNetworkEvents, so the reactor enumerates
 * the events and arms the packet again when it receives the completion. This
 * keeps the edge-triggered semantics of WSAEventSelect.
 *
 * A waitable handle can stay signaled (a process, a manual-reset event), so
 * the reactor never arms it again by itself, or it would spin: the owner adds
 * the handle again to wait for the next signal.
 */
struct _HevTaskIOReactorIOCPNode
{
    HevRBTreeNode base;

    uintptr_t handle;
    void *data;

    void *ehandle; /* event to wait for */
    void *packet; /* wait completion packet */
    int armed; /* a completion is pending or queued */
};

typedef long (WINAPI *HevNtCreateWaitCompletionPacket) (PHANDLE, unsigned long,
                                                        PVOID);
typedef long (WINAPI *HevNtAssociateWaitCompletionPacket) (HANDLE, HANDLE,
                                                           HANDLE, PVOID, PVOID,
                                                           long, ULONG_PTR,
                                                           unsigned char *);
typedef long (WINAPI *HevNtCancelWaitCompletionPacket) (HANDLE, unsigned char);

static HevNtCreateWaitCompletionPacket nt_create_wait_packet;
static HevNtAssociateWaitCompletionPacket nt_associate_wait_packet;
static HevNtCancelWaitCompletionPacket nt_cancel_wait_packet;

/* GENERIC_READ | GENERIC_WRITE */
#define HEV_WAIT_PACKET_ACCESS (0x80000000UL | 0x40000000UL)

static int
hev_task_io_reactor_iocp_init (void)
{
    void *ntdll;

    if (nt_create_wait_packet)
        return 0;

    ntdll = GetModuleHandleA ("ntdll.dll");
    if (!ntdll)
        return -1;

    nt_associate_wait_packet =
        (HevNtAssociateWaitCompletionPacket)GetProcAddress (
            ntdll, "NtAssociateWaitCompletionPacket");
    nt_cancel_wait_packet = (HevNtCancelWaitCompletionPacket)GetProcAddress (
        ntdll, "NtCancelWaitCompletionPacket");
    nt_create_wait_packet = (HevNtCreateWaitCompletionPacket)GetProcAddress (
        ntdll, "NtCreateWaitCompletionPacket");
    if (!nt_associate_wait_packet || !nt_cancel_wait_packet ||
        !nt_create_wait_packet) {
        nt_create_wait_packet = NULL;
        return -1;
    }

    return 0;
}

static inline int
hev_task_io_reactor_iocp_node_is_handle (HevTaskIOReactorIOCPNode *node)
{
    return node->handle == (uintptr_t)node->ehandle;
}

/* Arming an armed node is a no-op, its completion is still to be received. */
static int
hev_task_io_reactor_iocp_node_arm (HevTaskIOReactorIOCPNode *node, void *port)
{
    unsigned char signaled = 0;

    if (node->armed)
        return 0;

    if (nt_associate_wait_packet (node->packet, port, node->ehandle, NULL, node,
                                  0, 1, &signaled) < 0)
        return -1;
    node->armed = 1;

    return 0;
}

static void
hev_task_io_reactor_iocp_node_free (HevTaskIOReactorIOCPNode *node)
{
    /* Remove a completion that is queued but not yet received. */
    if (node->armed)
        nt_cancel_wait_packet (node->packet, 1);
    if (node->packet)
        CloseHandle (node->packet);

    if (!hev_task_io_reactor_iocp_node_is_handle (node)) {
        uintptr_t sock = hev_sys_handle_to_socket (node->handle);

        /* The owner may have closed the socket without deleting it. */
        if (sock != HEV_SYS_INVALID_SOCKET)
            WSAEventSelect (sock, NULL, 0);
        WSACloseEvent (node->ehandle);
    }

    free (node);
}

static void
hev_task_io_reactor_iocp_node_detach (HevTaskIOReactorIOCP *self,
                                      HevTaskIOReactorIOCPNode *node)
{
    hev_rbtree_erase (&self->tree, &node->base);
    hev_task_io_reactor_iocp_node_free (node);
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

static int
hev_task_io_reactor_iocp_add (HevTaskIOReactorIOCP *self,
                              HevTaskIOReactorSetupEvent *event)
{
    HevTaskIOReactorIOCPNode *node;

    node = hev_task_io_reactor_iocp_lookup (self, event->handle);
    if (node) {
        /* Only a waitable handle is added again, to wait for the next signal. */
        if (!hev_task_io_reactor_iocp_node_is_handle (node) || event->events)
            return -1;
        node->data = event->data;
        return hev_task_io_reactor_iocp_node_arm (node, self->base.handle);
    }

    node = calloc (1, sizeof (HevTaskIOReactorIOCPNode));
    if (!node)
        return -1;

    node->data = event->data;
    node->handle = event->handle;

    if (event->events) {
        uintptr_t sock = hev_sys_handle_to_socket (event->handle);
        long events = event->events | HEV_TASK_IO_REACTOR_EV_ER;

        if (sock == HEV_SYS_INVALID_SOCKET)
            goto free_node;

        node->ehandle = CreateEventA (NULL, TRUE, FALSE, NULL);
        if (!node->ehandle)
            goto free_node;

        if (WSAEventSelect (sock, node->ehandle, events))
            goto free_event;
    } else {
        node->ehandle = (void *)event->handle;
    }

    if (nt_create_wait_packet (&node->packet, HEV_WAIT_PACKET_ACCESS, NULL) < 0)
        goto free_event;

    if (hev_task_io_reactor_iocp_node_arm (node, self->base.handle) < 0)
        goto free_node_all;

    hev_task_io_reactor_iocp_insert (self, node);

    return 0;

free_event:
    if (!hev_task_io_reactor_iocp_node_is_handle (node)) {
        WSAEventSelect (hev_sys_handle_to_socket (event->handle), NULL, 0);
        WSACloseEvent (node->ehandle);
    }
free_node:
    free (node);
    return -1;
free_node_all:
    hev_task_io_reactor_iocp_node_free (node);
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

    if (!hev_task_io_reactor_iocp_node_is_handle (node) && event->events) {
        uintptr_t sock = hev_sys_handle_to_socket (node->handle);
        long events = event->events | HEV_TASK_IO_REACTOR_EV_ER;

        if (sock == HEV_SYS_INVALID_SOCKET)
            return -1;

        if (WSAEventSelect (sock, node->ehandle, events))
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

    hev_task_io_reactor_iocp_node_detach (self, node);

    return 0;
}

HevTaskIOReactor *
hev_task_io_reactor_new (void)
{
    HevTaskIOReactorIOCP *self;
    HANDLE handle;

    if (hev_task_io_reactor_iocp_init () < 0)
        return NULL;

    self = hev_malloc0 (sizeof (HevTaskIOReactorIOCP));
    if (!self)
        return NULL;

    handle = CreateIoCompletionPort (INVALID_HANDLE_VALUE, NULL, 0, 0);
    if (!handle) {
        hev_free (self);
        return NULL;
    }

    self->base.handle = handle;

    return &self->base;
}

void
hev_task_io_reactor_destroy (HevTaskIOReactor *_self)
{
    HevTaskIOReactorIOCP *self = (HevTaskIOReactorIOCP *)_self;

    while (self->tree.root) {
        HevTaskIOReactorIOCPNode *node;

        node = container_of (self->tree.root, HevTaskIOReactorIOCPNode, base);
        hev_task_io_reactor_iocp_node_detach (self, node);
    }

    CloseHandle (_self->handle);
    hev_free (self);
}

int
hev_task_io_reactor_setup (HevTaskIOReactor *_self,
                           HevTaskIOReactorSetupEvent *events, int count)
{
    HevTaskIOReactorIOCP *self = (HevTaskIOReactorIOCP *)_self;

    assert (count <= HEV_TASK_IO_REACTOR_EVENT_GEN_MAX);

    if (!events || count <= 0)
        return -1;

    switch (events->op) {
    case HEV_TASK_IO_REACTOR_OP_ADD:
        return hev_task_io_reactor_iocp_add (self, events);
    case HEV_TASK_IO_REACTOR_OP_MOD:
        return hev_task_io_reactor_iocp_mod (self, events);
    case HEV_TASK_IO_REACTOR_OP_DEL:
        return hev_task_io_reactor_iocp_del (self, events);
    default:
        return -1;
    }
}

int
hev_task_io_reactor_wait (HevTaskIOReactor *self,
                          HevTaskIOReactorWaitEvent *events, int count,
                          int timeout)
{
    if (count <= 0)
        return -1;

    for (;;) {
        HevTaskIOReactorIOCPNode *node;
        WSANETWORKEVENTS nevents;
        OVERLAPPED *overlap = NULL;
        ULONG_PTR key = 0;
        DWORD bytes = 0;
        uintptr_t sock;
        int res;

        res = GetQueuedCompletionStatus (self->handle, &bytes, &key, &overlap,
                                         timeout);
        if (!res) {
            if (GetLastError () == WAIT_TIMEOUT)
                return 0;
            return -1;
        }

        /* The completion is received, the node is idle. */
        node = (HevTaskIOReactorIOCPNode *)overlap;
        node->armed = 0;

        events->data = node->data;

        if (hev_task_io_reactor_iocp_node_is_handle (node)) {
            events->events = 0;
            return 1;
        }

        /* The owner may have closed the socket without deleting it. */
        sock = hev_sys_handle_to_socket (node->handle);
        if (sock == HEV_SYS_INVALID_SOCKET)
            continue;

        res = WSAEnumNetworkEvents (sock, node->ehandle, &nevents);
        hev_task_io_reactor_iocp_node_arm (node, self->handle);
        if (res || !nevents.lNetworkEvents)
            continue;

        events->events = nevents.lNetworkEvents;
        return 1;
    }
}

#endif /* defined(__MSYS__) || (defined(_WIN32) && !defined(__CYGWIN__)) */
