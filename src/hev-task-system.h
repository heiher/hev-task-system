/*
 ============================================================================
 Name        : hev-task-system.h
 Author      : Heiher <r@hev.cc>
 Copyright   : Copyright (c) 2017 everyone.
 Description :
 ============================================================================
 */

#ifndef __HEV_TASK_SYSTEM_H__
#define __HEV_TASK_SYSTEM_H__

typedef enum _HevTaskSystemYieldType HevTaskSystemYieldType;

enum _HevTaskSystemYieldType
{
	HEV_TASK_SYSTEM_YIELD,
	HEV_TASK_SYSTEM_WAITIO,
	HEV_TASK_SYSTEM_YIELD_COUNT,
};

int hev_task_system_init (void);
void hev_task_system_fini (void);
void hev_task_system_run (void);
void hev_task_system_yield (HevTaskSystemYieldType type);

#endif /* __HEV_TASK_SYSTEM_H__ */

