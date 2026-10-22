/*
 ============================================================================
 Name        : hev-task-io-ring.c
 Author      : Heiher <r@hev.cc>
 Copyright   : Copyright (c) 2025 everyone.
 Description : Task I/O Ring
 ============================================================================
 */

#if defined(__linux__) && defined(ENABLE_IO_RING)
#include <poll.h>
#include <linux/io_uring.h>
#endif

#include "kern/task/hev-task.h"
#include "lib/io/basic/hev-task-io.h"
#include "lib/misc/hev-compiler.h"
#include "kern/core/hev-task-system-private.h"

#include "hev-task-io-ring.h"

EXPORT_SYMBOL int
hev_task_io_ring_is_enabled (void)
{
#if defined(__linux__) && defined(ENABLE_IO_RING)
    HevTaskSystemContext *ctx;

    ctx = hev_task_system_get_context ();

    return ctx && !!ctx->uring;
#else
    return 0;
#endif /* !defined(__linux__) || !defined(ENABLE_IO_RING) */
}

EXPORT_SYMBOL void *
hev_task_io_ring_get_sqe (void)
{
#if defined(__linux__) && defined(ENABLE_IO_RING)
    HevTaskSystemContext *ctx;
    struct io_uring_sqe *sqe;

    ctx = hev_task_system_get_context ();
    sqe = hev_task_io_uring_get_sqe (ctx->uring);
    sqe->user_data = (intptr_t)ctx->uring;

    return sqe;
#else
    return NULL;
#endif /* !defined(__linux__) || !defined(ENABLE_IO_RING) */
}

EXPORT_SYMBOL void
hev_task_io_ring_submit (void *_sqe, void *cqe)
{
#if defined(__linux__) && defined(ENABLE_IO_RING)
    struct io_uring_sqe *sqe;
    HevTaskIOURing *uring;

    sqe = _sqe;
    uring = (void *)sqe->user_data;
    hev_task_io_uring_submit (uring, sqe, cqe);
#endif
}

EXPORT_SYMBOL int
hev_task_io_ring_poll (int fd, unsigned int events,
                       HevTaskIOYielder yielder, void *yielder_data)
{
#if defined(__linux__) && defined(ENABLE_IO_RING)
    struct io_uring_sqe *sqe;
    struct io_uring_cqe cqe;

    sqe = hev_task_io_ring_get_sqe ();
    sqe->opcode = IORING_OP_POLL_ADD;
    sqe->fd = fd;
    sqe->poll_events = events;
    hev_task_io_ring_submit (sqe, &cqe);

    while (cqe.user_data) {
        if (yielder) {
            if (yielder (HEV_TASK_WAITIO, yielder_data)) {
                hev_task_io_ring_cancel (&cqe);
                return -2;
            }
        } else {
            hev_task_yield (HEV_TASK_WAITIO);
        }
    }

    if (cqe.res < 0)
        return -1;

    return 0;
#else
    return -1;
#endif /* !defined(__linux__) || !defined(ENABLE_IO_RING) */
}

EXPORT_SYMBOL void
hev_task_io_ring_cancel (void *_cqe)
{
#if defined(__linux__) && defined(ENABLE_IO_RING)
    struct io_uring_sqe *sqe;
    struct io_uring_cqe cqe;

    if (!((struct io_uring_cqe *)_cqe)->user_data)
        return;

    sqe = hev_task_io_ring_get_sqe ();
    sqe->opcode = IORING_OP_ASYNC_CANCEL;
    sqe->fd = -1;
    sqe->addr = (intptr_t)_cqe;
    hev_task_io_ring_submit (sqe, &cqe);

    while (cqe.user_data)
        hev_task_yield (HEV_TASK_WAITIO);
#endif
}
