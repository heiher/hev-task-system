/*
 ============================================================================
 Name        : hev-sys-thread.c
 Author      : Heiher <r@hev.cc>
 Copyright   : Copyright (c) 2026 everyone.
 Description : Threads and one-time initialization on Win32
 ============================================================================
 */

#define HEV_SYS_RAW
#include "sys/hev-sys-private.h"

#include <process.h>

typedef struct _HevSysThreadStart HevSysThreadStart;

struct _HevSysThreadStart
{
    void *(*entry) (void *);
    void *data;
};

static unsigned __stdcall
hev_sys_thread_entry (void *data)
{
    HevSysThreadStart start = *(HevSysThreadStart *)data;

    free (data);
    start.entry (start.data);

    return 0;
}

int
hev_sys_thread_create (HevSysThread *thread, void *(*entry) (void *),
                       void *data)
{
    HevSysThreadStart *start;
    uintptr_t handle;

    start = malloc (sizeof (HevSysThreadStart));
    if (!start)
        return ENOMEM;

    start->entry = entry;
    start->data = data;

    handle = _beginthreadex (NULL, 0, hev_sys_thread_entry, start, 0, NULL);
    if (!handle) {
        free (start);
        return EAGAIN;
    }

    *thread = (HevSysThread)handle;

    return 0;
}

int
hev_sys_thread_join (HevSysThread thread)
{
    WaitForSingleObject (thread, INFINITE);
    CloseHandle (thread);

    return 0;
}

static BOOL CALLBACK
hev_sys_once_entry (PINIT_ONCE once, PVOID func, PVOID *context)
{
    (void)once;
    (void)context;

    ((void (*) (void))func) ();

    return TRUE;
}

void
hev_sys_once (HevSysOnce *once, void (*func) (void))
{
    InitOnceExecuteOnce (once, hev_sys_once_entry, (PVOID)func, NULL);
}
