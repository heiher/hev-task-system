/*
 ============================================================================
 Name        : hev-task-timer.c
 Author      : Heiher <r@hev.cc>
 Copyright   : Copyright (c) 2018 everyone.
 Description : Timer
 ============================================================================
 */

#include <time.h>
#include <errno.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/timerfd.h>

#include "hev-task-timer.h"
#include "kern/task/hev-task-private.h"
#include "kern/core/hev-task-system-private.h"
#include "mm/api/hev-memory-allocator-api.h"
#include "lib/rbtree/hev-rbtree-cached.h"

typedef struct _HevTaskTimerNode HevTaskTimerNode;

struct _HevTaskTimer
{
    HevRBTreeCached sort_tree;
    HevTaskSchedEntity sched_entity;

    int fd;
};

struct _HevTaskTimerNode
{
    HevRBTreeNode base;

    struct timespec expire;
    HevTask *task;
};

HevTaskTimer *
hev_task_timer_new (void)
{
    HevTaskTimer *self;
    int epoll_fd;
    struct epoll_event event;

    self = hev_malloc0 (sizeof (HevTaskTimer));
    if (!self)
        return NULL;

    self->fd = timerfd_create (CLOCK_MONOTONIC, TFD_NONBLOCK | TFD_CLOEXEC);
    if (self->fd == -1) {
        hev_free (self);
        return NULL;
    }

    epoll_fd = hev_task_system_get_context ()->epoll_fd;
    event.events = EPOLLET | EPOLLIN;
    event.data.ptr = &self->sched_entity;
    if (epoll_ctl (epoll_fd, EPOLL_CTL_ADD, self->fd, &event) == -1) {
        close (self->fd);
        hev_free (self);
        return NULL;
    }

    return self;
}

void
hev_task_timer_destroy (HevTaskTimer *self)
{
    HevRBTreeNode *node;

    while ((node = hev_rbtree_cached_first (&self->sort_tree)))
        hev_rbtree_cached_erase (&self->sort_tree, node);

    close (self->fd);
    hev_free (self);
}

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

static inline int
hev_task_timer_set_time (HevTaskTimer *self, const struct timespec *expire)
{
    struct itimerspec sp;

    sp.it_value = *expire;
    sp.it_interval.tv_sec = 0;
    sp.it_interval.tv_nsec = 0;

    return timerfd_settime (self->fd, TFD_TIMER_ABSTIME, &sp, NULL);
}

static inline unsigned int
hev_task_timer_get_time (HevTaskTimer *self)
{
    struct itimerspec sp;

    if (timerfd_gettime (self->fd, &sp) == -1)
        abort ();

    if ((sp.it_value.tv_sec + sp.it_value.tv_nsec) == 0)
        return 0;

    return (sp.it_value.tv_sec * 1000000) + (sp.it_value.tv_nsec / 1000);
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
        uint64_t time;

        /* update timer: pick current */
        if (hev_task_timer_set_time (self, &node->expire) == -1)
            abort ();
        /* fast path: check is expired */
        if (read (self->fd, &time, sizeof (time)) == sizeof (time)) {
            hev_rbtree_cached_erase (&self->sort_tree, &node->base);
            return 0;
        }
        self->sched_entity.task = task;
    }

    /* slow path: wait io */
    hev_task_yield (HEV_TASK_WAITIO);

    /* remove expired from sort tree */
    hev_rbtree_cached_erase (&self->sort_tree, &node->base);

    /* update timer: pick next */
    next = hev_rbtree_cached_first (&self->sort_tree);
    if (next) {
        node = container_of (next, HevTaskTimerNode, base);
        if (hev_task_timer_set_time (self, &node->expire) == -1)
            abort ();
        self->sched_entity.task = node->task;
    }

    /* get left microseconds */
    return hev_task_timer_get_time (self);
}
