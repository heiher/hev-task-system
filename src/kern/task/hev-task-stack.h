/*
 ============================================================================
 Name        : hev-task-stack.h
 Author      : Heiher <r@hev.cc>
 Copyright   : Copyright (c) 2020 everyone.
 Description :
 ============================================================================
 */

#ifndef __HEV_TASK_STACK_H__
#define __HEV_TASK_STACK_H__

#include <stdint.h>

#define STACK_HEAP (0)
#define STACK_MMAP (1)

typedef struct _HevTaskStack HevTaskStack;

HevTaskStack *hev_task_stack_new (int size);
void hev_task_stack_destroy (HevTaskStack *self);

void *hev_task_stack_get_base (HevTaskStack *self);
void *hev_task_stack_get_bottom (HevTaskStack *self);

void hev_task_stack_release (HevTaskStack *self, void *sp);

/* Give the pages in [start, sp - margin) back to the OS, whole pages only. */
void hev_task_stack_discard (uintptr_t start, void *sp);

#endif /* __HEV_TASK_STACK_H__ */
