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

#define MAX_BUFFER_SIZE (16384)

typedef union _HevTaskChannelData HevTaskChannelData;
typedef struct _HevTaskChannelBuffer HevTaskChannelBuffer;

union _HevTaskChannelData
{
    char b[MAX_BUFFER_SIZE];
    short h[0];
    int w[0];
    long long d[0];
};

struct _HevTaskChannelBuffer
{
    size_t size;
    HevTaskChannelData *data;
};

struct _HevTaskChannel
{
    HevTaskChannel *peer;

    HevTaskCond cond_full;
    HevTaskCond cond_empty;

    unsigned int rd_idx;
    unsigned int wr_idx;
    unsigned int use_count;
    unsigned int max_count;
    unsigned int ref_count;

    HevTaskChannelBuffer buffers[0];
};

int
hev_task_channel_new (HevTaskChannel **chan1, HevTaskChannel **chan2)
{
    return hev_task_channel_new_with_buffers (chan1, chan2, 0);
}

int
hev_task_channel_new_with_buffers (HevTaskChannel **chan1,
                                   HevTaskChannel **chan2, unsigned int buffers)
{
    HevTaskChannel *c1, *c2;
    unsigned int buffers_size;
    unsigned int data_size = 0;

    buffers += 1;
    if (buffers > 1)
        data_size = sizeof (HevTaskChannelData) * buffers;
    buffers_size = sizeof (HevTaskChannelBuffer) * buffers;

    c1 = hev_malloc (sizeof (HevTaskChannel) + buffers_size + data_size);
    if (!c1)
        goto err0;

    c2 = hev_malloc (sizeof (HevTaskChannel) + buffers_size + data_size);
    if (!c2)
        goto err1;

    c1->peer = c2;
    c1->rd_idx = 0;
    c1->wr_idx = 0;
    c1->use_count = 0;
    c1->max_count = buffers;
    c1->ref_count = 1;

    if (hev_task_cond_init (&c1->cond_full) != 0)
        goto err2;

    if (hev_task_cond_init (&c1->cond_empty) != 0)
        goto err2;

    c2->peer = c1;
    c2->rd_idx = 0;
    c2->wr_idx = 0;
    c2->use_count = 0;
    c2->max_count = buffers;
    c2->ref_count = 1;

    if (hev_task_cond_init (&c2->cond_full) != 0)
        goto err2;

    if (hev_task_cond_init (&c2->cond_empty) != 0)
        goto err2;

    if (buffers > 1) {
        unsigned int i;
        HevTaskChannelData *c1_data, *c2_data;

        c1_data = (void *)c1->buffers + buffers_size;
        c2_data = (void *)c2->buffers + buffers_size;

        for (i = 0; i < buffers; i++) {
            c1->buffers[i].data = &c1_data[i];
            c2->buffers[i].data = &c2_data[i];
        }
    }

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
    if (self->peer) {
        self->peer->peer = NULL;
        hev_task_cond_broadcast (&self->peer->cond_empty);
    }
    hev_task_cond_broadcast (&self->cond_full);

    hev_task_channel_unref (self);
}

static ssize_t
hev_task_channel_data_copy (void *dst, const void *src, size_t size)
{
    HevTaskChannelData *d = dst;
    const HevTaskChannelData *s = src;

    if (size > MAX_BUFFER_SIZE)
        size = MAX_BUFFER_SIZE;

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
    case sizeof (long long):
        d->d[0] = s->d[0];
        break;
    default:
        memcpy (dst, src, size);
    }

    return size;
}

ssize_t
hev_task_channel_read (HevTaskChannel *self, void *buffer, size_t count)
{
    HevTaskChannelBuffer *cbuf;
    ssize_t size = -1;

    /* wait on empty */
    while (self->use_count == 0) {
        /* check is peer alive because cond wait may yield */
        if (!self->peer)
            goto out;
        hev_task_cond_wait (&self->cond_empty, NULL);
    }

    cbuf = &self->buffers[self->rd_idx];
    self->rd_idx = (self->rd_idx + 1) % self->max_count;

    size = count;
    if (cbuf->size < count)
        size = cbuf->size;
    size = hev_task_channel_data_copy (buffer, cbuf->data, size);

    if (self->use_count == self->max_count)
        hev_task_cond_signal (&self->cond_full);

    self->use_count--;

out:
    return size;
}

ssize_t
hev_task_channel_write (HevTaskChannel *self, const void *buffer, size_t count)
{
    HevTaskChannel *peer = self->peer;
    HevTaskChannelBuffer *cbuf;
    ssize_t size = -1;

    if (!peer)
        goto out0;

    hev_task_channel_ref (peer);

    /* wait on full */
    while (peer->use_count == peer->max_count) {
        /* check is peer alive because cond wait may yield */
        if (!self->peer)
            goto out1;
        hev_task_cond_wait (&peer->cond_full, NULL);
    }

    cbuf = &peer->buffers[peer->wr_idx];
    peer->wr_idx = (peer->wr_idx + 1) % peer->max_count;

    size = count;
    cbuf->size = size;
    if (peer->max_count == 1)
        cbuf->data = (HevTaskChannelData *)buffer;
    else
        size = hev_task_channel_data_copy (cbuf->data, buffer, size);

    if (peer->use_count == 0)
        hev_task_cond_signal (&peer->cond_empty);

    peer->use_count++;

    /* sync */
    while (peer->use_count == peer->max_count) {
        /* check is peer alive because cond wait may yield */
        if (!self->peer) {
            size = -1;
            break;
        }
        hev_task_cond_wait (&peer->cond_full, NULL);
    }

out1:
    hev_task_channel_unref (peer);
out0:

    return size;
}
