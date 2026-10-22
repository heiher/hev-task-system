/*
 ============================================================================
 Name        : hev-task-io.c
 Author      : Heiher <r@hev.cc>
 Copyright   : Copyright (c) 2018 everyone.
 Description : Task I/O operations
 ============================================================================
 */

#if !defined(__linux__) && defined(ENABLE_IO_SPLICE_SYSCALL)
#undef ENABLE_IO_SPLICE_SYSCALL
#endif /* !defined(__linux__) && ENABLE_IO_SPLICE_SYSCALL */

#ifdef ENABLE_IO_SPLICE_SYSCALL
#define _GNU_SOURCE
#endif /* ENABLE_IO_SPLICE_SYSCALL */

#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdarg.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <sys/socket.h>

#if defined(__linux__) && defined(ENABLE_IO_RING)
#include <linux/io_uring.h>
#endif

#include "kern/task/hev-task.h"
#include "lib/io/buffer/hev-circular-buffer.h"
#include "lib/misc/hev-compiler.h"

#include "hev-task-io.h"
#include "lib/io/ring/hev-task-io-ring.h"

typedef struct _HevTaskIOSplicer HevTaskIOSplicer;

struct _HevTaskIOSplicer
{
#ifdef ENABLE_IO_SPLICE_SYSCALL
    int fd[2];
    size_t wlen;
    size_t blen;
#else
    HevCircularBuffer *buf;
#endif /* !ENABLE_IO_SPLICE_SYSCALL */

#if defined(__linux__) && defined(ENABLE_IO_RING)
    struct io_uring_cqe cqe[2];
    struct iovec iov[2][2];
    int missions[2];
    int dead[2];
    int eagain[2];
#endif
};

EXPORT_SYMBOL int
hev_task_io_open (const char *pathname, int flags, ...)
{
    flags |= O_NONBLOCK;

    if (O_CREAT & flags) {
        int fd;
        va_list ap;

        va_start (ap, flags);
        fd = open (pathname, flags, va_arg (ap, int));
        va_end (ap);

        return fd;
    }

    return open (pathname, flags);
}

EXPORT_SYMBOL int
hev_task_io_creat (const char *pathname, mode_t mode)
{
    return hev_task_io_open (pathname, O_CREAT | O_WRONLY | O_TRUNC, mode);
}

EXPORT_SYMBOL int
hev_task_io_openat (int dirfd, const char *pathname, int flags, ...)
{
    flags |= O_NONBLOCK;

    if (O_CREAT & flags) {
        int fd;
        va_list ap;

        va_start (ap, flags);
        fd = openat (dirfd, pathname, flags, va_arg (ap, int));
        va_end (ap);

        return fd;
    }

    return openat (dirfd, pathname, flags);
}

EXPORT_SYMBOL int
hev_task_io_dup (int oldfd)
{
    int newfd;
    int nonblock = 1;

    newfd = dup (oldfd);
    if (0 > newfd)
        return -1;

    if (0 > ioctl (newfd, FIONBIO, (char *)&nonblock)) {
        close (newfd);
        return -2;
    }

    return newfd;
}

EXPORT_SYMBOL int
hev_task_io_dup2 (int oldfd, int newfd)
{
    int nonblock = 1;

    newfd = dup2 (oldfd, newfd);
    if (0 > newfd)
        return -1;

    if (0 > ioctl (newfd, FIONBIO, (char *)&nonblock)) {
        close (newfd);
        return -2;
    }

    return newfd;
}

EXPORT_SYMBOL ssize_t
hev_task_io_read (int fd, void *buf, size_t count, HevTaskIOYielder yielder,
                  void *yielder_data)
{
#if defined(__linux__) && defined(ENABLE_IO_RING)
    struct io_uring_sqe *sqe;
    struct io_uring_cqe cqe;
    int res;

    if (hev_task_io_ring_is_enabled ()) {
again:
        sqe = hev_task_io_ring_get_sqe ();
        sqe->opcode = IORING_OP_READ;
        sqe->fd = fd;
        sqe->off = 0;
        sqe->addr = (intptr_t)buf;
        sqe->len = count;
        hev_task_io_ring_submit (sqe, &cqe);

wait:
        if (yielder) {
            if (yielder (HEV_TASK_WAITIO, yielder_data)) {
                hev_task_io_ring_cancel (&cqe);
                return -2;
            }
        } else {
            hev_task_yield (HEV_TASK_WAITIO);
        }
        if (cqe.user_data)
            goto wait;

        if (cqe.res != -EAGAIN) {
            if (cqe.res < 0) {
                errno = -cqe.res;
                return -1;
            }

            return cqe.res;
        }

        res = hev_task_io_ring_poll (fd, POLLIN, yielder, yielder_data);
        if (res < 0)
            return res;

        goto again;
    }
#endif

    ssize_t s;

retry:
    s = read (fd, buf, count);
    if (s < 0 && errno == EAGAIN) {
        if (yielder) {
            if (yielder (HEV_TASK_WAITIO, yielder_data))
                return -2;
        } else {
            hev_task_yield (HEV_TASK_WAITIO);
        }
        goto retry;
    }

    return s;
}

EXPORT_SYMBOL ssize_t
hev_task_io_readv (int fd, const struct iovec *iov, int iovcnt,
                   HevTaskIOYielder yielder, void *yielder_data)
{
#if defined(__linux__) && defined(ENABLE_IO_RING)
    struct io_uring_sqe *sqe;
    struct io_uring_cqe cqe;
    int res;

    if (hev_task_io_ring_is_enabled ()) {
again:
        sqe = hev_task_io_ring_get_sqe ();
        sqe->opcode = IORING_OP_READV;
        sqe->fd = fd;
        sqe->off = 0;
        sqe->addr = (intptr_t)iov;
        sqe->len = iovcnt;
        hev_task_io_ring_submit (sqe, &cqe);

wait:
        if (yielder) {
            if (yielder (HEV_TASK_WAITIO, yielder_data)) {
                hev_task_io_ring_cancel (&cqe);
                return -2;
            }
        } else {
            hev_task_yield (HEV_TASK_WAITIO);
        }
        if (cqe.user_data)
            goto wait;

        if (cqe.res != -EAGAIN) {
            if (cqe.res < 0) {
                errno = -cqe.res;
                return -1;
            }

            return cqe.res;
        }

        res = hev_task_io_ring_poll (fd, POLLIN, yielder, yielder_data);
        if (res < 0)
            return res;

        goto again;
    }
#endif

    ssize_t s;

retry:
    s = readv (fd, iov, iovcnt);
    if (s < 0 && errno == EAGAIN) {
        if (yielder) {
            if (yielder (HEV_TASK_WAITIO, yielder_data))
                return -2;
        } else {
            hev_task_yield (HEV_TASK_WAITIO);
        }
        goto retry;
    }

    return s;

}

EXPORT_SYMBOL ssize_t
hev_task_io_write (int fd, const void *buf, size_t count,
                   HevTaskIOYielder yielder, void *yielder_data)
{
#if defined(__linux__) && defined(ENABLE_IO_RING)
    struct io_uring_sqe *sqe;
    struct io_uring_cqe cqe;
    int res;

    if (hev_task_io_ring_is_enabled ()) {
again:
        sqe = hev_task_io_ring_get_sqe ();
        sqe->opcode = IORING_OP_WRITE;
        sqe->fd = fd;
        sqe->off = 0;
        sqe->addr = (intptr_t)buf;
        sqe->len = count;
        hev_task_io_ring_submit (sqe, &cqe);

wait:
        if (yielder) {
            if (yielder (HEV_TASK_WAITIO, yielder_data)) {
                hev_task_io_ring_cancel (&cqe);
                return -2;
            }
        } else {
            hev_task_yield (HEV_TASK_WAITIO);
        }
        if (cqe.user_data)
            goto wait;

        if (cqe.res != -EAGAIN) {
            if (cqe.res < 0) {
                errno = -cqe.res;
                return -1;
            }

            return cqe.res;
        }

        res = hev_task_io_ring_poll (fd, POLLOUT, yielder, yielder_data);
        if (res < 0)
            return res;

        goto again;
    }
#endif

    ssize_t s;

retry:
    s = write (fd, buf, count);
    if (s < 0 && errno == EAGAIN) {
        if (yielder) {
            if (yielder (HEV_TASK_WAITIO, yielder_data))
                return -2;
        } else {
            hev_task_yield (HEV_TASK_WAITIO);
        }
        goto retry;
    }

    return s;

}

EXPORT_SYMBOL ssize_t
hev_task_io_writev (int fd, const struct iovec *iov, int iovcnt,
                    HevTaskIOYielder yielder, void *yielder_data)
{
#if defined(__linux__) && defined(ENABLE_IO_RING)
    struct io_uring_sqe *sqe;
    struct io_uring_cqe cqe;
    int res;

    if (hev_task_io_ring_is_enabled ()) {
again:
        sqe = hev_task_io_ring_get_sqe ();
        sqe->opcode = IORING_OP_WRITEV;
        sqe->fd = fd;
        sqe->off = 0;
        sqe->addr = (intptr_t)iov;
        sqe->len = iovcnt;
        hev_task_io_ring_submit (sqe, &cqe);

wait:
        if (yielder) {
            if (yielder (HEV_TASK_WAITIO, yielder_data)) {
                hev_task_io_ring_cancel (&cqe);
                return -2;
            }
        } else {
            hev_task_yield (HEV_TASK_WAITIO);
        }
        if (cqe.user_data)
            goto wait;

        if (cqe.res != -EAGAIN) {
            if (cqe.res < 0) {
                errno = -cqe.res;
                return -1;
            }

            return cqe.res;
        }

        res = hev_task_io_ring_poll (fd, POLLOUT, yielder, yielder_data);
        if (res < 0)
            return res;

        goto again;
    }
#endif

    ssize_t s;

retry:
    s = writev (fd, iov, iovcnt);
    if (s < 0 && errno == EAGAIN) {
        if (yielder) {
            if (yielder (HEV_TASK_WAITIO, yielder_data))
                return -2;
        } else {
            hev_task_yield (HEV_TASK_WAITIO);
        }
        goto retry;
    }

    return s;

}

#ifdef ENABLE_IO_SPLICE_SYSCALL

static int
task_io_splicer_init (HevTaskIOSplicer *self, size_t buf_size)
{
    HevTask *task = hev_task_self ();
    int res;

    res = pipe2 (self->fd, O_NONBLOCK);
    if (res < 0)
        goto exit;

    res = hev_task_add_fd (task, self->fd[0], POLLIN);
    if (res < 0)
        goto exit_close;

    res = hev_task_add_fd (task, self->fd[1], POLLOUT);
    if (res < 0)
        goto exit_close;

#ifdef F_GETPIPE_SZ
    buf_size = fcntl (self->fd[0], F_GETPIPE_SZ);
#endif

#if defined(__linux__) && defined(ENABLE_IO_RING)
    self->missions[0] = 0;
    self->missions[1] = 0;
    self->dead[0] = 0;
    self->dead[1] = 0;
    self->eagain[0] = 0;
    self->eagain[1] = 0;
#endif

    self->wlen = 0;
    self->blen = buf_size;

    return 0;

exit_close:
    close (self->fd[0]);
    close (self->fd[1]);
exit:
    return res;
}

static void
task_io_splicer_fini (HevTaskIOSplicer *self)
{
#if defined(__linux__) && defined(ENABLE_IO_RING)
    hev_task_io_ring_cancel (&self->cqe[0]);
    hev_task_io_ring_cancel (&self->cqe[1]);
#endif

    close (self->fd[0]);
    close (self->fd[1]);
}

static int
task_io_splice (HevTaskIOSplicer *self, int fd_in, int fd_out)
{
#if defined(__linux__) && defined(ENABLE_IO_RING)
    struct io_uring_sqe *sqe;
    int res = 0, i;

    /* Each mission keeps exactly one operation in flight, so that the
     * completions drive this task forward.  A mission that would block
     * switches its slot to a readiness poll instead of resubmitting a
     * splice that would only return EAGAIN again. */
    for (i = 0; i < 2; i++) {
        if (!self->missions[i] || self->cqe[i].user_data)
            continue;

        self->missions[i] = 0;
        if (self->eagain[i]) {
            /* The readiness poll completed. */
            self->eagain[i] = 0;
            if (self->cqe[i].res < 0)
                self->dead[i] = 1;

            continue;
        }

        if (self->cqe[i].res > 0) {
            self->wlen += i ? -self->cqe[i].res : self->cqe[i].res;
            res = 1;
        } else if (self->cqe[i].res == -EAGAIN) {
            self->eagain[i] = 1;
        } else {
            self->dead[i] = 1;
        }
    }

    for (i = 0; i < 2; i++) {
        if (self->missions[i] || self->dead[i])
            continue;

        if (self->eagain[i]) {
            sqe = hev_task_io_ring_get_sqe ();
            sqe->opcode = IORING_OP_POLL_ADD;
            sqe->fd = i ? fd_out : fd_in;
            sqe->poll_events = i ? POLLOUT : POLLIN;
            hev_task_io_ring_submit (sqe, &self->cqe[i]);
            self->missions[i] = 1;
        } else if (i == 0) {
            if (self->wlen >= self->blen)
                continue;

            sqe = hev_task_io_ring_get_sqe ();
            sqe->opcode = IORING_OP_SPLICE;
            sqe->fd = self->fd[1];
            sqe->off = -1;
            sqe->splice_off_in = -1;
            sqe->len = self->blen;
            sqe->splice_fd_in = fd_in;
            sqe->splice_flags = SPLICE_F_MOVE;
            hev_task_io_ring_submit (sqe, &self->cqe[0]);
            self->missions[0] = 1;
        } else if (self->wlen > 0) {
            sqe = hev_task_io_ring_get_sqe ();
            sqe->opcode = IORING_OP_SPLICE;
            sqe->fd = fd_out;
            sqe->off = -1;
            sqe->splice_off_in = -1;
            sqe->len = self->blen;
            sqe->splice_fd_in = self->fd[0];
            sqe->splice_flags = SPLICE_F_MOVE;
            hev_task_io_ring_submit (sqe, &self->cqe[1]);
            self->missions[1] = 1;
        }
    }

    if (self->dead[1])
        return -1;

    if (self->dead[0] && self->wlen <= 0 && !self->missions[1]) {
        shutdown (fd_out, SHUT_WR);
        return -1;
    }

    return res;
#else
    int res;
    ssize_t s;

    s = splice (fd_in, NULL, self->fd[1], NULL, self->blen,
                SPLICE_F_MOVE | SPLICE_F_NONBLOCK);
    if (0 >= s) {
        if ((0 > s) && (EAGAIN == errno))
            res = 0;
        else
            res = -1;
    } else {
        res = 1;
        self->wlen += s;
    }

    if (self->wlen) {
        s = splice (self->fd[0], NULL, fd_out, NULL, self->blen,
                    SPLICE_F_MOVE | SPLICE_F_NONBLOCK);
        if (0 >= s) {
            if ((0 > s) && (EAGAIN == errno))
                res = 0;
            else
                res = -1;
        } else {
            res = 1;
            self->wlen -= s;
        }
    } else if (res < 0) {
        shutdown (fd_out, SHUT_WR);
    }

    return res;
#endif /* !defined(__linux__) || !defined(ENABLE_IO_RING) */
}

#else

static int
task_io_splicer_init (HevTaskIOSplicer *self, size_t buf_size)
{
    self->buf = hev_circular_buffer_new (buf_size);
    if (!self->buf)
        return -1;

#if defined(__linux__) && defined(ENABLE_IO_RING)
    self->missions[0] = 0;
    self->missions[1] = 0;
    self->dead[0] = 0;
    self->dead[1] = 0;
    self->eagain[0] = 0;
    self->eagain[1] = 0;
#endif

    return 0;
}

static void
task_io_splicer_fini (HevTaskIOSplicer *self)
{
#if defined(__linux__) && defined(ENABLE_IO_RING)
    hev_task_io_ring_cancel (&self->cqe[0]);
    hev_task_io_ring_cancel (&self->cqe[1]);
#endif

    if (self->buf)
        hev_circular_buffer_unref (self->buf);
}

static int
task_io_splice (HevTaskIOSplicer *self, int fd_in, int fd_out)
{
#if defined(__linux__) && defined(ENABLE_IO_RING)
    struct io_uring_sqe *sqe;
    int res = 0, iovc, i;

    /* Each mission keeps exactly one operation in flight, so that the
     * completions drive this task forward.  A mission that would block
     * switches its slot to a readiness poll instead of resubmitting a
     * read or write that would only return EAGAIN again. */
    for (i = 0; i < 2; i++) {
        if (!self->missions[i] || self->cqe[i].user_data)
            continue;

        self->missions[i] = 0;
        if (self->eagain[i]) {
            /* The readiness poll completed. */
            self->eagain[i] = 0;
            if (self->cqe[i].res < 0)
                self->dead[i] = 1;

            continue;
        }

        if (self->cqe[i].res > 0) {
            if (i)
                hev_circular_buffer_read_finish (self->buf,
                                                 self->cqe[i].res);
            else
                hev_circular_buffer_write_finish (self->buf,
                                                  self->cqe[i].res);
            res = 1;
        } else if (self->cqe[i].res == -EAGAIN) {
            self->eagain[i] = 1;
        } else {
            self->dead[i] = 1;
        }
    }

    for (i = 0; i < 2; i++) {
        if (self->missions[i] || self->dead[i])
            continue;

        if (self->eagain[i]) {
            sqe = hev_task_io_ring_get_sqe ();
            sqe->opcode = IORING_OP_POLL_ADD;
            sqe->fd = i ? fd_out : fd_in;
            sqe->poll_events = i ? POLLOUT : POLLIN;
            hev_task_io_ring_submit (sqe, &self->cqe[i]);
            self->missions[i] = 1;

            continue;
        }

        iovc = i ? hev_circular_buffer_reading (self->buf, self->iov[1])
                 : hev_circular_buffer_writing (self->buf, self->iov[0]);
        if (!iovc)
            continue;

        sqe = hev_task_io_ring_get_sqe ();
        sqe->opcode = i ? IORING_OP_WRITEV : IORING_OP_READV;
        sqe->fd = i ? fd_out : fd_in;
        sqe->off = 0;
        sqe->addr = (intptr_t)self->iov[i];
        sqe->len = iovc;
        hev_task_io_ring_submit (sqe, &self->cqe[i]);
        self->missions[i] = 1;
    }

    if (self->dead[1]) {
        shutdown (fd_out, SHUT_WR);
        return -1;
    }

    iovc = hev_circular_buffer_reading (self->buf, self->iov[1]);
    if (self->dead[0] && !iovc && !self->missions[1]) {
        shutdown (fd_out, SHUT_WR);
        return -1;
    }

    return res;
#else
    struct iovec iov[2];
    int res = 1, iovc;

    iovc = hev_circular_buffer_writing (self->buf, iov);
    if (iovc) {
        ssize_t s = readv (fd_in, iov, iovc);
        if (0 >= s) {
            if ((0 > s) && (EAGAIN == errno))
                res = 0;
            else
                res = -1;
        } else {
            hev_circular_buffer_write_finish (self->buf, s);
        }
    }

    iovc = hev_circular_buffer_reading (self->buf, iov);
    if (iovc) {
        ssize_t s = writev (fd_out, iov, iovc);
        if (0 >= s) {
            if ((0 > s) && (EAGAIN == errno))
                res = 0;
            else
                res = -1;
        } else {
            res = 1;
            hev_circular_buffer_read_finish (self->buf, s);
        }
    } else if (res < 0) {
        shutdown (fd_out, SHUT_WR);
    }

    return res;
#endif /* !defined(__linux__) || !defined(ENABLE_IO_RING) */
}

#endif /* !ENABLE_IO_SPLICE_SYSCALL */

EXPORT_SYMBOL void
hev_task_io_splice (int fd_a_i, int fd_a_o, int fd_b_i, int fd_b_o,
                    size_t buf_size, HevTaskIOYielder yielder,
                    void *yielder_data)
{
    HevTaskIOSplicer splicer_f;
    HevTaskIOSplicer splicer_b;
    int res_f = 1;
    int res_b = 1;

    if (task_io_splicer_init (&splicer_f, buf_size) < 0)
        return;
    if (task_io_splicer_init (&splicer_b, buf_size) < 0)
        goto exit;

    for (;;) {
        HevTaskYieldType type;

        if (res_f >= 0)
            res_f = task_io_splice (&splicer_f, fd_a_i, fd_b_o);
        if (res_b >= 0)
            res_b = task_io_splice (&splicer_b, fd_b_i, fd_a_o);

        if (res_f > 0 || res_b > 0)
            type = HEV_TASK_YIELD;
        else if ((res_f & res_b) == 0)
            type = HEV_TASK_WAITIO;
        else
            break;

        if (yielder) {
            if (yielder (type, yielder_data))
                break;
        } else {
            hev_task_yield (type);
        }
    }

    task_io_splicer_fini (&splicer_b);
exit:
    task_io_splicer_fini (&splicer_f);
}
