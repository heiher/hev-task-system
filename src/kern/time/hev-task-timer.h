/*
 ============================================================================
 Name        : hev-task-timer.h
 Author      : Heiher <r@hev.cc>
 Copyright   : Copyright (c) 2018 everyone.
 Description : Timer
 ============================================================================
 */

#ifndef __HEV_TASK_TIMER_H__
#define __HEV_TASK_TIMER_H__

#include "kern/task/hev-task-private.h"
#include "lib/rbtree/hev-rbtree-cached.h"

typedef struct _HevTaskTimer HevTaskTimer;

struct _HevTaskTimer
{
    HevRBTreeCached sort_tree;
    HevTaskSchedEntity sched_entity;
};

HevTaskTimer *hev_task_timer_new (void);
void hev_task_timer_destroy (HevTaskTimer *self);

unsigned int hev_task_timer_wait (HevTaskTimer *self, unsigned int milliseconds,
                                  HevTask *task);

#endif /* __HEV_TASK_TIMER_H__ */
