/*
 ============================================================================
 Name        : hev-task-io.h
 Author      : Heiher <r@hev.cc>
 Copyright   : Copyright (c) 2018 everyone.
 Description : Task I/O operations
 ============================================================================
 */

#ifndef __HEV_TASK_IO_H__
#define __HEV_TASK_IO_H__

typedef int (*HevTaskIOYielder) (HevTaskYieldType type, void *data);

/**
 * hev_task_io_read:
 * @fd: a file descriptor
 * @buf: buffer
 * @count: buffer length
 * @yielder: a #HevTaskIOYielder
 * @yielder_data: user data
 *
 * The read function shall attempt to read @count bytes from the file associated
 * with the open file descriptor, @fd, into the buffer pointed to by @buf.
 *
 * Returns: the number of bytes actually read
 *
 * Since: 3.2
 */
ssize_t hev_task_io_read (int fd, void *buf, size_t count,
                          HevTaskIOYielder yielder, void *yielder_data);

/**
 * hev_task_io_write:
 * @fd: a file descriptor
 * @buf: buffer
 * @count: buffer length
 * @yielder: a #HevTaskIOYielder
 * @yielder_data: user data
 *
 * The write function shall attempt to write @count bytes from the buffer
 * pointed to by @buf to the file associated with the open file descriptor, @fd.
 *
 * Returns: the number of bytes actually write
 *
 * Since: 3.2
 */
ssize_t hev_task_io_write (int fd, const void *buf, size_t count,
                           HevTaskIOYielder yielder, void *yielder_data);

/**
 * hev_task_io_splice:
 * @fd_a_i: a file descriptor for input
 * @fd_a_o: a file descriptor for output
 * @fd_b_i: another file descriptor for input
 * @fd_b_o: another file destriptor for output
 * @buf_size: buffer length
 * @yielder: a #HevTaskIOYielder
 * @yielder_data: user data
 *
 * The splice moves data between two file descriptors until one error or closed.
 *
 * Since: 3.2
 */
void hev_task_io_splice (int fd_a_i, int fd_a_o, int fd_b_i, int fd_b_o,
                         size_t buf_size, HevTaskIOYielder yielder,
                         void *yielder_data);

#endif /* __HEV_TASK_IO_H__ */
