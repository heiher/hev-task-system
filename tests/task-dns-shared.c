/*
 ============================================================================
 Name        : task-dns-shared.c
 Author      : Heiher <r@hev.cc>
 Copyright   : Copyright (c) 2026 everyone.
 Description : Task DNS Shared Aide Test
 ============================================================================
 */

#include <assert.h>
#include <stddef.h>
#include <string.h>
#include <dirent.h>
#include <pthread.h>

#include <hev-sys.h>
#include <hev-task.h>
#include <hev-task-dns.h>
#include <hev-task-system.h>

#define THREAD_COUNT (8)
#define ROUND_COUNT (50)

static pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t cond = PTHREAD_COND_INITIALIZER;
static int ready_count;
static int done;

static int
get_thread_count (void)
{
#if defined(__linux__)
    struct dirent *ent;
    DIR *dir;
    int count = 0;

    dir = opendir ("/proc/self/task");
    assert (dir);
    while ((ent = readdir (dir)))
        if (ent->d_name[0] != '.')
            count++;
    closedir (dir);

    return count;
#else
    return -1;
#endif
}

static void
getaddrinfo_entry (void *data)
{
    struct addrinfo hints = { 0 };
    struct addrinfo *result = NULL;
    struct sockaddr_in *addr;

    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_NUMERICHOST | AI_NUMERICSERV;

    assert (hev_task_dns_getaddrinfo ("127.0.0.1", "80", &hints, &result) == 0);
    assert (result != NULL);
    addr = (struct sockaddr_in *)result->ai_addr;
    assert (addr->sin_port == htons (80));
    freeaddrinfo (result);
}

static void
resolve (void)
{
    int i;

    for (i = 0; i < 4; i++) {
        HevTask *task = hev_task_new (-1);
        assert (task);
        hev_task_run (task, getaddrinfo_entry, NULL);
    }

    hev_task_system_run ();
}

static void *
hold_entry (void *data)
{
    assert (hev_task_system_init () == 0);
    resolve ();

    pthread_mutex_lock (&mutex);
    ready_count++;
    pthread_cond_broadcast (&cond);
    while (!done)
        pthread_cond_wait (&cond, &mutex);
    pthread_mutex_unlock (&mutex);

    hev_task_system_fini ();

    return NULL;
}

static void *
churn_entry (void *data)
{
    int i;

    for (i = 0; i < ROUND_COUNT; i++) {
        assert (hev_task_system_init () == 0);
        resolve ();
        hev_task_system_fini ();
    }

    return NULL;
}

int
main (int argc, char *argv[])
{
    pthread_t threads[THREAD_COUNT];
    int base;
    int i;

    base = get_thread_count ();

    /* A task system without name resolution starts no aide. */
    assert (hev_task_system_init () == 0);
    hev_task_system_run ();
    if (base > 0)
        assert (get_thread_count () == base);
    hev_task_system_fini ();

    /* Task systems that resolve names at the same time share one aide. */
    for (i = 0; i < THREAD_COUNT; i++)
        assert (pthread_create (&threads[i], NULL, hold_entry, NULL) == 0);

    pthread_mutex_lock (&mutex);
    while (ready_count < THREAD_COUNT)
        pthread_cond_wait (&cond, &mutex);
    if (base > 0)
        assert (get_thread_count () == base + THREAD_COUNT + 1);
    done = 1;
    pthread_cond_broadcast (&cond);
    pthread_mutex_unlock (&mutex);

    for (i = 0; i < THREAD_COUNT; i++)
        pthread_join (threads[i], NULL);

    /* The aide is stopped by the last user. */
    if (base > 0)
        assert (get_thread_count () == base);

    /* Task systems that come and go concurrently restart the aide. */
    for (i = 0; i < THREAD_COUNT; i++)
        assert (pthread_create (&threads[i], NULL, churn_entry, NULL) == 0);
    for (i = 0; i < THREAD_COUNT; i++)
        pthread_join (threads[i], NULL);

    if (base > 0)
        assert (get_thread_count () == base);

    return 0;
}
