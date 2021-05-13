/*
 ============================================================================
 Name        : hev-task-dns.h
 Author      : Heiher <r@hev.cc>
 Copyright   : Copyright (c) 2021 everyone.
 Description : DNS
 ============================================================================
 */

#ifndef __HEV_TASK_DNS_H__
#define __HEV_TASK_DNS_H__

#include <netdb.h>
#include <sys/types.h>
#include <sys/socket.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * hev_task_dns_getaddrinfo:
 * @node: an internet host
 * @service: an internet service
 * @hints: an #addrinfo
 * @res: an #addrinfo
 *
 * Get a list of IP addresses and port numbers for host and service.
 *
 * Returns: When successful, returns zero. When error occurs, returns nonzero.
 *
 * Since: 5.0.0
 */
int hev_task_dns_getaddrinfo (const char *node, const char *service,
                              const struct addrinfo *hints,
                              struct addrinfo **res);

#ifdef __cplusplus
}
#endif

#endif /* __HEV_TASK_DNS_H__ */
