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

#if defined(_MSC_VER) && defined(_M_ARM64EC)
#error "ARM64EC is not supported: it uses the x64-compatible ABI, _JUMP_BUFFER layout and unwind model, which differ from native ARM64 and have not been independently validated. Do not substitute the x64 or ARM64 backend."
#endif

#if defined(_MSC_VER) && !defined(_M_X64) && !defined(_M_IX86) && \
    !defined(_M_ARM64)
#error "Native MSVC coroutine backends exist only for x64, x86 and native ARM64."
#endif

#if defined(_MSC_VER) && (defined(_M_X64) || defined(_M_IX86) || \
                          defined(_M_ARM64))
/* Switching coroutine stacks must not invoke the CRT unwinder.  Each MSVC
 * target has its own assembler context restore with a guarded jmp_buf layout
 * (src/arch/x86/hev-task-msvc-x64-abi.c, ...-x86-abi.c and
 * src/arch/arm/hev-task-msvc-arm64-abi.c). */
__declspec(noreturn) void hev_task_msvc_longjmp (void *env, int value);
#define HEV_TASK_LONGJMP(env, value) \
    hev_task_msvc_longjmp ((void *)(env), (value))
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
