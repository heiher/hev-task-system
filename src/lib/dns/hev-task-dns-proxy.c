/*
 ============================================================================
 Name        : hev-task-dns-proxy.c
 Author      : Heiher <r@hev.cc>
 Copyright   : Copyright (c) 2021 everyone.
 Description : DNS Proxy
 ============================================================================
 */

#include <poll.h>
#include <errno.h>
#include <unistd.h>
#include <sys/ioctl.h>

#ifdef ENABLE_PTHREAD
#include <pthread.h>
#endif

#include "kern/task/hev-task.h"
#include "kern/core/hev-task-system-private.h"
#include "lib/list/hev-list.h"
#include "lib/misc/hev-compiler.h"
#include "lib/io/basic/hev-task-io.h"
#include "lib/io/socket/hev-task-io-socket.h"
#include "mem/api/hev-memory-allocator-api.h"

#include "hev-task-dns-proxy.h"

typedef enum _HevTaskDNSCallType HevTaskDNSCallType;
typedef struct _HevTaskDNSCall HevTaskDNSCall;
typedef struct _HevTaskDNSCallGetAddrInfo HevTaskDNSCallGetAddrInfo;
typedef struct _HevTaskDNSCallGetNameInfo HevTaskDNSCallGetNameInfo;

enum _HevTaskDNSCallType
{
    HEV_TASK_DNS_CALL_GETADDRINFO,
    HEV_TASK_DNS_CALL_GETNAMEINFO,
};

struct _HevTaskDNSProxy
{
    int client_fd;
    int server_fd;
    HevList call_list;
    HevTaskSchedEntity sched_entity;
};

struct _HevTaskDNSCall
{
    int type;
    int stat;
    HevTask *task;
    HevListNode node;
};

struct _HevTaskDNSCallGetAddrInfo
{
    HevTaskDNSCall base;
    const char *node;
    const char *service;
    const struct addrinfo *hints;
    struct addrinfo **res;
    int ret;
};

struct _HevTaskDNSCallGetNameInfo
{
    HevTaskDNSCall base;
    const struct sockaddr *addr;
    char *node;
    char *service;
    socklen_t addrlen;
    socklen_t nodelen;
    socklen_t servicelen;
    int flags;
    int ret;
};

static HevTaskIOReactor *server_reactor;

#ifdef ENABLE_PTHREAD

static pthread_t thread;
static pthread_cond_t cond = PTHREAD_COND_INITIALIZER;
static pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

static void
hev_task_dns_server_getaddrinfo (HevTaskDNSCall *call)
{
    HevTaskDNSCallGetAddrInfo *gai = (HevTaskDNSCallGetAddrInfo *)call;

    gai->ret = getaddrinfo (gai->node, gai->service, gai->hints, gai->res);
}

static void
hev_task_dns_server_getnameinfo (HevTaskDNSCall *call)
{
    HevTaskDNSCallGetNameInfo *gni = (HevTaskDNSCallGetNameInfo *)call;

    gni->ret = getnameinfo (gni->addr, gni->addrlen, gni->node, gni->nodelen,
                            gni->service, gni->servicelen, gni->flags);
}

static void *
hev_task_dns_server_handler (void *data)
{
    pthread_mutex_lock (&mutex);
    server_reactor = hev_task_io_reactor_new ();
    pthread_cond_signal (&cond);
    pthread_mutex_unlock (&mutex);

    for (;;) {
        int i, count;
        HevTaskIOReactorWaitEvent events[256];

        count = hev_task_io_reactor_wait (server_reactor, events, 256, -1);

        for (i = 0; i < count; i++) {
            HevTaskDNSCall *call;
            int res;
            int fd;

            fd = (intptr_t)hev_task_io_reactor_wait_event_get_data (&events[i]);

            for (;;) {
                res = read (fd, &call, sizeof (call));
                if (0 >= res)
                    break;

                switch (call->type) {
                case HEV_TASK_DNS_CALL_GETADDRINFO:
                    hev_task_dns_server_getaddrinfo (call);
                    break;
                case HEV_TASK_DNS_CALL_GETNAMEINFO:
                    hev_task_dns_server_getnameinfo (call);
                    break;
                }

                for (;;) {
                    struct pollfd pfd;

                    res = write (fd, &call, sizeof (call));
                    if (0 < res)
                        break;

                    pfd.fd = fd;
                    pfd.events = POLLOUT;
                    poll (&pfd, 1, -1);
                }
            }
        }
    }

    return NULL;
}

#endif /* !ENABLE_PTHREAD */

static int
hev_task_dns_server_init (void)
{
    int res = -1;

#ifdef ENABLE_PTHREAD

    if (server_reactor)
        return 0;

    pthread_mutex_lock (&mutex);
    if (!server_reactor) {
        res = pthread_create (&thread, NULL, hev_task_dns_server_handler, NULL);
        if (0 == res) {
            while (!server_reactor)
                pthread_cond_wait (&cond, &mutex);
        }
    }
    pthread_mutex_unlock (&mutex);

#endif /* !ENABLE_PTHREAD */

    return res;
}

HevTaskDNSProxy *
hev_task_dns_proxy_new (void)
{
    HevTaskDNSProxy *self;
    HevTaskIOReactor *reactor;
    HevTaskIOReactorSetupEvent revents[HEV_TASK_IO_REACTOR_EVENT_GEN_MAX];
    int fds[2];
    int count;
    int res;

    res = hev_task_dns_server_init ();
    if (0 > res)
        return NULL;

    self = hev_malloc0 (sizeof (HevTaskDNSProxy));
    if (!self)
        goto exit;

    res = hev_task_io_socket_socketpair (PF_LOCAL, SOCK_SEQPACKET, 0, fds);
    if (0 > res)
        goto free;

    reactor = hev_task_system_get_context ()->reactor;
    count = hev_task_io_reactor_setup_event_gen (revents, fds[0],
                                                 HEV_TASK_IO_REACTOR_OP_ADD,
                                                 POLLIN | POLLOUT,
                                                 &self->sched_entity);
    res = hev_task_io_reactor_setup (reactor, revents, count);
    if (0 > res)
        goto close;

    count = hev_task_io_reactor_setup_event_gen (revents, fds[1],
                                                 HEV_TASK_IO_REACTOR_OP_ADD,
                                                 POLLIN,
                                                 (void *)(intptr_t)fds[1]);
    res = hev_task_io_reactor_setup (server_reactor, revents, count);
    if (0 > res)
        goto close;

    self->client_fd = fds[0];
    self->server_fd = fds[1];

    return self;

close:
    close (fds[0]);
    close (fds[1]);
free:
    hev_free (self);
exit:
    return NULL;
}

void
hev_task_dns_proxy_destroy (HevTaskDNSProxy *self)
{
    close (self->client_fd);
    close (self->server_fd);
    hev_free (self);
}

static void
hev_task_dns_proxy_call (HevTaskDNSProxy *self, HevTaskDNSCall *call)
{
    HevTask *task_self = hev_task_self ();
    HevTaskDNSCall *repl;
    HevListNode *node;
    int res;

    call->stat = 0;
    call->task = task_self;
    call->node.prev = NULL;
    call->node.next = NULL;
    hev_list_add_tail (&self->call_list, &call->node);

    for (;;) {
        res = write (self->client_fd, &call, sizeof (call));
        if ((0 < res) || ((0 > res) && (EAGAIN != errno)))
            break;

        self->sched_entity.task = task_self;
        hev_task_yield (HEV_TASK_WAITIO);
    }

    for (;;) {
        res = read (self->client_fd, &repl, sizeof (repl));
        if (0 >= res) {
            if ((0 > res) && (EAGAIN == errno)) {
                if (call->stat == 0) {
                    self->sched_entity.task = task_self;
                    hev_task_yield (HEV_TASK_WAITIO);
                    continue;
                }
            }
            break;
        }

        repl->stat = 1;
        hev_task_wakeup (repl->task);
    }

    hev_list_del (&self->call_list, &call->node);
    node = hev_list_first (&self->call_list);
    if (node) {
        call = container_of (node, HevTaskDNSCall, node);
        self->sched_entity.task = call->task;
    }
}

int
hev_task_dns_proxy_getaddrinfo (HevTaskDNSProxy *self, const char *node,
                                const char *service,
                                const struct addrinfo *hints,
                                struct addrinfo **res)
{
    HevTaskDNSCallGetAddrInfo gai;

    gai.base.type = HEV_TASK_DNS_CALL_GETADDRINFO;
    gai.node = node;
    gai.service = service;
    gai.hints = hints;
    gai.res = res;
    gai.ret = -1;

    hev_task_dns_proxy_call (self, &gai.base);

    return gai.ret;
}

int
hev_task_dns_proxy_getnameinfo (HevTaskDNSProxy *self,
                                const struct sockaddr *addr, socklen_t addrlen,
                                char *node, socklen_t nodelen, char *service,
                                socklen_t servicelen, int flags)
{
    HevTaskDNSCallGetNameInfo gni;

    gni.base.type = HEV_TASK_DNS_CALL_GETNAMEINFO;
    gni.addr = addr;
    gni.addrlen = addrlen;
    gni.node = node;
    gni.nodelen = nodelen;
    gni.service = service;
    gni.servicelen = servicelen;
    gni.flags = flags;
    gni.ret = -1;

    hev_task_dns_proxy_call (self, &gni.base);

    return gni.ret;
}
