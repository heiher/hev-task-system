/*
 ============================================================================
 Name        : hev-task-channel.c
 Author      : Heiher <r@hev.cc>
 Copyright   : Copyright (c) 2019 everyone.
 Description : Channel
 ============================================================================
 */

#include "kern/task/hev-task.h"
#include "lib/utils/hev-compiler.h"
#include "mm/api/hev-memory-allocator-api.h"

#include "hev-task-channel.h"
#include "hev-task-channel-private.h"

EXPORT_SYMBOL int
hev_task_channel_new (HevTaskChannel **chan1, HevTaskChannel **chan2)
{
    return hev_task_channel_new_with_buffers (chan1, chan2, 0);
}

EXPORT_SYMBOL int
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
    c1->reader = NULL;
    c1->writer = NULL;
    c1->rd_idx = 0;
    c1->wr_idx = 0;
    c1->use_count = 0;
    c1->max_count = buffers;
    c1->ref_count = 1;

    c2->peer = c1;
    c2->reader = NULL;
    c2->writer = NULL;
    c2->rd_idx = 0;
    c2->wr_idx = 0;
    c2->use_count = 0;
    c2->max_count = buffers;
    c2->ref_count = 1;

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

EXPORT_SYMBOL void
hev_task_channel_destroy (HevTaskChannel *self)
{
    if (self->peer) {
        self->peer->peer = NULL;
        if (self->peer->reader)
            hev_task_wakeup (self->peer->reader);
    }
    if (self->writer)
        hev_task_wakeup (self->writer);

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
        __builtin_memcpy (dst, src, size);
    }

    return size;
}

EXPORT_SYMBOL ssize_t
hev_task_channel_read (HevTaskChannel *self, void *buffer, size_t count)
{
    HevTaskChannelBuffer *cbuf;
    ssize_t size = -1;

    /* wait on empty */
    while (!hev_task_channel_is_readable (self)) {
        /* check is peer alive because cond wait may yield */
        if (!hev_task_channel_is_active (self))
            goto out;

        WRITE_ONCE (self->reader, hev_task_self ());
        hev_task_yield (HEV_TASK_WAITIO);
        WRITE_ONCE (self->reader, NULL);
    }

    barrier ();
    cbuf = &self->buffers[self->rd_idx];
    self->rd_idx = (self->rd_idx + 1) % self->max_count;

    size = count;
    if (cbuf->size < count)
        size = cbuf->size;
    size = hev_task_channel_data_copy (buffer, cbuf->data, size);

    if (self->use_count == self->max_count) {
        if (self->writer)
            hev_task_wakeup (self->writer);
    }

    self->use_count--;

out:
    return size;
}

EXPORT_SYMBOL ssize_t
hev_task_channel_write (HevTaskChannel *self, const void *buffer, size_t count)
{
    HevTaskChannel *peer = self->peer;
    HevTaskChannelBuffer *cbuf;
    ssize_t size = -1;

    if (!peer)
        goto out0;

    hev_task_channel_ref (peer);

    /* wait on full */
    while (!hev_task_channel_is_writable (peer)) {
        /* check is peer alive because cond wait may yield */
        if (!hev_task_channel_is_active (self))
            goto out1;

        WRITE_ONCE (peer->writer, hev_task_self ());
        hev_task_yield (HEV_TASK_WAITIO);
        WRITE_ONCE (peer->writer, NULL);
    }

    barrier ();
    cbuf = &peer->buffers[peer->wr_idx];
    peer->wr_idx = (peer->wr_idx + 1) % peer->max_count;

    size = count;
    cbuf->size = size;
    if (peer->max_count == 1)
        cbuf->data = (HevTaskChannelData *)buffer;
    else
        size = hev_task_channel_data_copy (cbuf->data, buffer, size);

    if (peer->use_count == 0) {
        if (peer->reader)
            hev_task_wakeup (peer->reader);
    }

    peer->use_count++;

    /* sync */
    while (!hev_task_channel_is_writable (peer)) {
        /* check is peer alive because cond wait may yield */
        if (!hev_task_channel_is_active (self)) {
            size = -1;
            break;
        }

        WRITE_ONCE (peer->writer, hev_task_self ());
        hev_task_yield (HEV_TASK_WAITIO);
        WRITE_ONCE (peer->writer, NULL);
    }

    barrier ();
out1:
    hev_task_channel_unref (peer);
out0:

    return size;
}
