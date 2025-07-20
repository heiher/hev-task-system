/*
 ============================================================================
 Name        : hev-task-cio.h
 Author      : hev <r@hev.cc>
 Copyright   : Copyright (c) 2024 hev.
 Description : Task Chain I/O
 ============================================================================
 */

#ifndef __HEV_TASK_CIO_H__
#define __HEV_TASK_CIO_H__

#include <sys/uio.h>

#include <hev-object.h>

#ifdef __cplusplus
extern "C" {
#endif

#define HEV_TASK_CIO(p) ((HevTaskCIO *)p)
#define HEV_TASK_CIO_CLASS(p) ((HevTaskCIOClass *)p)
#define HEV_TASK_CIO_TYPE (hev_task_cio_class ())

typedef struct _HevTaskCIO HevTaskCIO;
typedef struct _HevTaskCIOClass HevTaskCIOClass;
typedef enum _HevTaskCIOCtrl HevTaskCIOCtrl;

struct _HevTaskCIO
{
    HevObject base;

    int err;
    HevTaskCIO *next;
};

struct _HevTaskCIOClass
{
    HevObjectClass base;

    ssize_t (*read) (HevTaskCIO *, void *, size_t);
    ssize_t (*write) (HevTaskCIO *, const void *, size_t);
    ssize_t (*readv) (HevTaskCIO *, const struct iovec *, int);
    ssize_t (*writev) (HevTaskCIO *, const struct iovec *, int);
    ssize_t (*read_dgram) (HevTaskCIO *, void *, size_t, void *);
    ssize_t (*write_dgram) (HevTaskCIO *, const void *, size_t, const void *);
    ssize_t (*readv_dgram) (HevTaskCIO *, const struct iovec *, int, void *);
    ssize_t (*writev_dgram) (HevTaskCIO *, const struct iovec *, int,
                             const void *);
    long (*ctrl) (HevTaskCIO *, int, long, void *);
};

enum _HevTaskCIOCtrl
{
    HEV_TASK_CIO_CTRL_FLUSH,
    HEV_TASK_CIO_CTRL_GET_FD,
    HEV_TASK_CIO_CTRL_SHUTDOWN,
};

/**
 * hev_task_cio_class:
 *
 * Get the class of #HevTaskCIO.
 *
 * Returns: a new #HeObjectClass.
 *
 * Since: 5.3
 */
HevObjectClass *hev_task_cio_class (void);

/**
 * hev_task_cio_construct:
 * @self: a #HevTaskCIO
 *
 * Construct a new #HevTaskCIO.
 *
 * Returns: When successful, returns zero. otherwise, returns -1.
 *
 * Since: 5.3
 */
int hev_task_cio_construct (HevTaskCIO *self);

/**
 * hev_task_cio_push:
 * @self: a #HevTaskCIO
 * @next: (transfer full): a #HevTaskCIO
 *
 * Push the @next into chain.
 *
 * Returns: a #HevTaskCIO
 *
 * Since: 5.3
 */
HevTaskCIO *hev_task_cio_push (HevTaskCIO *self, HevTaskCIO *next);

/**
 * hev_task_cio_pop:
 * @self: a #HevTaskCIO
 * @next: a #HevTaskCIO
 *
 * Pop and unref the first CIO in chain, return the second CIO.
 *
 * Returns: a #HevTaskCIO
 *
 * Since: 5.3
 */
HevTaskCIO *hev_task_cio_pop (HevTaskCIO *self);

/**
 * hev_task_cio_get_fd:
 * @self: a #HevTaskCIO
 * @dir: direction of fd: 0 for read, 1 for write
 *
 * Get file descriptor of this #HevTaskCIO.
 *
 * Returns: the new file descriptor, or -1 if not exist.
 *
 * Since: 5.3
 */
int hev_task_cio_get_fd (HevTaskCIO *self, int dir);

/**
 * hev_task_cio_read:
 * @self: a #HevTaskCIO
 * @buf: buffer
 * @count: buffer length
 * @yielder: a #HevTaskIOYielder
 * @yielder_data: user data
 *
 * The read function shall attempt to read @count bytes from @self,
 * into the buffer pointed to by @buf.
 *
 * Returns: the number of bytes actually read
 *
 * Since: 5.3
 */
ssize_t hev_task_cio_read (HevTaskCIO *self, void *buf, size_t count,
                           HevTaskIOYielder yielder, void *yielder_data);

/**
 * hev_task_cio_read_exact:
 * @self: a #HevTaskCIO
 * @buf: buffer
 * @count: buffer length
 * @yielder: a #HevTaskIOYielder
 * @yielder_data: user data
 *
 * The read_exact function shall attempt to read exactly @count bytes
 * from @self, into the buffer pointed to by @buf.
 *
 * Returns: the number of bytes actually read
 *
 * Since: 5.3
 */
ssize_t hev_task_cio_read_exact (HevTaskCIO *self, void *buf, size_t count,
                                 HevTaskIOYielder yielder, void *yielder_data);

/**
 * hev_task_cio_readv:
 * @self: a #HevTaskCIO
 * @iov: io vector
 * @iovcnt: io vector count
 * @yielder: a #HevTaskIOYielder
 * @yielder_data: user data
 *
 * The readv function shall attempt to read @count bytes from @self,
 * into the buffers pointed to by @iov.
 *
 * Returns: the number of bytes actually read
 *
 * Since: 5.3
 */
ssize_t hev_task_cio_readv (HevTaskCIO *self, const struct iovec *iov,
                            int iovcnt, HevTaskIOYielder yielder,
                            void *yielder_data);

/**
 * hev_task_cio_readv_exact:
 * @self: a #HevTaskCIO
 * @iov: io vector
 * @iovcnt: io vector count
 * @yielder: a #HevTaskIOYielder
 * @yielder_data: user data
 *
 * The read_exact function shall attempt to read exactly bytes from @self,
 * into the buffer pointed to by @buf.
 *
 * Returns: the number of bytes actually read
 *
 * Since: 5.3
 */
ssize_t hev_task_cio_readv_exact (HevTaskCIO *self, const struct iovec *iov,
                                  int iovcnt, HevTaskIOYielder yielder,
                                  void *yielder_data);

/**
 * hev_task_cio_write:
 * @self: a #HevTaskCIO
 * @buf: buffer
 * @count: buffer length
 * @yielder: a #HevTaskIOYielder
 * @yielder_data: user data
 *
 * The write function shall attempt to write @count bytes from the buffer
 * pointed to by @buf to @self.
 *
 * Returns: the number of bytes actually write
 *
 * Since: 5.3
 */
ssize_t hev_task_cio_write (HevTaskCIO *self, const void *buf, size_t count,
                            HevTaskIOYielder yielder, void *yielder_data);

/**
 * hev_task_cio_write_exact:
 * @self: a #HevTaskCIO
 * @buf: buffer
 * @count: buffer length
 * @yielder: a #HevTaskIOYielder
 * @yielder_data: user data
 *
 * The write_exact function shall attempt to write exactly @count bytes
 * from the buffer pointed to by @buf to @self.
 *
 * Returns: the number of bytes actually write
 *
 * Since: 5.3
 */
ssize_t hev_task_cio_write_exact (HevTaskCIO *self, const void *buf,
                                  size_t count, HevTaskIOYielder yielder,
                                  void *yielder_data);

/**
 * hev_task_cio_writev:
 * @self: a #HevTaskCIO
 * @iov: io vector
 * @iovcnt: io vector count
 * @yielder: a #HevTaskIOYielder
 * @yielder_data: user data
 *
 * The writev function shall attempt to write bytes from the buffer
 * pointed to by @iov to @self.
 *
 * Returns: the number of bytes actually write
 *
 * Since: 5.3
 */
ssize_t hev_task_cio_writev (HevTaskCIO *self, const struct iovec *iov,
                             int iovcnt, HevTaskIOYielder yielder,
                             void *yielder_data);

/**
 * hev_task_cio_writev_exact:
 * @self: a #HevTaskCIO
 * @iov: io vector
 * @iovcnt: io vector count
 * @yielder: a #HevTaskIOYielder
 * @yielder_data: user data
 *
 * The writev_exact function shall attempt to write exactly bytes
 * from the buffer pointed to by @iov to @self.
 *
 * Returns: the number of bytes actually write
 *
 * Since: 5.3
 */
ssize_t hev_task_cio_writev_exact (HevTaskCIO *self, const struct iovec *iov,
                                   int iovcnt, HevTaskIOYielder yielder,
                                   void *yielder_data);

/**
 * hev_task_cio_read_dgram:
 * @self: a #HevTaskCIO
 * @buf: buffer
 * @count: buffer length
 * @addr: (out): address
 * @yielder: a #HevTaskIOYielder
 * @yielder_data: user data
 *
 * The read_dgram function shall attempt to read @count bytes from @self,
 * into the buffer pointed to by @buf.
 *
 * Returns: the number of bytes actually read
 *
 * Since: 5.3
 */
ssize_t hev_task_cio_read_dgram (HevTaskCIO *self, void *buf, size_t count,
                                 void *addr, HevTaskIOYielder yielder,
                                 void *yielder_data);

/**
 * hev_task_cio_readv_dgram:
 * @self: a #HevTaskCIO
 * @iov: io vector
 * @iovcnt: io vector count
 * @addr: (out): address
 * @yielder: a #HevTaskIOYielder
 * @yielder_data: user data
 *
 * The readv_dgram function shall attempt to read bytes from @self,
 * into the buffer pointed to by @iov.
 *
 * Returns: the number of bytes actually read
 *
 * Since: 5.3
 */
ssize_t hev_task_cio_readv_dgram (HevTaskCIO *self, const struct iovec *iov,
                                  int iovcnt, void *addr,
                                  HevTaskIOYielder yielder, void *yielder_data);

/**
 * hev_task_cio_write_dgram:
 * @self: a #HevTaskCIO
 * @buf: buffer
 * @count: buffer length
 * @addr: address
 * @yielder: a #HevTaskIOYielder
 * @yielder_data: user data
 *
 * The write_dgram function shall attempt to write @count bytes from
 * the buffer pointed to by @buf to @self.
 *
 * Returns: the number of bytes actually write
 *
 * Since: 5.3
 */
ssize_t hev_task_cio_write_dgram (HevTaskCIO *self, const void *buf,
                                  size_t count, const void *addr,
                                  HevTaskIOYielder yielder, void *yielder_data);

/**
 * hev_task_cio_writev_dgram:
 * @self: a #HevTaskCIO
 * @iov: io vector
 * @iovcnt: io vector count
 * @addr: address
 * @yielder: a #HevTaskIOYielder
 * @yielder_data: user data
 *
 * The writev_dgram function shall attempt to write bytes from the buffer
 * pointed to by @iov to @self.
 *
 * Returns: the number of bytes actually write
 *
 * Since: 5.3
 */
ssize_t hev_task_cio_writev_dgram (HevTaskCIO *self, const struct iovec *iov,
                                   int iovcnt, const void *addr,
                                   HevTaskIOYielder yielder,
                                   void *yielder_data);

/**
 * hev_task_cio_flush:
 * @self: a #HevTaskCIO
 * @yielder: a #HevTaskIOYielder
 * @yielder_data: user data
 *
 * The flush function normally writes out any internally buffered data.
 *
 * Returns: When successful, returns zero. otherwise, returns -1.
 *
 * Since: 5.3
 */
int hev_task_cio_flush (HevTaskCIO *self, HevTaskIOYielder yielder,
                        void *yielder_data);

/**
 * hev_task_cio_splice:
 * @a: a #HevTaskCIO
 * @b: a #HevTaskCIO
 * @buf_size: buffer length
 * @yielder: a #HevTaskIOYielder
 * @yielder_data: user data
 *
 * The splice moves data between two CIOs until one error or closed.
 *
 * Since: 5.3
 */
void hev_task_cio_splice (HevTaskCIO *a, HevTaskCIO *b, size_t buf_size,
                          HevTaskIOYielder yielder, void *yielder_data);

/**
 * hev_task_cio_next_read:
 * @self: a #HevTaskCIO
 * @buf: buffer
 * @count: buffer length
 *
 * Call next #HevTaskCIO read.
 *
 * Returns: the number of bytes actually read
 *
 * Since: 5.3
 */
static inline ssize_t
hev_task_cio_next_read (HevTaskCIO *self, void *buf, size_t count)
{
    HevTaskCIO *next = self->next;
    HevTaskCIOClass *nkptr;
    ssize_t res;

    if (!next) {
        self->err = 0;
        return -1;
    }

    nkptr = HEV_OBJECT_GET_CLASS (next);
    res = nkptr->read (next, buf, count);
    self->err = next->err;

    return res;
}

/**
 * hev_task_cio_next_readv:
 * @self: a #HevTaskCIO
 * @iov: io vector
 * @iovcnt: io vector count
 *
 * Call next #HevTaskCIO readv.
 *
 * Returns: the number of bytes actually read
 *
 * Since: 5.3
 */
static inline ssize_t
hev_task_cio_next_readv (HevTaskCIO *self, const struct iovec *iov, int iovcnt)
{
    HevTaskCIO *next = self->next;
    HevTaskCIOClass *nkptr;
    ssize_t res;

    if (!next) {
        self->err = 0;
        return -1;
    }

    nkptr = HEV_OBJECT_GET_CLASS (next);
    res = nkptr->readv (next, iov, iovcnt);
    self->err = next->err;

    return res;
}

/**
 * hev_task_cio_next_write:
 * @self: a #HevTaskCIO
 * @buf: buffer
 * @count: buffer length
 *
 * Call next #HevTaskCIO write.
 *
 * Returns: the number of bytes actually write
 *
 * Since: 5.3
 */
static inline ssize_t
hev_task_cio_next_write (HevTaskCIO *self, const void *buf, size_t count)
{
    HevTaskCIO *next = self->next;
    HevTaskCIOClass *nkptr;
    ssize_t res;

    if (!next) {
        self->err = 0;
        return -1;
    }

    nkptr = HEV_OBJECT_GET_CLASS (next);
    res = nkptr->write (next, buf, count);
    self->err = next->err;

    return res;
}

/**
 * hev_task_cio_next_writev:
 * @self: a #HevTaskCIO
 * @iov: io vector
 * @iovcnt: io vector count
 *
 * Call next #HevTaskCIO writev.
 *
 * Returns: the number of bytes actually write
 *
 * Since: 5.3
 */
static inline ssize_t
hev_task_cio_next_writev (HevTaskCIO *self, const struct iovec *iov, int iovcnt)
{
    HevTaskCIO *next = self->next;
    HevTaskCIOClass *nkptr;
    ssize_t res;

    if (!next) {
        self->err = 0;
        return -1;
    }

    nkptr = HEV_OBJECT_GET_CLASS (next);
    res = nkptr->writev (next, iov, iovcnt);
    self->err = next->err;

    return res;
}

/**
 * hev_task_cio_next_read_dgram:
 * @self: a #HevTaskCIO
 * @buf: buffer
 * @count: buffer length
 * @addr: (out): address
 *
 * Call next #HevTaskCIO read.
 *
 * Returns: the number of bytes actually read
 *
 * Since: 5.3
 */
static inline ssize_t
hev_task_cio_next_read_dgram (HevTaskCIO *self, void *buf, size_t count,
                              void *addr)
{
    HevTaskCIO *next = self->next;
    HevTaskCIOClass *nkptr;
    ssize_t res;

    if (!next) {
        self->err = 0;
        return -1;
    }

    nkptr = HEV_OBJECT_GET_CLASS (next);
    res = nkptr->read_dgram (next, buf, count, addr);
    self->err = next->err;

    return res;
}

/**
 * hev_task_cio_next_readv_dgram:
 * @self: a #HevTaskCIO
 * @iov: io vector
 * @iovcnt: io vector count
 * @addr: (out): address
 *
 * Call next #HevTaskCIO readv.
 *
 * Returns: the number of bytes actually read
 *
 * Since: 5.3
 */
static inline ssize_t
hev_task_cio_next_readv_dgram (HevTaskCIO *self, const struct iovec *iov,
                               int iovcnt, void *addr)
{
    HevTaskCIO *next = self->next;
    HevTaskCIOClass *nkptr;
    ssize_t res;

    if (!next) {
        self->err = 0;
        return -1;
    }

    nkptr = HEV_OBJECT_GET_CLASS (next);
    res = nkptr->readv_dgram (next, iov, iovcnt, addr);
    self->err = next->err;

    return res;
}

/**
 * hev_task_cio_next_write_dgram:
 * @self: a #HevTaskCIO
 * @buf: buffer
 * @count: buffer length
 * @addr: address
 *
 * Call next #HevTaskCIO write.
 *
 * Returns: the number of bytes actually write
 *
 * Since: 5.3
 */
static inline ssize_t
hev_task_cio_next_write_dgram (HevTaskCIO *self, const void *buf, size_t count,
                               const void *addr)
{
    HevTaskCIO *next = self->next;
    HevTaskCIOClass *nkptr;
    ssize_t res;

    if (!next) {
        self->err = 0;
        return -1;
    }

    nkptr = HEV_OBJECT_GET_CLASS (next);
    res = nkptr->write_dgram (next, buf, count, addr);
    self->err = next->err;

    return res;
}

/**
 * hev_task_cio_next_writev_dgram:
 * @self: a #HevTaskCIO
 * @iov: io vector
 * @iovcnt: io vector count
 * @addr: address
 *
 * Call next #HevTaskCIO writev.
 *
 * Returns: the number of bytes actually write
 *
 * Since: 5.3
 */
static inline ssize_t
hev_task_cio_next_writev_dgram (HevTaskCIO *self, const struct iovec *iov,
                                int iovcnt, const void *addr)
{
    HevTaskCIO *next = self->next;
    HevTaskCIOClass *nkptr;
    ssize_t res;

    if (!next) {
        self->err = 0;
        return -1;
    }

    nkptr = HEV_OBJECT_GET_CLASS (next);
    res = nkptr->writev_dgram (next, iov, iovcnt, addr);
    self->err = next->err;

    return res;
}

/**
 * hev_task_cio_next_ctrl:
 * @self: a #HevTaskCIO
 * @ctrl: a #HevTaskCIOCtrl
 * @larg: argument
 * @parg: argument
 *
 * Call next #HevTaskCIO ctrl.
 *
 * Returns: When successful, returns zero. otherwise, returns -1.
 *
 * Since: 5.3
 */
static inline long
hev_task_cio_next_ctrl (HevTaskCIO *self, int ctrl, long larg, void *parg)
{
    HevTaskCIO *next = self->next;
    HevTaskCIOClass *nkptr;
    long res;

    if (!next) {
        self->err = 0;
        return -1;
    }

    nkptr = HEV_OBJECT_GET_CLASS (next);
    res = nkptr->ctrl (next, ctrl, larg, parg);
    self->err = next->err;

    return res;
}

/**
 * hev_task_cio_next_destruct:
 * @self: a #HevTaskCIO
 *
 * Call next #HevTaskCIO destruct.
 *
 * Since: 5.3
 */
static inline void
hev_task_cio_next_destruct (HevTaskCIO *self)
{
    HevTaskCIO *next = self->next;
    HevObjectClass *nkptr;

    if (!next)
        return;

    nkptr = HEV_OBJECT_GET_CLASS (next);
    nkptr->destruct (HEV_OBJECT (next));
}

#ifdef __cplusplus
}
#endif

#endif /* __HEV_TASK_CIO_H__ */
