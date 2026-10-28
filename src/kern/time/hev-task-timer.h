/*
 ============================================================================
 Name        : hev-task-timer.h
 Author      : hev <r@hev.cc>
 Copyright   : Copyright (c) 2018 - 2026 hev
 Description : Timer
 ============================================================================
 */

#ifndef __HEV_TASK_TIMER_H__
#define __HEV_TASK_TIMER_H__

#include <time.h>
#include <stdint.h>

#include <hev-task.h>

#include "lib/rbtree/hev-rbtree-cached.h"

typedef struct _HevTaskTimer HevTaskTimer;

HevTaskTimer *hev_task_timer_new (void *ctx);
void hev_task_timer_destroy (HevTaskTimer *self);

int hev_task_timer_get_timeout (HevTaskTimer *self);
void hev_task_timer_wake (HevTaskTimer *self);

/* Returns whether the next non-blocking I/O poll is due: its deadline
 * has been reached, or the nearest task timer expires before it (so
 * that timer wakeups keep their precision). */
int hev_task_timer_poll_due (HevTaskTimer *self);

/* Re-arms the non-blocking I/O poll deadline. */
void hev_task_timer_poll_rearm (HevTaskTimer *self, uint64_t interval);

unsigned int hev_task_timer_wait (HevTaskTimer *self, unsigned int milliseconds,
                                  HevTask *task);

#endif /* __HEV_TASK_TIMER_H__ */
