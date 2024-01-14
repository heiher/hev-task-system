/*
 ============================================================================
 Name        : hev-task-cio-fd.c
 Author      : hev <r@hev.cc>
 Copyright   : Copyright (c) 2024 hev.
 Description : Task Chain I/O Fd
 ============================================================================
 */

#include <errno.h>
#include <string.h>
#include <unistd.h>

#include "kern/task/hev-task.h"
#include "lib/io/basic/hev-task-io.h"
#include "lib/misc/hev-compiler.h"
#include "mem/api/hev-memory-allocator-api.h"

#include "hev-task-cio-fd.h"

EXPORT_SYMBOL HevTaskCIOFd *
hev_task_cio_fd_new (int fdi, int fdo)
{
    HevTaskCIOFd *self;
    int res;

    self = hev_malloc0 (sizeof (HevTaskCIOFd));
    if (!self)
        return NULL;

    res = hev_task_cio_fd_construct (self, fdi, fdo);
    if (res < 0) {
        hev_free (self);
        return NULL;
    }

    return self;
}

static ssize_t
_hev_task_cio_fd_read (HevTaskCIO *base, void *buf, size_t count)
{
    HevTaskCIOFd *self = HEV_TASK_CIO_FD (base);
    ssize_t res;

    res = read (self->fdi, buf, count);
    base->err = errno;

    return res;
}

static ssize_t
_hev_task_cio_fd_write (HevTaskCIO *base, const void *buf, size_t count)
{
    HevTaskCIOFd *self = HEV_TASK_CIO_FD (base);
    ssize_t res;

    res = write (self->fdo, buf, count);
    base->err = errno;

    return res;
}

static ssize_t
_hev_task_cio_fd_readv (HevTaskCIO *base, const struct iovec *iov, int iovcnt)
{
    HevTaskCIOFd *self = HEV_TASK_CIO_FD (base);
    ssize_t res;

    res = readv (self->fdi, iov, iovcnt);
    base->err = errno;

    return res;
}

static ssize_t
_hev_task_cio_fd_writev (HevTaskCIO *base, const struct iovec *iov, int iovcnt)
{
    HevTaskCIOFd *self = HEV_TASK_CIO_FD (base);
    ssize_t res;

    res = writev (self->fdo, iov, iovcnt);
    base->err = errno;

    return res;
}

static long
_hev_task_cio_fd_ctrl (HevTaskCIO *base, int ctrl, long larg, void *parg)
{
    HevTaskCIOFd *self = HEV_TASK_CIO_FD (base);

    switch (ctrl) {
    case HEV_TASK_CIO_CTRL_FLUSH:
        return 0;
    case HEV_TASK_CIO_CTRL_GET_FD:
        if (larg)
            return self->fdo;
        return self->fdi;
    }

    return -1;
}

EXPORT_SYMBOL int
hev_task_cio_fd_construct (HevTaskCIOFd *self, int fdi, int fdo)
{
    int res;

    res = hev_task_cio_construct (&self->base);
    if (res < 0)
        return res;

    HEV_OBJECT (self)->klass = HEV_TASK_CIO_FD_TYPE;

    self->fdi = fdi;
    self->fdo = fdo;

    return 0;
}

static void
_hev_task_cio_fd_destruct (HevObject *base)
{
    HevTaskCIOFd *self = HEV_TASK_CIO_FD (base);

    if (self->fdi >= 0)
        close (self->fdi);
    if (self->fdo >= 0)
        close (self->fdo);

    HEV_TASK_CIO_TYPE->destruct (base);
    hev_free (base);
}

EXPORT_SYMBOL HevObjectClass *
hev_task_cio_fd_class (void)
{
    static HevTaskCIOFdClass klass;
    HevTaskCIOFdClass *kptr = &klass;
    HevObjectClass *okptr = HEV_OBJECT_CLASS (kptr);
    HevTaskCIOClass *ckptr;

    if (okptr->name)
        return okptr;

    memcpy (kptr, HEV_TASK_CIO_TYPE, sizeof (HevTaskCIOClass));

    okptr->name = "HevTaskCIOFd";
    okptr->destruct = _hev_task_cio_fd_destruct;

    ckptr = &kptr->base;
    ckptr->read = _hev_task_cio_fd_read;
    ckptr->write = _hev_task_cio_fd_write;
    ckptr->readv = _hev_task_cio_fd_readv;
    ckptr->writev = _hev_task_cio_fd_writev;
    ckptr->ctrl = _hev_task_cio_fd_ctrl;

    return okptr;
}
