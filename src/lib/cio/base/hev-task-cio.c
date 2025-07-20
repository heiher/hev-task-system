/*
 ============================================================================
 Name        : hev-task-cio.c
 Author      : hev <r@hev.cc>
 Copyright   : Copyright (c) 2024 hev.
 Description : Task Chain I/O
 ============================================================================
 */

#include <errno.h>
#include <string.h>

#include "kern/task/hev-task.h"
#include "lib/io/basic/hev-task-io.h"
#include "lib/io/buffer/hev-circular-buffer.h"
#include "lib/misc/hev-compiler.h"
#include "mem/api/hev-memory-allocator-api.h"

#include "hev-task-cio.h"

EXPORT_SYMBOL HevTaskCIO *
hev_task_cio_push (HevTaskCIO *self, HevTaskCIO *next)
{
    self->next = next;

    return self;
}

EXPORT_SYMBOL HevTaskCIO *
hev_task_cio_pop (HevTaskCIO *self)
{
    HevTaskCIO *next = self->next;

    self->next = NULL;
    hev_object_unref (HEV_OBJECT (self));

    return next;
}

EXPORT_SYMBOL int
hev_task_cio_get_fd (HevTaskCIO *self, int dir)
{
    HevTaskCIOClass *kptr = HEV_OBJECT_GET_CLASS (self);

    return kptr->ctrl (self, HEV_TASK_CIO_CTRL_GET_FD, dir, NULL);
}

EXPORT_SYMBOL ssize_t
hev_task_cio_read (HevTaskCIO *self, void *buf, size_t count,
                   HevTaskIOYielder yielder, void *yielder_data)
{
    HevTaskCIOClass *kptr = HEV_OBJECT_GET_CLASS (self);
    ssize_t res;

retry:
    res = kptr->read (self, buf, count);
    if ((res < 0) && (self->err == EAGAIN)) {
        if (yielder) {
            if (yielder (HEV_TASK_WAITIO, yielder_data))
                return -2;
        } else {
            hev_task_yield (HEV_TASK_WAITIO);
        }
        goto retry;
    }

    return res;
}

EXPORT_SYMBOL ssize_t
hev_task_cio_read_exact (HevTaskCIO *self, void *buf, size_t count,
                         HevTaskIOYielder yielder, void *yielder_data)
{
    HevTaskCIOClass *kptr = HEV_OBJECT_GET_CLASS (self);
    size_t size = 0;
    ssize_t res;

retry:
    res = kptr->read (self, buf + size, count - size);
    if ((res < 0) && (self->err == EAGAIN)) {
        if (yielder) {
            if (yielder (HEV_TASK_WAITIO, yielder_data))
                return -2;
        } else {
            hev_task_yield (HEV_TASK_WAITIO);
        }
        goto retry;
    }

    if (res <= 0) {
        if (size)
            return size;
        return res;
    }

    size += res;
    if (size < count)
        goto retry;

    return size;
}

EXPORT_SYMBOL ssize_t
hev_task_cio_readv (HevTaskCIO *self, const struct iovec *iov, int iovcnt,
                    HevTaskIOYielder yielder, void *yielder_data)
{
    HevTaskCIOClass *kptr = HEV_OBJECT_GET_CLASS (self);
    ssize_t res;

retry:
    res = kptr->readv (self, iov, iovcnt);
    if ((res < 0) && (self->err == EAGAIN)) {
        if (yielder) {
            if (yielder (HEV_TASK_WAITIO, yielder_data))
                return -2;
        } else {
            hev_task_yield (HEV_TASK_WAITIO);
        }
        goto retry;
    }

    return res;
}

EXPORT_SYMBOL ssize_t
hev_task_cio_readv_exact (HevTaskCIO *self, const struct iovec *iov, int iovcnt,
                          HevTaskIOYielder yielder, void *yielder_data)
{
    HevTaskCIOClass *kptr = HEV_OBJECT_GET_CLASS (self);
    struct iovec iovw[iovcnt];
    struct iovec *iovp = iovw;
    size_t size = 0, len = 0;
    ssize_t res;
    int i;

    for (i = 0; i < iovcnt; i++) {
        iovp[i] = iov[i];
        len += iov[i].iov_len;
    }

retry:
    res = kptr->readv (self, iovp, iovcnt);
    if ((res < 0) && (self->err == EAGAIN)) {
        if (yielder) {
            if (yielder (HEV_TASK_WAITIO, yielder_data))
                return -2;
        } else {
            hev_task_yield (HEV_TASK_WAITIO);
        }
        goto retry;
    }

    if (res <= 0) {
        if (size)
            return size;
        return res;
    }

    size += res;
    if (size < len) {
        for (i = 0; i < iovcnt; i++) {
            if (res < iovp[i].iov_len) {
                iovp[i].iov_base += res;
                iovp[i].iov_len -= res;
                break;
            }

            res -= iovp[i].iov_len;
        }

        iovp += i;
        iovcnt -= i;

        goto retry;
    }

    return size;
}

EXPORT_SYMBOL ssize_t
hev_task_cio_write (HevTaskCIO *self, const void *buf, size_t count,
                    HevTaskIOYielder yielder, void *yielder_data)
{
    HevTaskCIOClass *kptr = HEV_OBJECT_GET_CLASS (self);
    ssize_t res;

retry:
    res = kptr->write (self, buf, count);
    if ((res < 0) && (self->err == EAGAIN)) {
        if (yielder) {
            if (yielder (HEV_TASK_WAITIO, yielder_data))
                return -2;
        } else {
            hev_task_yield (HEV_TASK_WAITIO);
        }
        goto retry;
    }

    return res;
}

EXPORT_SYMBOL ssize_t
hev_task_cio_write_exact (HevTaskCIO *self, const void *buf, size_t count,
                          HevTaskIOYielder yielder, void *yielder_data)
{
    HevTaskCIOClass *kptr = HEV_OBJECT_GET_CLASS (self);
    size_t size = 0;
    ssize_t res;

retry:
    res = kptr->write (self, buf + size, count - size);
    if ((res < 0) && (self->err == EAGAIN)) {
        if (yielder) {
            if (yielder (HEV_TASK_WAITIO, yielder_data))
                return -2;
        } else {
            hev_task_yield (HEV_TASK_WAITIO);
        }
        goto retry;
    }

    if (res <= 0) {
        if (size)
            return size;
        return res;
    }

    size += res;
    if (size < count)
        goto retry;

    return size;
}

EXPORT_SYMBOL ssize_t
hev_task_cio_writev (HevTaskCIO *self, const struct iovec *iov, int iovcnt,
                     HevTaskIOYielder yielder, void *yielder_data)
{
    HevTaskCIOClass *kptr = HEV_OBJECT_GET_CLASS (self);
    ssize_t res;

retry:
    res = kptr->writev (self, iov, iovcnt);
    if ((res < 0) && (self->err == EAGAIN)) {
        if (yielder) {
            if (yielder (HEV_TASK_WAITIO, yielder_data))
                return -2;
        } else {
            hev_task_yield (HEV_TASK_WAITIO);
        }
        goto retry;
    }

    return res;
}

EXPORT_SYMBOL ssize_t
hev_task_cio_writev_exact (HevTaskCIO *self, const struct iovec *iov,
                           int iovcnt, HevTaskIOYielder yielder,
                           void *yielder_data)
{
    HevTaskCIOClass *kptr = HEV_OBJECT_GET_CLASS (self);
    struct iovec iovw[iovcnt];
    struct iovec *iovp = iovw;
    size_t size = 0, len = 0;
    ssize_t res;
    int i;

    for (i = 0; i < iovcnt; i++) {
        iovp[i] = iov[i];
        len += iov[i].iov_len;
    }

retry:
    res = kptr->writev (self, iovp, iovcnt);
    if ((res < 0) && (self->err == EAGAIN)) {
        if (yielder) {
            if (yielder (HEV_TASK_WAITIO, yielder_data))
                return -2;
        } else {
            hev_task_yield (HEV_TASK_WAITIO);
        }
        goto retry;
    }

    if (res <= 0) {
        if (size)
            return size;
        return res;
    }

    size += res;
    if (size < len) {
        for (i = 0; i < iovcnt; i++) {
            if (res < iovp[i].iov_len) {
                iovp[i].iov_base += res;
                iovp[i].iov_len -= res;
                break;
            }

            res -= iovp[i].iov_len;
        }

        iovp += i;
        iovcnt -= i;

        goto retry;
    }

    return size;
}

EXPORT_SYMBOL ssize_t
hev_task_cio_read_dgram (HevTaskCIO *self, void *buf, size_t count, void *addr,
                         HevTaskIOYielder yielder, void *yielder_data)
{
    HevTaskCIOClass *kptr = HEV_OBJECT_GET_CLASS (self);
    ssize_t res;

retry:
    res = kptr->read_dgram (self, buf, count, addr);
    if ((res < 0) && (self->err == EAGAIN)) {
        if (yielder) {
            if (yielder (HEV_TASK_WAITIO, yielder_data))
                return -2;
        } else {
            hev_task_yield (HEV_TASK_WAITIO);
        }
        goto retry;
    }

    return res;
}

EXPORT_SYMBOL ssize_t
hev_task_cio_readv_dgram (HevTaskCIO *self, const struct iovec *iov, int iovcnt,
                          void *addr, HevTaskIOYielder yielder,
                          void *yielder_data)
{
    HevTaskCIOClass *kptr = HEV_OBJECT_GET_CLASS (self);
    ssize_t res;

retry:
    res = kptr->readv_dgram (self, iov, iovcnt, addr);
    if ((res < 0) && (self->err == EAGAIN)) {
        if (yielder) {
            if (yielder (HEV_TASK_WAITIO, yielder_data))
                return -2;
        } else {
            hev_task_yield (HEV_TASK_WAITIO);
        }
        goto retry;
    }

    return res;
}

EXPORT_SYMBOL ssize_t
hev_task_cio_write_dgram (HevTaskCIO *self, const void *buf, size_t count,
                          const void *addr, HevTaskIOYielder yielder,
                          void *yielder_data)
{
    HevTaskCIOClass *kptr = HEV_OBJECT_GET_CLASS (self);
    ssize_t res;

retry:
    res = kptr->write_dgram (self, buf, count, addr);
    if ((res < 0) && (self->err == EAGAIN)) {
        if (yielder) {
            if (yielder (HEV_TASK_WAITIO, yielder_data))
                return -2;
        } else {
            hev_task_yield (HEV_TASK_WAITIO);
        }
        goto retry;
    }

    return res;
}

EXPORT_SYMBOL ssize_t
hev_task_cio_writev_dgram (HevTaskCIO *self, const struct iovec *iov,
                           int iovcnt, const void *addr,
                           HevTaskIOYielder yielder, void *yielder_data)
{
    HevTaskCIOClass *kptr = HEV_OBJECT_GET_CLASS (self);
    ssize_t res;

retry:
    res = kptr->writev_dgram (self, iov, iovcnt, addr);
    if ((res < 0) && (self->err == EAGAIN)) {
        if (yielder) {
            if (yielder (HEV_TASK_WAITIO, yielder_data))
                return -2;
        } else {
            hev_task_yield (HEV_TASK_WAITIO);
        }
        goto retry;
    }

    return res;
}

EXPORT_SYMBOL int
hev_task_cio_flush (HevTaskCIO *self, HevTaskIOYielder yielder,
                    void *yielder_data)
{
    HevTaskCIOClass *kptr = HEV_OBJECT_GET_CLASS (self);
    int res;

retry:
    res = kptr->ctrl (self, HEV_TASK_CIO_CTRL_FLUSH, 0, NULL);
    if ((res < 0) && (self->err == EAGAIN)) {
        if (yielder) {
            if (yielder (HEV_TASK_WAITIO, yielder_data))
                return -2;
        } else {
            hev_task_yield (HEV_TASK_WAITIO);
        }
        goto retry;
    }

    return res;
}

static int
task_cio_splice (HevCircularBuffer *buf, HevTaskCIO *i, HevTaskCIO *o)
{
    struct iovec iov[2];
    int res = 1, iovc;

    iovc = hev_circular_buffer_writing (buf, iov);
    if (iovc) {
        HevTaskCIOClass *kptr = HEV_OBJECT_GET_CLASS (i);
        ssize_t s = kptr->readv (i, iov, iovc);
        if (0 >= s) {
            if ((0 > s) && (i->err == EAGAIN))
                res = 0;
            else
                res = -1;
        } else {
            hev_circular_buffer_write_finish (buf, s);
        }
    }

    iovc = hev_circular_buffer_reading (buf, iov);
    if (iovc) {
        HevTaskCIOClass *kptr = HEV_OBJECT_GET_CLASS (o);
        ssize_t s = kptr->writev (o, iov, iovc);
        if (0 >= s) {
            if ((0 > s) && (o->err == EAGAIN))
                res = 0;
            else
                res = -1;
        } else {
            hev_circular_buffer_read_finish (buf, s);
            kptr->ctrl (o, HEV_TASK_CIO_CTRL_FLUSH, 0, NULL);
            res = 1;
        }
    } else if (res < 0) {
        HevTaskCIOClass *kptr = HEV_OBJECT_GET_CLASS (o);
        kptr->ctrl (o, HEV_TASK_CIO_CTRL_SHUTDOWN, 1, NULL);
    }

    return res;
}

EXPORT_SYMBOL void
hev_task_cio_splice (HevTaskCIO *a, HevTaskCIO *b, size_t buf_size,
                     HevTaskIOYielder yielder, void *data)
{
    HevCircularBuffer *buf_f;
    HevCircularBuffer *buf_b;
    int fai, fao, fbi, fbo;
    int res_f = 1;
    int res_b = 1;

    fai = hev_task_cio_get_fd (a, 0);
    fao = hev_task_cio_get_fd (a, 1);
    fbi = hev_task_cio_get_fd (b, 0);
    fbo = hev_task_cio_get_fd (b, 1);

    if (((fai | fao | fbi | fbo) >= 0) && !a->next && !b->next) {
        hev_task_io_splice (fai, fao, fbi, fbo, buf_size, yielder, data);
        return;
    }

    buf_f = hev_circular_buffer_new (buf_size);
    if (!buf_f)
        return;
    buf_b = hev_circular_buffer_new (buf_size);
    if (!buf_b)
        goto exit;

    for (;;) {
        HevTaskYieldType type;

        if (res_f >= 0)
            res_f = task_cio_splice (buf_f, a, b);
        if (res_b >= 0)
            res_b = task_cio_splice (buf_b, b, a);

        if (res_f > 0 || res_b > 0)
            type = HEV_TASK_YIELD;
        else if ((res_f & res_b) == 0)
            type = HEV_TASK_WAITIO;
        else
            break;

        if (yielder) {
            if (yielder (type, data))
                break;
        } else {
            hev_task_yield (type);
        }
    }

    hev_circular_buffer_unref (buf_b);
exit:
    hev_circular_buffer_unref (buf_f);
}

EXPORT_SYMBOL int
hev_task_cio_construct (HevTaskCIO *self)
{
    int res;

    res = hev_object_construct (&self->base);
    if (res < 0)
        return res;

    HEV_OBJECT (self)->klass = HEV_TASK_CIO_TYPE;

    self->err = 0;

    return 0;
}

static void
_hev_task_cio_destruct (HevObject *base)
{
    HevTaskCIO *self = HEV_TASK_CIO (base);

    hev_task_cio_next_destruct (self);

    HEV_OBJECT_TYPE->destruct (base);
}

EXPORT_SYMBOL HevObjectClass *
hev_task_cio_class (void)
{
    static HevTaskCIOClass klass;
    HevTaskCIOClass *kptr = &klass;
    HevObjectClass *okptr = HEV_OBJECT_CLASS (kptr);

    if (okptr->name)
        return okptr;

    memcpy (kptr, HEV_OBJECT_TYPE, sizeof (HevObjectClass));

    okptr->name = "HevTaskCIO";
    okptr->destruct = _hev_task_cio_destruct;

    kptr->read = hev_task_cio_next_read;
    kptr->write = hev_task_cio_next_write;
    kptr->readv = hev_task_cio_next_readv;
    kptr->writev = hev_task_cio_next_writev;
    kptr->read_dgram = hev_task_cio_next_read_dgram;
    kptr->write_dgram = hev_task_cio_next_write_dgram;
    kptr->readv_dgram = hev_task_cio_next_readv_dgram;
    kptr->writev_dgram = hev_task_cio_next_writev_dgram;
    kptr->ctrl = hev_task_cio_next_ctrl;

    return okptr;
}
