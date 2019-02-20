/*
 ============================================================================
 Name        : hev-task-channel.c
 Author      : Heiher <r@hev.cc>
 Copyright   : Copyright (c) 2019 everyone.
 Description : Channel
 ============================================================================
 */

#include <string.h>

#include "hev-task-channel.h"

#include "kern/sync/hev-task-cond.h"
#include "mm/api/hev-memory-allocator-api.h"

typedef union _HevTaskChannelData HevTaskChannelData;
typedef struct _HevTaskChannelNode HevTaskChannelNode;

struct _HevTaskChannel
{
    HevTaskChannel *peer;

    HevTaskMutex mutex;
    HevTaskCond cond;

    HevTaskChannelNode *head;
    HevTaskChannelNode *tail;

    unsigned int ref_count;
};

union _HevTaskChannelData
{
    char b[0];
    short h[0];
    int w[0];
    long d[0];
};

struct _HevTaskChannelNode
{
    HevTaskChannelNode *prev;
    HevTaskChannelNode *next;

    size_t size;
    HevTaskChannelData data;
};

int
hev_task_channel_new (HevTaskChannel **chan1, HevTaskChannel **chan2)
{
    HevTaskChannel *c1, *c2;

    c1 = hev_malloc (sizeof (HevTaskChannel));
    if (!c1)
        goto err0;

    c2 = hev_malloc (sizeof (HevTaskChannel));
    if (!c2)
        goto err1;

    c1->peer = c2;
    c1->head = NULL;
    c1->tail = NULL;
    c1->ref_count = 1;

    if (hev_task_mutex_init (&c1->mutex) != 0)
        goto err2;

    if (hev_task_cond_init (&c1->cond) != 0)
        goto err2;

    c2->peer = c1;
    c2->head = NULL;
    c2->tail = NULL;
    c2->ref_count = 1;

    if (hev_task_mutex_init (&c2->mutex) != 0)
        goto err2;

    if (hev_task_cond_init (&c2->cond) != 0)
        goto err2;

    *chan1 = c1;
    *chan2 = c2;

    return 0;

err2:
    hev_free (c2);
err1:
    hev_free (c1);
err0:

    return -1;
}

static void
hev_task_channel_ref (HevTaskChannel *self)
{
    self->ref_count++;
}

static void
hev_task_channel_unref (HevTaskChannel *self)
{
    self->ref_count--;
    if (self->ref_count)
        return;

    hev_free (self);
}

void
hev_task_channel_destroy (HevTaskChannel *self)
{
    HevTaskChannelNode *iter;

    hev_task_mutex_lock (&self->mutex);
    for (iter = self->head; iter;) {
        HevTaskChannelNode *node = iter;
        iter = node->next;
        hev_free (node);
    }
    if (self->peer)
        self->peer->peer = NULL;
    hev_task_cond_broadcast (&self->cond);
    hev_task_mutex_unlock (&self->mutex);

    hev_task_channel_unref (self);
}

static void
hev_task_channel_data_copy (void *dst, const void *src, size_t size)
{
    HevTaskChannelData *d = dst;
    const HevTaskChannelData *s = src;

    switch (size) {
    case 0:
        break;
    case sizeof (char):
        d->b[0] = s->b[0];
        break;
    case sizeof (short):
        d->h[0] = s->h[0];
        break;
    case sizeof (int):
        d->w[0] = s->w[0];
        break;
#ifdef _LP64
    case sizeof (long):
        d->d[0] = s->d[0];
        break;
#endif
    default:
        memcpy (dst, src, size);
    }
}

ssize_t
hev_task_channel_read (HevTaskChannel *self, void *buffer, size_t count)
{
    HevTaskChannelNode *node;
    ssize_t size = -1;

    hev_task_mutex_lock (&self->mutex);

    while (!self->head) {
        if (!self->peer)
            goto out;
        hev_task_cond_wait (&self->cond, &self->mutex);
    }

    node = self->head;
    if (node->next)
        node->next->prev = NULL;
    else
        self->tail = NULL;
    self->head = node->next;

    size = count;
    if (node->size < count)
        size = node->size;
    hev_task_channel_data_copy (buffer, &node->data, size);

    hev_free (node);

out:
    hev_task_mutex_unlock (&self->mutex);

    return size;
}

ssize_t
hev_task_channel_write (HevTaskChannel *self, const void *buffer, size_t count)
{
    HevTaskChannel *peer = self->peer;
    HevTaskChannelNode *node;
    ssize_t size = -1;

    if (!peer)
        goto out0;

    hev_task_channel_ref (peer);
    hev_task_mutex_lock (&peer->mutex);

    /* check is peer alive because mutex lock may yield */
    if (!self->peer)
        goto out1;

    node = hev_malloc (sizeof (HevTaskChannelNode) + count);
    if (!node)
        goto out1;

    if (!peer->tail)
        hev_task_cond_signal (&peer->cond);

    node->prev = peer->tail;
    node->next = NULL;
    node->size = count;
    if (peer->tail)
        peer->tail->next = node;
    else
        peer->head = node;
    peer->tail = node;

    size = count;
    hev_task_channel_data_copy (&node->data, buffer, size);

out1:
    hev_task_mutex_unlock (&peer->mutex);
    hev_task_channel_unref (peer);
out0:

    return size;
}
