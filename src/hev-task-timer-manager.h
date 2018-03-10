/*
 ============================================================================
 Name        : hev-task-timer-manager.h
 Author      : Heiher <r@hev.cc>
 Copyright   : Copyright (c) 2018 everyone.
 Description :
 ============================================================================
 */

#ifndef __HEV_TASK_TIMER_MANAGER_H__
#define __HEV_TASK_TIMER_MANAGER_H__

#include "hev-task.h"

typedef struct _HevTaskTimer HevTaskTimer;
typedef struct _HevTaskTimerManager HevTaskTimerManager;

HevTaskTimerManager * hev_task_timer_manager_new (void);
void hev_task_timer_manager_destroy (HevTaskTimerManager *self);

HevTaskTimer * hev_task_timer_manager_alloc (HevTaskTimerManager *self);
void hev_task_timer_manager_free (HevTaskTimerManager *self, HevTaskTimer *timer);

int hev_task_timer_get_fd (HevTaskTimer *timer);
void hev_task_timer_set_task (HevTaskTimer *timer, HevTask *task);

#endif /* __HEV_TASK_TIMER_MANAGER_H__ */

