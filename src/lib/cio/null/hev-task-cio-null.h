/*
 ============================================================================
 Name        : hev-task-cio-null.h
 Author      : hev <r@hev.cc>
 Copyright   : Copyright (c) 2024 hev.
 Description : Task Chain I/O Null
 ============================================================================
 */

#ifndef __HEV_TASK_CIO_NULL_H__
#define __HEV_TASK_CIO_NULL_H__

#include <hev-task-cio.h>

#ifdef __cplusplus
extern "C" {
#endif

#define HEV_TASK_CIO_NULL(p) ((HevTaskCIONull *)p)
#define HEV_TASK_CIO_NULL_CLASS(p) ((HevTaskCIONullClass *)p)
#define HEV_TASK_CIO_NULL_TYPE (hev_task_cio_null_class ())

typedef struct _HevTaskCIONull HevTaskCIONull;
typedef struct _HevTaskCIONullClass HevTaskCIONullClass;

struct _HevTaskCIONull
{
    HevTaskCIO base;
};

struct _HevTaskCIONullClass
{
    HevTaskCIOClass base;
};

/**
 * hev_task_cio_null_class:
 *
 * Get the class of #HevTaskCIONull.
 *
 * Returns: a new #HeObjectClass.
 *
 * Since: 5.3
 */
HevObjectClass *hev_task_cio_null_class (void);

/**
 * hev_task_cio_null_construct:
 * @self: a #HevTaskCIO
 *
 * Construct a new #HevTaskCIONull.
 *
 * Returns: When successful, returns zero. otherwise, returns -1.
 *
 * Since: 5.3
 */
int hev_task_cio_null_construct (HevTaskCIONull *self);

/**
 * hev_task_cio_null_new:
 *
 * Create a new #HevTaskCIONull.
 *
 * Returns: a #HevTaskCIONull
 *
 * Since: 5.3
 */
HevTaskCIONull *hev_task_cio_null_new (void);

#ifdef __cplusplus
}
#endif

#endif /* __HEV_TASK_CIO_NULL_H__ */
