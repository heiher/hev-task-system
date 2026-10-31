/*
 ============================================================================
 Name        : hev-task-io-socket.h
 Author      : hev <r@hev.cc>
 Copyright   : Copyright (c) 2018 - 2026 hev
 Description : Task socket I/O operations
 ============================================================================
 */

#ifndef __HEV_TASK_IO_SOCKET_H__
#define __HEV_TASK_IO_SOCKET_H__

#include <stdint.h>
#include <hev-sys.h>

#ifdef __cplusplus
extern "C" {
#endif

#ifndef MSG_WAITFORONE
struct mmsghdr
{
    struct msghdr msg_hdr;
    unsigned int msg_len;
};
#endif

/**
 * hev_task_io_socket_socket:
 * @domain: the communications domain
 * @type: the type of socket
 * @protocol: a particular protocol to be used with the socket
 *
 * The socket function shall create an unbound socket in a communications
 * domain, and return a file descriptor that can be used in later function calls
 * that operate on sockets.
 *
 * Returns: the socket file descriptor.
 *
 * Since: 3.3.5
 */
int hev_task_io_socket_socket (int domain, int type, int protocol);

/**
 * hev_task_io_socket_socketpair:
 * @domain: the communications domain
 * @type: the type of socket
 * @protocol: a particular protocol to be used with the socket
 * @socket_vector: the file descriptors of the created socket pair
 *
 * The socketpair function shall create an unbound pair of connected sockets in
 * a specified @domain, of a specified @type, under the protocol optionally
 * specified by the @protocol augument.
 *
 * Returns: the status of create.
 *
 * Since: 3.3.5
 */
int hev_task_io_socket_socketpair (int domain, int type, int protocol,
                                   int socket_vector[2]);

/**
 * hev_task_io_socket_bind:
 * @fd: a file descriptor
 * @addr: socket address
 * @addr_len: socket address length
 *
 * The bind function shall assign a local socket address @addr to a socket
 * identified by file descriptor @fd that has no local socket address assigned.
 *
 * Returns: the status of bind
 *
 * Since: 5.12
 */
int hev_task_io_socket_bind (int fd, const struct sockaddr *addr,
                             socklen_t addr_len);

/**
 * hev_task_io_socket_listen:
 * @fd: a file descriptor
 * @backlog: the length of the queue of pending connections
 *
 * The listen function shall mark a connection-mode socket, specified by the
 * file descriptor @fd, as accepting connections.
 *
 * Returns: the status of listen
 *
 * Since: 5.12
 */
int hev_task_io_socket_listen (int fd, int backlog);

/**
 * hev_task_io_socket_setsockopt:
 * @fd: a file descriptor
 * @level: the protocol level at which the option resides
 * @optname: the option name
 * @optval: the option value
 * @optlen: the length of @optval
 *
 * The setsockopt function shall set the option specified by the @optname, at
 * the protocol level specified by the @level, to the value pointed to by the
 * @optval for the socket associated with the file descriptor @fd.
 *
 * Returns: the status of setsockopt
 *
 * Since: 5.12
 */
int hev_task_io_socket_setsockopt (int fd, int level, int optname,
                                   const void *optval, socklen_t optlen);

/**
 * hev_task_io_socket_getsockname:
 * @fd: a file descriptor
 * @addr: socket address buffer
 * @addr_len: socket address buffer length, updated on return
 *
 * The getsockname function shall retrieve the locally-bound name of the
 * specified socket.
 *
 * Returns: the status of getsockname
 *
 * Since: 5.12
 */
int hev_task_io_socket_getsockname (int fd, struct sockaddr *addr,
                                    socklen_t *addr_len);

/**
 * hev_task_io_socket_getpeername:
 * @fd: a file descriptor
 * @addr: socket address buffer
 * @addr_len: socket address buffer length, updated on return
 *
 * The getpeername function shall retrieve the peer address of the specified
 * socket.
 *
 * Returns: the status of getpeername
 *
 * Since: 5.12
 */
int hev_task_io_socket_getpeername (int fd, struct sockaddr *addr,
                                    socklen_t *addr_len);

/**
 * hev_task_io_socket_shutdown:
 * @fd: a file descriptor
 * @how: SHUT_RD, SHUT_WR or SHUT_RDWR
 *
 * The shutdown function shall cause all or part of a full-duplex connection on
 * the socket associated with the file descriptor @fd to be shut down.
 *
 * Returns: the status of shutdown
 *
 * Since: 5.12
 */
int hev_task_io_socket_shutdown (int fd, int how);

/**
 * hev_task_io_socket_get_handle:
 * @fd: a file descriptor
 *
 * Get the native socket behind the file descriptor @fd, which is a SOCKET on
 * Windows and @fd itself elsewhere. The descriptor still owns the socket, do
 * not close it. For use in other threads that need blocking I/O on it.
 *
 * Returns: the native socket, or (uintptr_t)-1 if @fd is not a socket
 *
 * Since: 5.12
 */
uintptr_t hev_task_io_socket_get_handle (int fd);

/**
 * hev_task_io_socket_connect:
 * @fd: a file descriptor
 * @addr: socket address
 * @addr_len: socket address length
 * @yielder: a #HevTaskIOYielder
 * @yielder_data: user data
 *
 * The connect function shall attempt to make a connection on a connection-mode
 * socket or to set or reset the peer address of a connectionless-mode socket.
 *
 * Returns: the status of connect
 *
 * Since: 3.2
 */
int hev_task_io_socket_connect (int fd, const struct sockaddr *addr,
                                socklen_t addr_len, HevTaskIOYielder yielder,
                                void *yielder_data);

/**
 * hev_task_io_socket_accept:
 * @fd: a file descriptor
 * @addr: socket address
 * @addr_len: socket address length
 * @yielder: a #HevTaskIOYielder
 * @yielder_data: user data
 *
 * The accept function shall extract the first connection on the queue of
 * pending connections, create a new socket with the same socket type protocol
 * and address family as the specified socket, and allocate a new file
 * descriptor for that socket.
 *
 * Returns: the file descriptor of accepted socket
 *
 * Since: 3.2
 */
int hev_task_io_socket_accept (int fd, struct sockaddr *addr,
                               socklen_t *addr_len, HevTaskIOYielder yielder,
                               void *yielder_data);

/**
 * hev_task_io_socket_recv:
 * @fd: a file descriptor
 * @buf: buffer
 * @len: buffer length
 * @flags: flags
 * @yielder: a #HevTaskIOYielder
 * @yielder_data: user data
 *
 * The recv function shall receive a message from a connection-mode or
 * connectionless-mode socket. It is normally used with connected sockets
 * because it does not permit the application to retrieve the source address of
 * received data.
 *
 * Returns: the length of the message in bytes
 *
 * Since: 3.2
 */
ssize_t hev_task_io_socket_recv (int fd, void *buf, size_t len, int flags,
                                 HevTaskIOYielder yielder, void *yielder_data);

/**
 * hev_task_io_socket_send:
 * @fd: a file descriptor
 * @buf: buffer
 * @len: buffer length
 * @flags: flags
 * @yielder: a #HevTaskIOYielder
 * @yielder_data: user data
 *
 * The send function shall initiate transmission of a message from the specified
 * socket to its peer. The send function shall send a message only when the
 * socket is connected. If the socket is a connectionless-mode socket, the
 * message shall be sent to the pre-specified peer address.
 *
 * Returns: the length of the message in bytes
 *
 * Since: 3.2
 */
ssize_t hev_task_io_socket_send (int fd, const void *buf, size_t len, int flags,
                                 HevTaskIOYielder yielder, void *yielder_data);

/**
 * hev_task_io_socket_recvfrom:
 * @fd: a file descriptor
 * @buf: buffer
 * @len: buffer length
 * @flags: flags
 * @addr: socket address
 * @addr_len: socket address length
 * @yielder: a #HevTaskIOYielder
 * @yielder_data: user data
 *
 * The recvfrom function shall receive a message from a connection-mode or
 * connectionless-mode socket. It is normally used with connected sockets
 * because it does not permit the application to retrieve the source address of
 * received data.
 *
 * Returns: the length of the message in bytes
 *
 * Since: 3.2
 */
ssize_t hev_task_io_socket_recvfrom (int fd, void *buf, size_t len, int flags,
                                     struct sockaddr *addr, socklen_t *addr_len,
                                     HevTaskIOYielder yielder,
                                     void *yielder_data);

/**
 * hev_task_io_socket_sendto:
 * @fd: a file descriptor
 * @buf: buffer
 * @len: buffer length
 * @flags: flags
 * @addr: socket address
 * @addr_len: socket address length
 * @yielder: a #HevTaskIOYielder
 * @yielder_data: user data
 *
 * The sendto function shall initiate transmission of a message from the
 * specified socket to its peer. The send function shall send a message only
 * when the socket is connected. If the socket is a connectionless-mode socket,
 * the message shall be sent to the pre-specified peer address.
 *
 * Returns: the length of the message in bytes
 *
 * Since: 3.2
 */
ssize_t hev_task_io_socket_sendto (int fd, const void *buf, size_t len,
                                   int flags, const struct sockaddr *addr,
                                   socklen_t addr_len, HevTaskIOYielder yielder,
                                   void *yielder_data);

/**
 * hev_task_io_socket_recvmsg:
 * @fd: a file descriptor
 * @msg: message
 * @flags: flags
 * @yielder: a #HevTaskIOYielder
 * @yielder_data: user data
 *
 * The recvmsg function shall receive a message from a connection-mode or
 * connectionless-mode socket. It is normally used with connected sockets
 * because it does not permit the application to retrieve the source address of
 * received data.
 *
 * Returns: the length of the message in bytes
 *
 * Since: 3.2
 */
ssize_t hev_task_io_socket_recvmsg (int fd, struct msghdr *msg, int flags,
                                    HevTaskIOYielder yielder,
                                    void *yielder_data);

/**
 * hev_task_io_socket_sendmsg:
 * @fd: a file descriptor
 * @msg: message
 * @flags: flags
 * @yielder: a #HevTaskIOYielder
 * @yielder_data: user data
 *
 * The sendmsg function shall initiate transmission of a message from the
 * specified socket to its peer. The send function shall send a message only
 * when the socket is connected. If the socket is a connectionless-mode socket,
 * the message shall be sent to the pre-specified peer address.
 *
 * Returns: the length of the message in bytes
 *
 * Since: 3.2
 */
ssize_t hev_task_io_socket_sendmsg (int fd, const struct msghdr *msg, int flags,
                                    HevTaskIOYielder yielder,
                                    void *yielder_data);

/**
 * hev_task_io_socket_recvmmsg:
 * @fd: a file descriptor
 * @msgv: an array of mmsghdr structures
 * @n: size of mmsghdr array
 * @flags: flags
 * @yielder: a #HevTaskIOYielder
 * @yielder_data: user data
 *
 * The recvmmsg function that allows the caller to receive multiple messages
 * from a socket using a single system call.
 *
 * The recvmmsg is not supported for stream sockets.
 *
 * Returns: the number of messages received in msgv
 *
 * Since: 5.10
 */
int hev_task_io_socket_recvmmsg (int fd, void *msgv, unsigned int n, int flags,
                                 HevTaskIOYielder yielder, void *yielder_data);

/**
 * hev_task_io_socket_sendmmsg:
 * @fd: a file descriptor
 * @msgv: an array of mmsghdr structures
 * @n: size of mmsghdr array
 * @flags: flags
 * @yielder: a #HevTaskIOYielder
 * @yielder_data: user data
 *
 * The sendmmsg  that allows the caller to transmit multiple messages on a
 * socket using a single system call.
 *
 * The sendmmsg is not supported for stream sockets.
 *
 * Returns: the number of messages sent from msgv
 *
 * Since: 5.10
 */
int hev_task_io_socket_sendmmsg (int fd, void *msgv, unsigned int n, int flags,
                                 HevTaskIOYielder yielder, void *yielder_data);

#ifdef __cplusplus
}
#endif

#endif /* __HEV_TASK_IO_SOCKET_H__ */
