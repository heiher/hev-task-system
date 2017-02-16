/*
 ============================================================================
 Name        : hev-task-system-private.h
 Author      : Heiher <r@hev.cc>
 Copyright   : Copyright (c) 2017 everyone.
 Description :
 ============================================================================
 */

#ifndef __HEV_TASK_SYSTEM_PRIVATE_H__
#define __HEV_TASK_SYSTEM_PRIVATE_H__

#include "hev-task.h"
#include "hev-task-system.h"

void hev_task_system_schedule (HevTaskSystemYieldType type, HevTask *new_task);

HevTask * hev_task_system_get_current_task (void);
int hev_task_system_get_epoll_fd (void);

#endif /* __HEV_TASK_SYSTEM_PRIVATE_H__ */

