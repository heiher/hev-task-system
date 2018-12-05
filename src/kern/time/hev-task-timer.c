/*
 ============================================================================
 Name        : hev-task-timer.c
 Author      : Heiher <r@hev.cc>
 Copyright   : Copyright (c) 2018 everyone.
 Description : Timer
 ============================================================================
 */

#include <time.h>

#include "hev-task-timer.h"

#if defined(__linux__)
#include "kern/time/hev-task-timer-timerfd.h"
#else
#include "kern/time/hev-task-timer-kevent.h"
#endif

typedef struct _HevTaskTimerNode HevTaskTimerNode;

struct _HevTaskTimerNode
{
    HevRBTreeNode base;

    struct timespec expire;
    HevTask *task;
};

static inline int
hev_task_timer_node_compare (HevTaskTimerNode *a, HevTaskTimerNode *b)
{
    if (a->expire.tv_sec < b->expire.tv_sec)
        return -1;
    if (a->expire.tv_sec > b->expire.tv_sec)
        return 1;

    if (a->expire.tv_nsec < b->expire.tv_nsec)
        return -1;
    if (a->expire.tv_nsec > b->expire.tv_nsec)
        return 1;

    if (a < b)
        return -1;
    if (a > b)
        return 1;

    return 0;
}

unsigned int
hev_task_timer_wait (HevTaskTimer *self, unsigned int microseconds,
                     HevTask *task)
{
    HevTaskTimerNode curr_node, *node = &curr_node;
    HevRBTreeNode **new = &self->sort_tree.base.root, *parent = NULL, *next;
    int leftmost = 1;

    /* get expire time */
    if (clock_gettime (CLOCK_MONOTONIC, &node->expire) == -1)
        abort ();
    node->expire.tv_sec += microseconds / 1000000;
    node->expire.tv_nsec += (microseconds % 1000000) * 1000;
    if (node->expire.tv_nsec > 1000000000L) {
        node->expire.tv_sec++;
        node->expire.tv_nsec -= 1000000000L;
    }
    node->task = task;

    /* insert to sort tree */
    while (*new) {
        HevTaskTimerNode *this = container_of (*new, HevTaskTimerNode, base);
        int result = hev_task_timer_node_compare (node, this);

        parent = *new;
        if (result < 0) {
            new = &((*new)->left);
        } else {
            new = &((*new)->right);
            leftmost = 0;
        }
    }
    hev_rbtree_node_link (&node->base, parent, new);
    hev_rbtree_cached_insert_color (&self->sort_tree, &node->base, leftmost);

    if (leftmost) {
        /* update timer: pick current */
        if (hev_task_timer_set_time (self, &node->expire) == -1)
            abort ();
        /* fast path: check is expired */
        if (hev_task_timer_fast_check (self)) {
            hev_rbtree_cached_erase (&self->sort_tree, &node->base);
            return 0;
        }
        self->sched_entity.task = task;
    }

    /* slow path: wait io */
    hev_task_yield (HEV_TASK_WAITIO);

    /* remove expired from sort tree */
    hev_rbtree_cached_erase (&self->sort_tree, &node->base);

    /* get left microseconds */
    microseconds = hev_task_timer_get_time (self, &curr_node.expire);

    /* update timer: pick next */
    next = hev_rbtree_cached_first (&self->sort_tree);
    if (next) {
        node = container_of (next, HevTaskTimerNode, base);
        if (hev_task_timer_set_time (self, &node->expire) == -1)
            abort ();
        self->sched_entity.task = node->task;
    }

    return microseconds;
}
