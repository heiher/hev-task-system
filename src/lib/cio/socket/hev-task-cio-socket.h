/*
 ============================================================================
 Name        : hev-task-cio-socket.h
 Author      : hev <r@hev.cc>
 Copyright   : Copyright (c) 2024 hev.
 Description : Task Chain I/O Socket
 ============================================================================
 */

#ifndef __HEV_TASK_CIO_SOCKET_H__
#define __HEV_TASK_CIO_SOCKET_H__

#include <hev-task-cio.h>

#ifdef __cplusplus
extern "C" {
#endif

#define HEV_TASK_CIO_SOCKET(p) ((HevTaskCIOSocket *)p)
#define HEV_TASK_CIO_SOCKET_CLASS(p) ((HevTaskCIOSocketClass *)p)
#define HEV_TASK_CIO_SOCKET_TYPE (hev_task_cio_socket_class ())

typedef struct _HevTaskCIOSocket HevTaskCIOSocket;
typedef struct _HevTaskCIOSocketClass HevTaskCIOSocketClass;

struct _HevTaskCIOSocket
{
    HevTaskCIO base;

    int alen;
    int fd;
};

struct _HevTaskCIOSocketClass
{
    HevTaskCIOClass base;
};

/**
 * hev_task_cio_socket_class:
 *
 * Get the class of #HevTaskCIOSocket.
 *
 * Returns: a new #HeObjectClass.
 *
 * Since: 5.3
 */
HevObjectClass *hev_task_cio_socket_class (void);

/**
 * hev_task_cio_socket_construct:
 * @self: a #HevTaskCIO
 * @family: socket family
 * @fd: socket file descriptor
 *
 * Construct a new #HevTaskCIOSocket.
 *
 * Returns: When successful, returns zero. otherwise, returns -1.
 *
 * Since: 5.3
 */
int hev_task_cio_socket_construct (HevTaskCIOSocket *self, int family, int fd);

/**
 * hev_task_cio_socket_new:
 * @family: socket family
 * @fd: socket file descriptor
 *
 * Create a new #HevTaskCIOSocket.
 *
 * Returns: a #HevTaskCIOSocket
 *
 * Since: 5.3
 */
HevTaskCIOSocket *hev_task_cio_socket_new (int family, int fd);

#ifdef __cplusplus
}
#endif

#endif /* __HEV_TASK_CIO_SOCKET_H__ */
