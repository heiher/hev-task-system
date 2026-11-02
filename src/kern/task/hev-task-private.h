/*
 ============================================================================
 Name        : hev-task-private.h
 Author      : hev <r@hev.cc>
 Copyright   : Copyright (c) 2017 - 2026 hev
 Description :
 ============================================================================
 */

#ifndef __HEV_TASK_PRIVATE_H__
#define __HEV_TASK_PRIVATE_H__

#include <stdint.h>
#include <setjmp.h>

#include <hev-task.h>

#include "hev-task-stack.h"
#include "lib/list/hev-list.h"
#include "lib/rbtree/hev-rbtree.h"

#define HEV_TASK_SCHED_SLOT_TAG ((uintptr_t)1)

struct _HevTask
{
    void *stack_bottom;
    HevTaskEntry entry;
    void *data;

    uint64_t sched_key;
    HevRBTreeNode sched_node;

    HevTaskStack *stack;
    HevTask *joiner;

    int ref_count;
    uint8_t priority;
    uint8_t next_priority;
    uint8_t state;

    jmp_buf context;

    HevListNode list_node;
};

static inline void *
hev_task_to_data (HevTask *task)
{
    return task;
}

static inline void *
hev_task_slot_to_data (HevTask **slot)
{
    return (void *)((uintptr_t)slot | HEV_TASK_SCHED_SLOT_TAG);
}

static inline HevTask *
hev_task_from_data (void *data)
{
    uintptr_t v = (uintptr_t)data;

    if (v & HEV_TASK_SCHED_SLOT_TAG)
        return *(HevTask **)(v & ~HEV_TASK_SCHED_SLOT_TAG);

    return (HevTask *)v;
}

void hev_task_destroy (HevTask *self);

extern void hev_task_execute (HevTask *self, void *executer);

#endif /* __HEV_TASK_PRIVATE_H__ */
