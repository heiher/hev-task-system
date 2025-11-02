/*
 ============================================================================
 Name        : hev-task-io-uring.c
 Author      : hev <r@hev.cc>
 Copyright   : Copyright (c) 2025 everyone.
 Description : I/O URing
 ============================================================================
 */

#if defined(__linux__) && defined(ENABLE_IO_RING)

#include <errno.h>
#include <unistd.h>
#include <stdint.h>
#include <stdatomic.h>
#include <sys/mman.h>
#include <sys/syscall.h>

#include "kern/core/hev-task-system-private.h"
#include "mem/api/hev-memory-allocator-api.h"

#include "kern/io/hev-task-io-uring.h"

struct _HevTaskIOURing
{
    HevTaskSystemContext *ctx;

    uint32_t *sq_head;
    uint32_t *sq_tail;
    uint32_t *sq_flags;
    uint32_t *cq_head;
    uint32_t *cq_tail;

    uint32_t sq_mask;
    uint32_t cq_mask;
    uint32_t sqr_len;
    uint32_t sqe_len;

    void *sqr;
    void *sqe;
    void *cqe;

    int fd;
};

static HevTask dummy_task = { .state = HEV_TASK_RUNNING,
                              .sched_entity.task = &dummy_task };

static int
hev_io_uring_setup (int entries, struct io_uring_params *params)
{
    return syscall (__NR_io_uring_setup, entries, params);
}

static int
hev_io_uring_enter (int fd, unsigned to_submit, unsigned min_complete,
                    unsigned flags)
{
    return syscall (__NR_io_uring_enter, fd, to_submit, min_complete, flags,
                    NULL, 0L);
}

HevTaskIOURing *
hev_task_io_uring_new (void *ctx)
{
    HevTaskIOReactorSetupEvent revents[HEV_TASK_IO_REACTOR_EVENT_GEN_MAX];
    struct io_uring_params p = { 0 };
    int sq_len, cq_len, res;
    HevTaskIOURing *self;

    self = hev_malloc0 (sizeof (HevTaskIOURing));
    if (!self)
        goto exit;

    p.flags = IORING_SETUP_SQPOLL | IORING_SETUP_NO_SQARRAY;
    p.sq_thread_idle = 10;

    self->fd = hev_io_uring_setup (128, &p);
    if (self->fd < 0)
        goto exit_free;

    sq_len = p.sq_off.array + p.sq_entries * sizeof (uint32_t);
    cq_len = p.cq_off.cqes + p.cq_entries * sizeof (struct io_uring_cqe);
    self->sqr_len = sq_len < cq_len ? cq_len : sq_len;
    self->sqe_len = p.sq_entries * sizeof (struct io_uring_sqe);

    self->sqr = mmap (NULL, self->sqr_len, PROT_READ | PROT_WRITE,
                      MAP_SHARED | MAP_POPULATE, self->fd, IORING_OFF_SQ_RING);
    if (self->sqr == MAP_FAILED)
        goto exit_free;

    self->sqe = mmap (NULL, self->sqe_len, PROT_READ | PROT_WRITE,
                      MAP_SHARED | MAP_POPULATE, self->fd, IORING_OFF_SQES);
    if (self->sqe == MAP_FAILED)
        goto exit_unmap;

    self->ctx = ctx;
    res = hev_task_io_reactor_setup_event_fd_gen (revents, self->fd,
                                                  HEV_TASK_IO_REACTOR_OP_ADD,
                                                  POLLIN,
                                                  &dummy_task.sched_entity);
    res = hev_task_io_reactor_setup (self->ctx->reactor, revents, res);
    if (res < 0)
        goto exit_unmap;

    self->sq_head = self->sqr + p.sq_off.head;
    self->sq_tail = self->sqr + p.sq_off.tail;
    self->sq_flags = self->sqr + p.sq_off.flags;
    self->cq_head = self->sqr + p.cq_off.head;
    self->cq_tail = self->sqr + p.cq_off.tail;
    self->cqe = self->sqr + p.cq_off.cqes;

    self->sq_mask = *(uint32_t *)(self->sqr + p.sq_off.ring_mask);
    self->cq_mask = *(uint32_t *)(self->sqr + p.cq_off.ring_mask);

    return self;

exit_unmap:
    munmap (self->sqr, self->sqr_len);
exit_free:
    hev_free (self);
exit:
    return NULL;
}

void
hev_task_io_uring_destroy (HevTaskIOURing *self)
{
    munmap (self->sqr, self->sqr_len);
    munmap (self->sqe, self->sqe_len);
    close (self->fd);
    hev_free (self);
}

void *
hev_task_io_uring_get_sqe (HevTaskIOURing *self)
{
    uint32_t head, tail, mask, slot;
    struct io_uring_sqe *sqe;

    mask = self->sq_mask;

retry:
    head = atomic_load_explicit ((_Atomic uint32_t *)self->sq_head,
                                 memory_order_acquire);
    tail = *self->sq_tail;

    if ((head & mask) == ((tail + 1) & mask)) {
        hev_task_yield (HEV_TASK_YIELD);
        goto retry;
    }

    slot = tail & mask;
    sqe = self->sqe;
    sqe = &sqe[slot];

    sqe->flags = 0;
    sqe->ioprio = 0;
    sqe->rw_flags = 0;
    sqe->buf_index = 0;
    sqe->personality = 0;
    sqe->file_index = 0;
    sqe->addr3 = 0;
    sqe->__pad2[0] = 0;

    return sqe;
}

void
hev_task_io_uring_submit (HevTaskIOURing *self, void *_sqe, void *_cqe)
{
    struct io_uring_sqe *sqe = _sqe;
    struct io_uring_cqe *cqe = _cqe;

    sqe->user_data = (intptr_t)cqe;
    cqe->user_data = (intptr_t)self->ctx->current_task;

    atomic_store_explicit ((_Atomic uint32_t *)self->sq_tail,
                           *self->sq_tail + 1, memory_order_release);
    atomic_thread_fence (memory_order_seq_cst);

    if (*self->sq_flags & IORING_SQ_NEED_WAKEUP)
        hev_io_uring_enter (self->fd, 0, 0, IORING_ENTER_SQ_WAKEUP);
}

void
hev_task_io_uring_reap (HevTaskIOURing *self)
{
    uint32_t head, tail, mask, i;
    struct io_uring_cqe *cqe;

    cqe = self->cqe;
    mask = self->cq_mask;
    head = *self->cq_head;
    tail = atomic_load_explicit ((_Atomic uint32_t *)self->cq_tail,
                                 memory_order_acquire);

    for (i = head; i != tail; i++) {
        struct io_uring_cqe *es;
        struct io_uring_cqe *ed;
        HevTask *task;

        es = &cqe[i & mask];
        ed = (void *)es->user_data;
        task = (void *)ed->user_data;

        ed->flags = es->flags;
        ed->res = es->res;
        ed->user_data = 0;

        hev_task_system_wakeup_task_with_context (self->ctx, task);
    }

    atomic_store_explicit ((_Atomic uint32_t *)self->cq_head, tail,
                           memory_order_release);
    atomic_thread_fence (memory_order_seq_cst);

    if (*self->sq_flags & IORING_SQ_CQ_OVERFLOW)
        hev_io_uring_enter (self->fd, 0, 0, IORING_ENTER_GETEVENTS);
}

#endif /* defined(__linux__) && defined(ENABLE_IO_RING) */
