/*
 ============================================================================
 Name        : hev-task-dns-proxy.h
 Author      : Heiher <r@hev.cc>
 Copyright   : Copyright (c) 2021 everyone.
 Description : DNS Proxy
 ============================================================================
 */

#ifndef __HEV_TASK_DNS_PROXY_H__
#define __HEV_TASK_DNS_PROXY_H__

#include <netdb.h>
#include <sys/types.h>
#include <sys/socket.h>

typedef struct _HevTaskDNSProxy HevTaskDNSProxy;

HevTaskDNSProxy *hev_task_dns_proxy_new (void);
void hev_task_dns_proxy_destroy (HevTaskDNSProxy *self);

int hev_task_dns_proxy_getaddrinfo (HevTaskDNSProxy *self, const char *node,
                                    const char *service,
                                    const struct addrinfo *hints,
                                    struct addrinfo **res);

#endif /* __HEV_TASK_DNS_PROXY_H__ */
