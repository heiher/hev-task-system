/*
 ============================================================================
 Name        : whandle-rearm.c
 Author      : Heiher <r@hev.cc>
 Copyright   : Copyright (c) 2026 everyone.
 Description : Waitable Handle One-shot and Re-arm Test
 ============================================================================
 */

#include <assert.h>

#include <hev-task.h>
#include <hev-task-system.h>

#ifdef _WIN32
#include <windows.h>

#define LOOPS (1000)

static int woken;

static DWORD WINAPI
signaler (LPVOID p)
{
    Sleep (50);
    SetEvent (p);
    return 0;
}

static void
task_entry (void *data)
{
    HANDLE ev = CreateEventA (NULL, TRUE, FALSE, NULL);
    HANDLE ev2 = CreateEventA (NULL, TRUE, TRUE, NULL);
    HANDLE th;
    int i;

    assert (ev && ev2);

    /* Signaled before add: wakes up at once. */
    assert (hev_task_add_whandle (hev_task_self (), ev2) == 0);
    hev_task_yield (HEV_TASK_WAITIO);
    /* One-shot: stays idle while it is still signaled. */
    assert (hev_task_sleep (100) == 0);
    /* Add an armed handle again, and add a signaled one twice. */
    assert (hev_task_add_whandle (hev_task_self (), ev2) == 0);
    assert (hev_task_add_whandle (hev_task_self (), ev2) == 0);
    hev_task_yield (HEV_TASK_WAITIO);
    hev_task_del_whandle (hev_task_self (), ev2);
    /* Delete with a completion queued, then wait: no wakeup. */
    assert (hev_task_add_whandle (hev_task_self (), ev2) == 0);
    hev_task_del_whandle (hev_task_self (), ev2);
    assert (hev_task_sleep (100) == 0);
    assert (hev_task_add_whandle (hev_task_self (), ev2) == 0);
    hev_task_yield (HEV_TASK_WAITIO);
    hev_task_del_whandle (hev_task_self (), ev2);
    CloseHandle (ev2);

    /* Signaled from another thread while sleeping. */
    assert (hev_task_add_whandle (hev_task_self (), ev) == 0);
    th = CreateThread (NULL, 0, signaler, ev, 0, NULL);
    assert (hev_task_sleep (2000) > 0);
    WaitForSingleObject (th, INFINITE);
    CloseHandle (th);
    ResetEvent (ev);

    for (i = 0; i < LOOPS; i++) {
        SetEvent (ev);
        assert (hev_task_add_whandle (hev_task_self (), ev) == 0);
        hev_task_yield (HEV_TASK_WAITIO);
        woken++;
        ResetEvent (ev);
    }
    assert (woken == LOOPS);

    hev_task_del_whandle (hev_task_self (), ev);
    CloseHandle (ev);
}
#else
static void
task_entry (void *data)
{
}
#endif

int
main (int argc, char *argv[])
{
    HevTask *task;

    assert (hev_task_system_init () == 0);

    task = hev_task_new (-1);
    assert (task);
    hev_task_run (task, task_entry, NULL);

    hev_task_system_run ();

    hev_task_system_fini ();

    return 0;
}
