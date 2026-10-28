/*
 ============================================================================
 Name        : hev-sys-task-execute.h
 Author      : Heiher <r@hev.cc>
 Copyright   : Copyright (c) 2026 everyone.
 Description : Task entry execution
 ============================================================================
 */

#ifndef __HEV_SYS_UNIX_TASK_EXECUTE_H__
#define __HEV_SYS_UNIX_TASK_EXECUTE_H__

#define hev_sys_task_execute(ctx, top, limit, entry) \
    hev_task_execute ((ctx), (entry))

#endif /* __HEV_SYS_UNIX_TASK_EXECUTE_H__ */
