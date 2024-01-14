/*
 ============================================================================
 Name        : hev-task-cio-buffer.c
 Author      : hev <r@hev.cc>
 Copyright   : Copyright (c) 2024 hev.
 Description : Task Chain I/O Buffer
 ============================================================================
 */

#include <errno.h>
#include <string.h>

#include "kern/task/hev-task.h"
#include "lib/io/basic/hev-task-io.h"
#include "lib/misc/hev-compiler.h"
#include "mem/api/hev-memory-allocator-api.h"

#include "hev-task-cio-buffer.h"

EXPORT_SYMBOL HevTaskCIOBuffer *
hev_task_cio_buffer_new (size_t rsize, size_t wsize)
{
    HevTaskCIOBuffer *self;
    int res;

    self = hev_malloc0 (sizeof (HevTaskCIOBuffer));
    if (!self)
        return NULL;

    res = hev_task_cio_buffer_construct (self, rsize, wsize);
    if (res < 0) {
        hev_free (self);
        return NULL;
    }

    return self;
}

EXPORT_SYMBOL ssize_t
hev_task_cio_buffer_peek (HevTaskCIOBuffer *self, void *buf, size_t count)
{
    struct iovec iov[2];
    ssize_t res;
    int iovcnt;
    int i;

    if (!self->rbuf)
        return -1;

    iovcnt = hev_circular_buffer_writing (self->rbuf, iov);
    if (iovcnt) {
        HevTaskCIO *base = HEV_TASK_CIO (self);

        res = hev_task_cio_next_readv (base, iov, iovcnt);
        if (res > 0)
            hev_circular_buffer_write_finish (self->rbuf, res);
    }

    iovcnt = hev_circular_buffer_reading (self->rbuf, iov);
    for (res = 0, i = 0; i < iovcnt; i++) {
        if (iov[i].iov_len < count) {
            memcpy (buf + res, iov[i].iov_base, iov[i].iov_len);
            count -= iov[i].iov_len;
            res += iov[i].iov_len;
        } else {
            memcpy (buf + res, iov[i].iov_base, count);
            res += count;
            break;
        }
    }

    return res;
}

EXPORT_SYMBOL int
hev_task_cio_buffer_peekv (HevTaskCIOBuffer *self, struct iovec *iov)
{
    int iovcnt;
    int res;

    if (!self->rbuf)
        return -1;

    iovcnt = hev_circular_buffer_writing (self->rbuf, iov);
    if (iovcnt) {
        HevTaskCIO *base = HEV_TASK_CIO (self);
        ssize_t res;

        res = hev_task_cio_next_readv (base, iov, iovcnt);
        if (res > 0)
            hev_circular_buffer_write_finish (self->rbuf, res);
    }

    res = hev_circular_buffer_reading (self->rbuf, iov);

    return res;
}

static ssize_t
_hev_task_cio_buffer_read (HevTaskCIO *base, void *buf, size_t count)
{
    HevTaskCIOBuffer *self = HEV_TASK_CIO_BUFFER (base);
    struct iovec iov[2];
    size_t res;
    int iovcnt;
    int i;

    if (!self->rbuf)
        return hev_task_cio_next_read (base, buf, count);

    res = hev_circular_buffer_get_use_size (self->rbuf);
    if (res == 0) {
        ssize_t res;

        iovcnt = hev_circular_buffer_writing (self->rbuf, iov);
        res = hev_task_cio_next_readv (base, iov, iovcnt);
        if (res <= 0)
            return res;
        hev_circular_buffer_write_finish (self->rbuf, res);
    }

    iovcnt = hev_circular_buffer_reading (self->rbuf, iov);
    for (res = 0, i = 0; i < iovcnt; i++) {
        if (iov[i].iov_len < count) {
            memcpy (buf + res, iov[i].iov_base, iov[i].iov_len);
            count -= iov[i].iov_len;
            res += iov[i].iov_len;
        } else {
            memcpy (buf + res, iov[i].iov_base, count);
            res += count;
            break;
        }
    }
    hev_circular_buffer_read_finish (self->rbuf, res);

    return res;
}

static int
_hev_task_cio_buffer_flush (HevTaskCIOBuffer *self)
{
    HevTaskCIO *base = HEV_TASK_CIO (self);
    struct iovec iov[2];
    ssize_t res;
    int iovcnt;

    if (!self->wbuf)
        return 0;

    iovcnt = hev_circular_buffer_reading (self->wbuf, iov);
    if (iovcnt == 0)
        return 0;

    res = hev_task_cio_next_writev (base, iov, iovcnt);
    if (res < 0)
        return res;

    hev_circular_buffer_read_finish (self->wbuf, res);

    res = hev_circular_buffer_get_use_size (self->wbuf);
    if (res) {
        base->err = EAGAIN;
        return -1;
    }

    return 0;
}

static ssize_t
_hev_task_cio_buffer_write (HevTaskCIO *base, const void *buf, size_t count)
{
    HevTaskCIOBuffer *self = HEV_TASK_CIO_BUFFER (base);
    struct iovec iov[2];
    size_t res;
    int iovcnt;
    int i;

    if (!self->wbuf)
        return hev_task_cio_next_write (base, buf, count);

    iovcnt = hev_circular_buffer_writing (self->wbuf, iov);
    if (iovcnt == 0) {
        ssize_t res;

        res = _hev_task_cio_buffer_flush (self);
        if (res < 0)
            return res;
    }

    iovcnt = hev_circular_buffer_writing (self->wbuf, iov);
    for (res = 0, i = 0; i < iovcnt; i++) {
        if (iov[i].iov_len < count) {
            memcpy (iov[i].iov_base, buf + res, iov[i].iov_len);
            count -= iov[i].iov_len;
            res += iov[i].iov_len;
        } else {
            memcpy (iov[i].iov_base, buf + res, count);
            res += count;
            break;
        }
    }
    hev_circular_buffer_write_finish (self->wbuf, res);

    return res;
}

static ssize_t
_hev_task_cio_buffer_readv (HevTaskCIO *base, const struct iovec *iov,
                            int iovcnt)
{
    HevTaskCIOBuffer *self = HEV_TASK_CIO_BUFFER (base);
    size_t size = 0;
    int i;

    if (!self->rbuf)
        return hev_task_cio_next_readv (base, iov, iovcnt);

    for (i = 0; i < iovcnt; i++) {
        const struct iovec *v = &iov[i];
        ssize_t res;

        res = _hev_task_cio_buffer_read (base, v->iov_base, v->iov_len);
        if (res <= 0) {
            if (size)
                return size;
            return res;
        }

        size += res;
        if (res < v->iov_len)
            break;
    }

    return size;
}

static ssize_t
_hev_task_cio_buffer_writev (HevTaskCIO *base, const struct iovec *iov,
                             int iovcnt)
{
    HevTaskCIOBuffer *self = HEV_TASK_CIO_BUFFER (base);
    size_t size = 0;
    int i;

    if (!self->wbuf)
        return hev_task_cio_next_writev (base, iov, iovcnt);

    for (i = 0; i < iovcnt; i++) {
        const struct iovec *v = &iov[i];
        ssize_t res;

        res = _hev_task_cio_buffer_write (base, v->iov_base, v->iov_len);
        if (res <= 0) {
            if (size)
                return size;
            return res;
        }

        size += res;
        if (res < v->iov_len)
            break;
    }

    return size;
}

static long
_hev_task_cio_buffer_ctrl (HevTaskCIO *base, int ctrl, long larg, void *parg)
{
    HevTaskCIOBuffer *self = HEV_TASK_CIO_BUFFER (base);

    if (ctrl == HEV_TASK_CIO_CTRL_FLUSH) {
        int res;

        res = _hev_task_cio_buffer_flush (self);
        if (res < 0)
            return res;
    }

    return hev_task_cio_next_ctrl (base, ctrl, larg, parg);
}

EXPORT_SYMBOL int
hev_task_cio_buffer_construct (HevTaskCIOBuffer *self, size_t rsize,
                               size_t wsize)
{
    int res;

    res = hev_task_cio_construct (&self->base);
    if (res < 0)
        return res;

    HEV_OBJECT (self)->klass = HEV_TASK_CIO_BUFFER_TYPE;

    if (rsize)
        self->rbuf = hev_circular_buffer_new (rsize);
    if (wsize)
        self->wbuf = hev_circular_buffer_new (wsize);

    return 0;
}

static void
_hev_task_cio_buffer_destruct (HevObject *base)
{
    HevTaskCIOBuffer *self = HEV_TASK_CIO_BUFFER (base);

    if (self->rbuf)
        hev_circular_buffer_unref (self->rbuf);
    if (self->wbuf)
        hev_circular_buffer_unref (self->wbuf);

    HEV_TASK_CIO_TYPE->destruct (base);
    hev_free (base);
}

EXPORT_SYMBOL HevObjectClass *
hev_task_cio_buffer_class (void)
{
    static HevTaskCIOBufferClass klass;
    HevTaskCIOBufferClass *kptr = &klass;
    HevObjectClass *okptr = HEV_OBJECT_CLASS (kptr);
    HevTaskCIOClass *ckptr;

    if (okptr->name)
        return okptr;

    memcpy (kptr, HEV_TASK_CIO_TYPE, sizeof (HevTaskCIOClass));

    okptr->name = "HevTaskCIOBuffer";
    okptr->destruct = _hev_task_cio_buffer_destruct;

    ckptr = &kptr->base;
    ckptr->read = _hev_task_cio_buffer_read;
    ckptr->write = _hev_task_cio_buffer_write;
    ckptr->readv = _hev_task_cio_buffer_readv;
    ckptr->writev = _hev_task_cio_buffer_writev;
    ckptr->ctrl = _hev_task_cio_buffer_ctrl;

    return okptr;
}
