/*
 ============================================================================
 Name        : hev-task.h
 Author      : Heiher <r@hev.cc>
 Copyright   : Copyright (c) 2017 everyone.
 Description :
 ============================================================================
 */

#ifndef __HEV_TASK_H__
#define __HEV_TASK_H__

#include <sys/epoll.h>

#define HEV_TASK_PRIORITY_MIN	(0)
#define HEV_TASK_PRIORITY_MAX	(15)

typedef struct _HevTask HevTask;
typedef void (*HevTaskEntry) (void *data);

HevTask * hev_task_new (int stack_size);
HevTask * hev_task_ref (HevTask *self);
void hev_task_unref (HevTask *self);

HevTask * hev_task_self (void);

void hev_task_set_priority (HevTask *self, int priority);
int hev_task_get_priority (HevTask *self);

int hev_task_add_fd (HevTask *self, int fd, unsigned int events);
int hev_task_mod_fd (HevTask *self, int fd, unsigned int events);
int hev_task_del_fd (HevTask *self, int fd);

void hev_task_run (HevTask *self, HevTaskEntry entry, void *data);

#endif /* __HEV_TASK_H__ */

