/*
 ============================================================================
 Name        : hev-task-cio-null.c
 Author      : hev <r@hev.cc>
 Copyright   : Copyright (c) 2024 hev.
 Description : Task Chain I/O Null
 ============================================================================
 */

#include <string.h>

#include "kern/task/hev-task.h"
#include "lib/io/basic/hev-task-io.h"
#include "lib/misc/hev-compiler.h"
#include "mem/api/hev-memory-allocator-api.h"

#include "hev-task-cio-null.h"

EXPORT_SYMBOL HevTaskCIONull *
hev_task_cio_null_new (void)
{
    HevTaskCIONull *self;
    int res;

    self = hev_malloc0 (sizeof (HevTaskCIONull));
    if (!self)
        return NULL;

    res = hev_task_cio_null_construct (self);
    if (res < 0) {
        hev_free (self);
        return NULL;
    }

    return self;
}

static size_t
hev_task_cio_null_iov_len (const struct iovec *iov, int iovcnt)
{
    size_t res = 0;
    int i;

    for (i = 0; i < iovcnt; i++)
        res += iov[i].iov_len;

    return res;
}

static ssize_t
_hev_task_cio_null_read (HevTaskCIO *base, void *buf, size_t count)
{
    return 0;
}

static ssize_t
_hev_task_cio_null_write (HevTaskCIO *base, const void *buf, size_t count)
{
    return count;
}

static ssize_t
_hev_task_cio_null_readv (HevTaskCIO *base, const struct iovec *iov, int iovcnt)
{
    return 0;
}

static ssize_t
_hev_task_cio_null_writev (HevTaskCIO *base, const struct iovec *iov,
                           int iovcnt)
{
    return hev_task_cio_null_iov_len (iov, iovcnt);
}

static ssize_t
_hev_task_cio_null_read_dgram (HevTaskCIO *base, void *buf, size_t count,
                               void *addr)
{
    return 0;
}

static ssize_t
_hev_task_cio_null_write_dgram (HevTaskCIO *base, const void *buf, size_t count,
                                const void *addr)
{
    return count;
}

static ssize_t
_hev_task_cio_null_readv_dgram (HevTaskCIO *base, const struct iovec *iov,
                                int iovcnt, void *addr)
{
    return 0;
}

static ssize_t
_hev_task_cio_null_writev_dgram (HevTaskCIO *base, const struct iovec *iov,
                                 int iovcnt, const void *addr)
{
    return hev_task_cio_null_iov_len (iov, iovcnt);
}

static long
_hev_task_cio_null_ctrl (HevTaskCIO *base, int ctrl, long larg, void *parg)
{
    if (ctrl == HEV_TASK_CIO_CTRL_FLUSH)
        return 0;

    return -1;
}

EXPORT_SYMBOL int
hev_task_cio_null_construct (HevTaskCIONull *self)
{
    int res;

    res = hev_task_cio_construct (&self->base);
    if (res < 0)
        return res;

    HEV_OBJECT (self)->klass = HEV_TASK_CIO_NULL_TYPE;

    return 0;
}

static void
_hev_task_cio_null_destruct (HevObject *base)
{
    HEV_TASK_CIO_TYPE->destruct (base);
    hev_free (base);
}

EXPORT_SYMBOL HevObjectClass *
hev_task_cio_null_class (void)
{
    static HevTaskCIONullClass klass;
    HevTaskCIONullClass *kptr = &klass;
    HevObjectClass *okptr = HEV_OBJECT_CLASS (kptr);
    HevTaskCIOClass *ckptr;

    if (okptr->name)
        return okptr;

    memcpy (kptr, HEV_TASK_CIO_TYPE, sizeof (HevTaskCIOClass));

    okptr->name = "HevTaskCIONull";
    okptr->destruct = _hev_task_cio_null_destruct;

    ckptr = &kptr->base;
    ckptr->read = _hev_task_cio_null_read;
    ckptr->write = _hev_task_cio_null_write;
    ckptr->readv = _hev_task_cio_null_readv;
    ckptr->writev = _hev_task_cio_null_writev;
    ckptr->read_dgram = _hev_task_cio_null_read_dgram;
    ckptr->write_dgram = _hev_task_cio_null_write_dgram;
    ckptr->readv_dgram = _hev_task_cio_null_readv_dgram;
    ckptr->writev_dgram = _hev_task_cio_null_writev_dgram;
    ckptr->ctrl = _hev_task_cio_null_ctrl;

    return okptr;
}
