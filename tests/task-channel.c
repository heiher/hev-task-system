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
    const size_t vs = sizeof (v);

    assert (hev_task_channel_read (chan, &v, vs) == vs);
    assert (v == 1234);

    assert (hev_task_channel_read (chan, &v, vs) == vs);
    assert (v == 5678);

    assert (hev_task_channel_read (chan, &v, vs) == -1);

    hev_task_channel_destroy (chan);
}

static void
task2_entry (void *data)
{
    HevTaskChannel *chan = data;
    int v;
    const size_t vs = sizeof (v);

    v = 1234;
    assert (hev_task_channel_write (chan, &v, vs) == vs);

    v = 5678;
    assert (hev_task_channel_write (chan, &v, vs) == vs);

    hev_task_channel_destroy (chan);
}

static void
task3_entry (void *data)
{
    HevTaskChannel *chan = data;
    int v = 0;
    const size_t vs = sizeof (v);

    assert (hev_task_channel_read (chan, &v, vs) == vs);
    assert (v == 1234);

    v = 5678;
    assert (hev_task_channel_write (chan, &v, vs) == vs);

    assert (hev_task_channel_read (chan, &v, vs) == vs);
    assert (v == 9012);

    assert (hev_task_channel_write (chan, &v, vs) == -1);

    hev_task_channel_destroy (chan);
}

static void
task4_entry (void *data)
{
    HevTaskChannel *chan = data;
    int v;
    const size_t vs = sizeof (v);

    v = 1234;
    assert (hev_task_channel_write (chan, &v, vs) == vs);

    assert (hev_task_channel_read (chan, &v, vs) == vs);
    assert (v == 5678);

    v = 9012;
    assert (hev_task_channel_write (chan, &v, vs) == vs);

    hev_task_channel_destroy (chan);
}

static void
task5_entry (void *data)
{
    HevTaskChannel *chan = data;
    HevTaskChannel *chans[3];
    int v = 0;
    const size_t vs = sizeof (v);
    const size_t cs = sizeof (HevTaskChannel *);

    assert (hev_task_channel_read (chan, &chans[0], cs) == cs);
    assert (hev_task_channel_read (chan, &chans[1], cs) == cs);
    chans[2] = chan;

    assert (hev_task_channel_select_read (chans, 3, &v, vs, -1) == vs);
    assert (v == 1234);

    assert (hev_task_channel_select_read (chans, 3, &v, vs, -1) == vs);
    assert (v == 5678);

    assert (hev_task_channel_select_read (chans, 3, &v, vs, -1) == vs);
    assert (v == 9012);

    assert (hev_task_channel_select_read (chans, 3, &v, vs, 0) == -1);
    assert (hev_task_channel_select_read (chans, 3, &v, vs, 1) == -1);

    hev_task_channel_destroy (chans[0]);
    hev_task_channel_destroy (chans[1]);
    hev_task_channel_destroy (chan);
}

static void
task6_entry (void *data)
{
    HevTaskChannel *chan = data;
    HevTaskChannel *chans[4];
    int v;
    const size_t vs = sizeof (v);
    const size_t cs = sizeof (HevTaskChannel *);

    assert (hev_task_channel_new (&chans[0], &chans[2]) == 0);
    assert (hev_task_channel_new (&chans[1], &chans[3]) == 0);

    assert (hev_task_channel_write (chan, &chans[0], cs) == cs);
    assert (hev_task_channel_write (chan, &chans[1], cs) == cs);

    v = 1234;
    assert (hev_task_channel_write (chan, &v, vs) == vs);

    v = 5678;
    assert (hev_task_channel_write (chans[2], &v, vs) == vs);

    v = 9012;
    assert (hev_task_channel_write (chans[3], &v, vs) == vs);

    hev_task_channel_destroy (chans[2]);
    hev_task_channel_destroy (chans[3]);
    hev_task_channel_destroy (chan);
}

int
main (int argc, char *argv[])
{
    int i;

    assert (hev_task_system_init () == 0);

    for (i = 0; i < 5; i++) {
        HevTask *task;
        HevTaskChannel *chan1, *chan2;

        assert (hev_task_channel_new_with_buffers (&chan1, &chan2, i) == 0);

        task = hev_task_new (-1);
        assert (task);
        hev_task_set_priority (task, 1);
        hev_task_run (task, task1_entry, chan1);

        task = hev_task_new (-1);
        assert (task);
        hev_task_set_priority (task, 2);
        hev_task_run (task, task2_entry, chan2);

        assert (hev_task_channel_new_with_buffers (&chan1, &chan2, i) == 0);

        task = hev_task_new (-1);
        assert (task);
        hev_task_set_priority (task, 1);
        hev_task_run (task, task3_entry, chan1);

        task = hev_task_new (-1);
        assert (task);
        hev_task_set_priority (task, 2);
        hev_task_run (task, task4_entry, chan2);

        assert (hev_task_channel_new (&chan1, &chan2) == 0);

        task = hev_task_new (-1);
        assert (task);
        hev_task_set_priority (task, 1);
        hev_task_run (task, task5_entry, chan1);

        task = hev_task_new (-1);
        assert (task);
        hev_task_set_priority (task, 2);
        hev_task_run (task, task6_entry, chan2);
    }

    hev_task_system_run ();

    hev_task_system_fini ();

    return 0;
}
