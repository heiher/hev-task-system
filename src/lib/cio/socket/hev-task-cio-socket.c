/*
 ============================================================================
 Name        : hev-task-cio-socket.c
 Author      : hev <r@hev.cc>
 Copyright   : Copyright (c) 2024 hev.
 Description : Task Chain I/O Socket
 ============================================================================
 */

#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/un.h>

#include "kern/task/hev-task.h"
#include "lib/io/basic/hev-task-io.h"
#include "lib/misc/hev-compiler.h"
#include "mem/api/hev-memory-allocator-api.h"

#include "hev-task-cio-socket.h"

EXPORT_SYMBOL HevTaskCIOSocket *
hev_task_cio_socket_new (int family, int fd)
{
    HevTaskCIOSocket *self;
    int res;

    self = hev_malloc0 (sizeof (HevTaskCIOSocket));
    if (!self)
        return NULL;

    res = hev_task_cio_socket_construct (self, family, fd);
    if (res < 0) {
        hev_free (self);
        return NULL;
    }

    return self;
}

static ssize_t
_hev_task_cio_socket_read (HevTaskCIO *base, void *buf, size_t count)
{
    HevTaskCIOSocket *self = HEV_TASK_CIO_SOCKET (base);
    ssize_t res;

    res = read (self->fd, buf, count);
    base->err = errno;

    return res;
}

static ssize_t
_hev_task_cio_socket_write (HevTaskCIO *base, const void *buf, size_t count)
{
    HevTaskCIOSocket *self = HEV_TASK_CIO_SOCKET (base);
    ssize_t res;

    res = write (self->fd, buf, count);
    base->err = errno;

    return res;
}

static ssize_t
_hev_task_cio_socket_readv (HevTaskCIO *base, const struct iovec *iov,
                            int iovcnt)
{
    HevTaskCIOSocket *self = HEV_TASK_CIO_SOCKET (base);
    ssize_t res;

    res = readv (self->fd, iov, iovcnt);
    base->err = errno;

    return res;
}

static ssize_t
_hev_task_cio_socket_writev (HevTaskCIO *base, const struct iovec *iov,
                             int iovcnt)
{
    HevTaskCIOSocket *self = HEV_TASK_CIO_SOCKET (base);
    ssize_t res;

    res = writev (self->fd, iov, iovcnt);
    base->err = errno;

    return res;
}

static ssize_t
_hev_task_cio_socket_read_dgram (HevTaskCIO *base, void *buf, size_t count,
                                 void *addr)
{
    HevTaskCIOSocket *self = HEV_TASK_CIO_SOCKET (base);
    socklen_t alen = self->alen;
    ssize_t res;

    res = recvfrom (self->fd, buf, count, 0, addr, &alen);
    base->err = errno;

    return res;
}

static ssize_t
_hev_task_cio_socket_write_dgram (HevTaskCIO *base, const void *buf,
                                  size_t count, const void *addr)
{
    HevTaskCIOSocket *self = HEV_TASK_CIO_SOCKET (base);
    ssize_t res;

    res = sendto (self->fd, buf, count, 0, addr, self->alen);
    base->err = errno;

    return res;
}

static ssize_t
_hev_task_cio_socket_readv_dgram (HevTaskCIO *base, const struct iovec *iov,
                                  int iovcnt, void *addr)
{
    HevTaskCIOSocket *self = HEV_TASK_CIO_SOCKET (base);
    struct msghdr mh = { 0 };
    ssize_t res;

    mh.msg_name = addr;
    mh.msg_namelen = self->alen;
    mh.msg_iov = (struct iovec *)iov;
    mh.msg_iovlen = iovcnt;

    res = recvmsg (self->fd, &mh, 0);
    base->err = errno;

    return res;
}

static ssize_t
_hev_task_cio_socket_writev_dgram (HevTaskCIO *base, const struct iovec *iov,
                                   int iovcnt, const void *addr)
{
    HevTaskCIOSocket *self = HEV_TASK_CIO_SOCKET (base);
    struct msghdr mh = { 0 };
    ssize_t res;

    mh.msg_name = (void *)addr;
    mh.msg_namelen = self->alen;
    mh.msg_iov = (struct iovec *)iov;
    mh.msg_iovlen = iovcnt;

    res = sendmsg (self->fd, &mh, 0);
    base->err = errno;

    return res;
}

static long
_hev_task_cio_socket_ctrl (HevTaskCIO *base, int ctrl, long larg, void *parg)
{
    HevTaskCIOSocket *self = HEV_TASK_CIO_SOCKET (base);

    switch (ctrl) {
    case HEV_TASK_CIO_CTRL_FLUSH:
        return 0;
    case HEV_TASK_CIO_CTRL_GET_FD:
        return self->fd;
    }

    return -1;
}

EXPORT_SYMBOL int
hev_task_cio_socket_construct (HevTaskCIOSocket *self, int family, int fd)
{
    int alen;
    int res;

    switch (family) {
    case AF_UNIX:
        alen = sizeof (struct sockaddr_un);
        break;
    case AF_INET:
        alen = sizeof (struct sockaddr_in);
        break;
    case AF_INET6:
        alen = sizeof (struct sockaddr_in6);
        break;
    default:
        alen = 0;
    }

    res = hev_task_cio_construct (&self->base);
    if (res < 0)
        return res;

    HEV_OBJECT (self)->klass = HEV_TASK_CIO_SOCKET_TYPE;

    self->alen = alen;
    self->fd = fd;

    return 0;
}

static void
_hev_task_cio_socket_destruct (HevObject *base)
{
    HevTaskCIOSocket *self = HEV_TASK_CIO_SOCKET (base);

    if (self->fd >= 0)
        close (self->fd);

    HEV_TASK_CIO_TYPE->destruct (base);
    hev_free (base);
}

EXPORT_SYMBOL HevObjectClass *
hev_task_cio_socket_class (void)
{
    static HevTaskCIOSocketClass klass;
    HevTaskCIOSocketClass *kptr = &klass;
    HevObjectClass *okptr = HEV_OBJECT_CLASS (kptr);
    HevTaskCIOClass *ckptr;

    if (okptr->name)
        return okptr;

    memcpy (kptr, HEV_TASK_CIO_TYPE, sizeof (HevTaskCIOClass));

    okptr->name = "HevTaskCIOSocket";
    okptr->destruct = _hev_task_cio_socket_destruct;

    ckptr = &kptr->base;
    ckptr->read = _hev_task_cio_socket_read;
    ckptr->write = _hev_task_cio_socket_write;
    ckptr->readv = _hev_task_cio_socket_readv;
    ckptr->writev = _hev_task_cio_socket_writev;
    ckptr->read_dgram = _hev_task_cio_socket_read_dgram;
    ckptr->write_dgram = _hev_task_cio_socket_write_dgram;
    ckptr->readv_dgram = _hev_task_cio_socket_readv_dgram;
    ckptr->writev_dgram = _hev_task_cio_socket_writev_dgram;
    ckptr->ctrl = _hev_task_cio_socket_ctrl;

    return okptr;
}
