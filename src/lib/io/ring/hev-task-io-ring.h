/*
 ============================================================================
 Name        : hev-task-io-ring.h
 Author      : Heiher <r@hev.cc>
 Copyright   : Copyright (c) 2025 everyone.
 Description : Task I/O Ring
 ============================================================================
 */

#ifndef __HEV_TASK_IO_RING_H__
#define __HEV_TASK_IO_RING_H__

#ifdef __cplusplus
extern "C" {
#endif

/**
 * hev_task_io_ring_is_enabled:
 *
 * Check whether I/O ring is enabled.
 *
 * Returns: a non-zero if enabled, or zero if not.
 *
 * Since: 5.11.0
 */
int hev_task_io_ring_is_enabled (void);

/**
 * hev_task_io_ring_get_sqe:
 *
 * Get the submission queue entry.
 *
 * Returns: a #io_uring_sqe
 *
 * Since: 5.11.0
 */
void *hev_task_io_ring_get_sqe (void);

/**
 * hev_task_io_ring_submit:
 * @sqe: a submission queue entry
 * @cqe: a completion queue entry
 *
 * Submit a submission queue entry.
 *
 * Since: 5.11.0
 */
void hev_task_io_ring_submit (void *sqe, void *cqe);

/**
 * hev_task_io_ring_cancel:
 * @cqe: a completion queue entry for cancellation
 *
 * Cancel a submission by its completion queue entry.
 *
 * Since: 5.11.0
 */
void hev_task_io_ring_cancel (void *cqe);

#ifdef __cplusplus
}
#endif

#endif /* __HEV_TASK_IO_RING_H__ */
