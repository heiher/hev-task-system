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

#include <setjmp.h>

#include "hev-task.h"
#include "hev-task-private.h"
#include "hev-task-system.h"
#include "hev-task-timer-manager.h"
#include "hev-rbtree.h"

#define HEV_TASK_RUN_SCHEDULER HEV_TASK_YIELD_COUNT
#define PRIORITY_COUNT (HEV_TASK_PRIORITY_MAX - HEV_TASK_PRIORITY_MIN + 1)

typedef struct _HevTaskSystemContext HevTaskSystemContext;

struct _HevTaskSystemContext
{
    int epoll_fd;
    unsigned int total_task_count;
    unsigned int running_task_count;

    HevTaskTimerManager *timer_manager;

    HevTask *current_task;
    HevRBTree running_tasks;

    jmp_buf kernel_context;
};

void hev_task_system_schedule (HevTaskYieldType type);
void hev_task_system_wakeup_task (HevTask *task);
void hev_task_system_run_new_task (HevTask *task);
void hev_task_system_kill_current_task (void);

HevTaskSystemContext *hev_task_system_get_context (void);

#endif /* __HEV_TASK_SYSTEM_PRIVATE_H__ */
