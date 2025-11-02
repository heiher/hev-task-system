/*
 ============================================================================
 Name        : hev-task-io-ring.c
 Author      : Heiher <r@hev.cc>
 Copyright   : Copyright (c) 2025 everyone.
 Description : Task I/O Ring
 ============================================================================
 */

#if defined(__linux__) && defined(ENABLE_IO_RING)
#include <linux/io_uring.h>
#endif

#include "lib/misc/hev-compiler.h"
#include "kern/core/hev-task-system-private.h"

#include "hev-task-io-ring.h"

EXPORT_SYMBOL int
hev_task_io_ring_is_enabled (void)
{
#if defined(__linux__) && defined(ENABLE_IO_RING)
    return 1;
#else
    return 0;
#endif
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
    sqe->addr = (intptr_t)_cqe;
    hev_task_io_ring_submit (sqe, &cqe);

    while (cqe.user_data)
        hev_task_yield (HEV_TASK_WAITIO);
#endif
}
