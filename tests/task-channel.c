/*
 ============================================================================
 Name        : task-channel.c
 Author      : Heiher <r@hev.cc>
 Copyright   : Copyright (c) 2019 everyone.
 Description : Task Channel Test
 ============================================================================
 */

#include <stddef.h>
#include <assert.h>

#include <hev-task.h>
#include <hev-task-channel.h>
#include <hev-task-system.h>

static void
task1_entry (void *data)
{
    HevTaskChannel *chan = data;
    int v = 0;

    assert (hev_task_channel_read (chan, &v, sizeof (v)) == sizeof (v));
    assert (v == 1234);

    assert (hev_task_channel_read (chan, &v, sizeof (v)) == sizeof (v));
    assert (v == 5678);

    assert (hev_task_channel_read (chan, &v, sizeof (v)) == -1);

    hev_task_channel_destroy (chan);
}

static void
task2_entry (void *data)
{
    HevTaskChannel *chan = data;
    int v;

    v = 1234;
    assert (hev_task_channel_write (chan, &v, sizeof (v)) == sizeof (v));

    v = 5678;
    assert (hev_task_channel_write (chan, &v, sizeof (v)) == sizeof (v));

    hev_task_channel_destroy (chan);
}

static void
task3_entry (void *data)
{
    HevTaskChannel *chan = data;

    hev_task_channel_destroy (chan);
}

static void
task4_entry (void *data)
{
    HevTaskChannel *chan = data;
    int v;

    assert (hev_task_channel_write (chan, &v, sizeof (v)) == -1);
}

int
main (int argc, char *argv[])
{
    HevTask *task;
    HevTaskChannel *chan1, *chan2;

    assert (hev_task_system_init () == 0);

    assert (hev_task_channel_new (&chan1, &chan2) == 0);

    task = hev_task_new (-1);
    assert (task);
    hev_task_set_priority (task, 1);
    hev_task_run (task, task1_entry, chan1);

    task = hev_task_new (-1);
    assert (task);
    hev_task_set_priority (task, 2);
    hev_task_run (task, task2_entry, chan2);

    assert (hev_task_channel_new (&chan1, &chan2) == 0);

    task = hev_task_new (-1);
    assert (task);
    hev_task_set_priority (task, 1);
    hev_task_run (task, task3_entry, chan1);

    task = hev_task_new (-1);
    assert (task);
    hev_task_set_priority (task, 2);
    hev_task_run (task, task4_entry, chan2);

    hev_task_system_run ();

    hev_task_system_fini ();

    return 0;
}
