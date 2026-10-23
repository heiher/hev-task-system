/*
 ============================================================================
 Name        : hev-task-system-private.h
 Author      : Heiher <r@hev.cc>
 Copyright   : Copyright (c) 2017 - 2025 everyone.
 Description :
 ============================================================================
 */

#ifndef __HEV_TASK_SYSTEM_PRIVATE_H__
#define __HEV_TASK_SYSTEM_PRIVATE_H__

#include <time.h>
#include <setjmp.h>

/* The MSVC ABI targets use the architecture specific hev_task_longjmp backend
 * (src/arch), the other targets use the libc longjmp. */
#if defined(_MSC_VER) && \
    (defined(_M_X64) || defined(_M_IX86) || defined(_M_ARM64))
__declspec (noreturn) void hev_task_longjmp (void *env, int value);
#define HEV_TASK_LONGJMP(env, value) hev_task_longjmp ((void *)(env), (value))
#else
#define HEV_TASK_LONGJMP(env, value) _longjmp ((env), (value))
#endif

#include "hev-task-system.h"
#include "kern/task/hev-task.h"
#include "kern/task/hev-task-private.h"
#include "kern/time/hev-task-timer.h"
#include "kern/io/hev-task-io-reactor.h"
#include "lib/list/hev-list.h"
#include "lib/dns/hev-task-dns-proxy.h"
#include "lib/rbtree/hev-rbtree-cached.h"
#include "lib/misc/hev-task-stack-detector.h"

#define CLOCK_NONE (-1)
#define HEV_TASK_RUN_SCHEDULER HEV_TASK_YIELD_COUNT
#define PRIORITY_COUNT (HEV_TASK_PRIORITY_MAX - HEV_TASK_PRIORITY_MIN + 1)

typedef struct _HevTaskSystemContext HevTaskSystemContext;

enum
{
    HEV_TASK_SCHED_SWITCH = HEV_TASK_YIELD,
    HEV_TASK_SCHED_WAITIO = HEV_TASK_WAITIO,
    HEV_TASK_SCHED_REMOVE = HEV_TASK_YIELD_COUNT,
};

struct _HevTaskSystemContext
{
    unsigned int total_task_count;
    unsigned int running_task_count;

    HevTaskTimer *timer;
    HevTaskIOReactor *reactor;
    HevTaskDNSProxy *dns_proxy;
    HevTaskStackDetector *stack_detector;

    HevTask *current_task;
    HevRBTreeCached running_tasks;

    struct timespec sched_time;

    jmp_buf kernel_context;

    HevList all_tasks;
};

void hev_task_system_schedule (HevTaskYieldType type);
void hev_task_system_wakeup_task (HevTask *task);
void hev_task_system_run_new_task (HevTask *task);
void hev_task_system_kill_current_task (void);

void hev_task_system_wakeup_task_with_context (HevTaskSystemContext *ctx,
                                               HevTask *task);

HevTaskSystemContext *hev_task_system_get_context (void);

#endif /* __HEV_TASK_SYSTEM_PRIVATE_H__ */
