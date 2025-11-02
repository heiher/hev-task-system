/*
 ============================================================================
 Name        : hev-task-io-uring.h
 Author      : hev <r@hev.cc>
 Copyright   : Copyright (c) 2025 everyone.
 Description : I/O URing
 ============================================================================
 */

#ifndef __HEV_TASK_IO_URING_H__
#define __HEV_TASK_IO_URING_H__

#if defined(__linux__) && defined(ENABLE_IO_RING)
#include <linux/io_uring.h>
#endif

typedef struct _HevTaskIOURing HevTaskIOURing;

HevTaskIOURing *hev_task_io_uring_new (void *ctx);
void hev_task_io_uring_destroy (HevTaskIOURing *self);

void *hev_task_io_uring_get_sqe (HevTaskIOURing *self);
void hev_task_io_uring_submit (HevTaskIOURing *self, void *sqe, void *cqe);

void hev_task_io_uring_reap (HevTaskIOURing *self);

#endif /* __HEV_TASK_IO_URING_H__ */
