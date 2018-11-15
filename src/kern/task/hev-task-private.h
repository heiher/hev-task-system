/*
 ============================================================================
 Name        : hev-task-private.h
 Author      : Heiher <r@hev.cc>
 Copyright   : Copyright (c) 2017 everyone.
 Description :
 ============================================================================
 */

#ifndef __HEV_TASK_PRIVATE_H__
#define __HEV_TASK_PRIVATE_H__

#include <stdint.h>
#include <setjmp.h>

#include "hev-task.h"
#include "lib/rbtree/hev-rbtree.h"

typedef struct _HevTaskSchedEntity HevTaskSchedEntity;

struct _HevTaskSchedEntity
{
    HevTask *task;
};

struct _HevTask
{
    void *stack_top;
    HevTaskEntry entry;
    void *data;

    uint64_t sched_key;
    HevRBTreeNode sched_node;
    HevTaskSchedEntity sched_entity;

    void *stack;

    int ref_count;
    int priority;
    int next_priority;
    int stack_size;
    HevTaskState state;

    jmp_buf context;
};

extern void hev_task_execute (HevTask *self, void *executer);

#endif /* __HEV_TASK_PRIVATE_H__ */
