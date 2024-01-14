/*
 ============================================================================
 Name        : hev-task-cio-fd.h
 Author      : hev <r@hev.cc>
 Copyright   : Copyright (c) 2024 hev.
 Description : Task Chain I/O Fd
 ============================================================================
 */

#ifndef __HEV_TASK_CIO_FD_H__
#define __HEV_TASK_CIO_FD_H__

#include <hev-task-cio.h>

#ifdef __cplusplus
extern "C" {
#endif

#define HEV_TASK_CIO_FD(p) ((HevTaskCIOFd *)p)
#define HEV_TASK_CIO_FD_CLASS(p) ((HevTaskCIOFdClass *)p)
#define HEV_TASK_CIO_FD_TYPE (hev_task_cio_fd_class ())

typedef struct _HevTaskCIOFd HevTaskCIOFd;
typedef struct _HevTaskCIOFdClass HevTaskCIOFdClass;

struct _HevTaskCIOFd
{
    HevTaskCIO base;

    int fdi;
    int fdo;
};

struct _HevTaskCIOFdClass
{
    HevTaskCIOClass base;
};

/**
 * hev_task_cio_fd_class:
 *
 * Get the class of #HevTaskCIOFd.
 *
 * Returns: a new #HeObjectClass.
 *
 * Since: 5.3
 */
HevObjectClass *hev_task_cio_fd_class (void);

/**
 * hev_task_cio_fd_construct:
 * @self: a #HevTaskCIO
 * @fdi: input file descriptor
 * @fdo: output file descriptor
 *
 * Construct a new #HevTaskCIOFd.
 *
 * Returns: When successful, returns zero. otherwise, returns -1.
 *
 * Since: 5.3
 */
int hev_task_cio_fd_construct (HevTaskCIOFd *self, int fdi, int fdo);

/**
 * hev_task_cio_fd_new:
 * @fdi: input file descriptor
 * @fdo: output file descriptor
 *
 * Create a new #HevTaskCIOFd.
 *
 * Returns: a #HevTaskCIOFd
 *
 * Since: 5.3
 */
HevTaskCIOFd *hev_task_cio_fd_new (int fdi, int fdo);

#ifdef __cplusplus
}
#endif

#endif /* __HEV_TASK_CIO_FD_H__ */
