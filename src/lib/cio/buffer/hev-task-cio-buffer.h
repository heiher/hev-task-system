/*
 ============================================================================
 Name        : hev-task-cio-buffer.h
 Author      : hev <r@hev.cc>
 Copyright   : Copyright (c) 2024 hev.
 Description : Task Chain I/O Buffer
 ============================================================================
 */

#ifndef __HEV_TASK_CIO_BUFFER_H__
#define __HEV_TASK_CIO_BUFFER_H__

#include <hev-task-cio.h>
#include <hev-circular-buffer.h>

#ifdef __cplusplus
extern "C" {
#endif

#define HEV_TASK_CIO_BUFFER(p) ((HevTaskCIOBuffer *)p)
#define HEV_TASK_CIO_BUFFER_CLASS(p) ((HevTaskCIOBufferClass *)p)
#define HEV_TASK_CIO_BUFFER_TYPE (hev_task_cio_buffer_class ())

typedef struct _HevTaskCIOBuffer HevTaskCIOBuffer;
typedef struct _HevTaskCIOBufferClass HevTaskCIOBufferClass;

struct _HevTaskCIOBuffer
{
    HevTaskCIO base;

    HevCircularBuffer *rbuf;
    HevCircularBuffer *wbuf;
};

struct _HevTaskCIOBufferClass
{
    HevTaskCIOClass base;
};

/**
 * hev_task_cio_buffer_class:
 *
 * Get the class of #HevTaskCIOBuffer.
 *
 * Returns: a new #HeObjectClass.
 *
 * Since: 5.3
 */
HevObjectClass *hev_task_cio_buffer_class (void);

/**
 * hev_task_cio_buffer_construct:
 * @self: a #HevTaskCIO
 * @rsize: read buffer size
 * @wsize: write buffer size
 *
 * Construct a new #HevTaskCIOBuffer.
 *
 * Returns: When successful, returns zero. otherwise, returns -1.
 *
 * Since: 5.3
 */
int hev_task_cio_buffer_construct (HevTaskCIOBuffer *self, size_t rsize,
                                   size_t wsize);

/**
 * hev_task_cio_buffer_new:
 * @rsize: read buffer size
 * @wsize: write buffer size
 *
 * Create a new #HevTaskCIOBuffer.
 *
 * Returns: a #HevTaskCIOBuffer
 *
 * Since: 5.3
 */
HevTaskCIOBuffer *hev_task_cio_buffer_new (size_t rsize, size_t wsize);

/**
 * hev_task_cio_buffer_peek:
 * @self: a #HevTaskCIOBuffer
 * @buf: buffer
 * @count: buffer length
 *
 * The peek function shall attempt to peek at @count bytes from @self,
 * into the buffer pointed to by @buf.
 *
 * Returns: the number of bytes actually peek
 *
 * Since: 5.3
 */
ssize_t hev_task_cio_buffer_peek (HevTaskCIOBuffer *self, void *buf,
                                  size_t count);

/**
 * hev_task_cio_buffer_peekv:
 * @self: a #HevTaskCIOBuffer
 * @iov: a io vector contain two elements
 *
 * The peekv function shall attempt to peek at all bytes from @self,
 * writing the references into @iov.
 *
 * Returns: the number of iov actually used
 *
 * Since: 5.3
 */
int hev_task_cio_buffer_peekv (HevTaskCIOBuffer *self, struct iovec *iov);

#ifdef __cplusplus
}
#endif

#endif /* __HEV_TASK_CIO_BUFFER_H__ */
