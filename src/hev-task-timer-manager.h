/*
 ============================================================================
 Name        : hev-task-timer-manager.h
 Author      : Heiher <r@hev.cc>
 Copyright   : Copyright (c) 2018 everyone.
 Description :
 ============================================================================
 */

#ifndef __HEV_TASK_TIMER_MANAGER_H__
#define __HEV_TASK_TIMER_MAANGER_H__

typedef struct _HevTaskTimer HevTaskTimer;
typedef struct _HevTaskTimerManager HevTaskTimerManager;

HevTaskTimerManager * hev_task_timer_manager_new (void);
void hev_task_timer_manager_destroy (HevTaskTimerManager *self);

HevTaskTimer * hev_task_timer_manager_alloc (HevTaskTimerManager *self);
void hev_task_timer_manager_free (HevTaskTimerManager *self, HevTaskTimer *timer);

int hev_task_timer_get_fd (HevTaskTimer *timer);

#endif /* __HEV_TASK_TIMER_MAANGER_H__ */

