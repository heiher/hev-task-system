/*
 ============================================================================
 Name        : whandle-rearm.c
 Author      : Heiher <r@hev.cc>
 Copyright   : Copyright (c) 2026 everyone.
 Description : Waitable handle re-arm benchmark
 ============================================================================
 */

#include <stdio.h>
#include <assert.h>

#include <hev-task.h>
#include <hev-task-system.h>

#ifdef _WIN32
#include <windows.h>

#define LOOPS (200000)

static int woken;

static void
task_entry (void *data)
{
    HANDLE ev = CreateEventA (NULL, TRUE, FALSE, NULL);
    LARGE_INTEGER f, t0, t1;
    int i;

    assert (ev);

    QueryPerformanceFrequency (&f);
    QueryPerformanceCounter (&t0);
    for (i = 0; i < LOOPS; i++) {
        SetEvent (ev);
        assert (hev_task_add_whandle (hev_task_self (), ev) == 0);
        hev_task_yield (HEV_TASK_WAITIO);
        woken++;
        ResetEvent (ev);
    }
    QueryPerformanceCounter (&t1);
    printf ("re-arm + wake: %.0f ns/op\n",
            (double)(t1.QuadPart - t0.QuadPart) * 1e9 / f.QuadPart / LOOPS);

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
