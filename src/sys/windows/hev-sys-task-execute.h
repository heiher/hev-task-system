/*
 ============================================================================
 Name        : hev-sys-task-execute.h
 Author      : Heiher <r@hev.cc>
 Copyright   : Copyright (c) 2026 everyone.
 Description : Task stack range in the TEB
 ============================================================================
 */

#ifndef __HEV_SYS_TASK_EXECUTE_H__
#define __HEV_SYS_TASK_EXECUTE_H__

#ifdef __cplusplus
extern "C" {
#endif

void hev_sys_task_execute (void *ctx, void *top, void *limit, void *entry);

#ifdef __cplusplus
}
#endif

#endif /* __HEV_SYS_TASK_EXECUTE_H__ */
