/*
 ============================================================================
 Name        : task-channel-select.c
 Author      : Heiher <r@hev.cc>
 Copyright   : Copyright (c) 2019 everyone.
 Description : Task Channel Select Test
 ============================================================================
 */

#include <stddef.h>
#include <assert.h>

#include <hev-task.h>
#include <hev-task-system.h>
#include <hev-task-channel.h>
#include <hev-task-channel-select.h>

static void
task1_entry (void *data)
{
    HevTaskChannelSelect *sel;
    HevTaskChannel *chan = data;
    HevTaskChannel *chans[2];
    HevTaskChannel *c;
    int v = 0;
    const size_t vs = sizeof (v);
    const size_t cs = sizeof (HevTaskChannel *);

    sel = hev_task_channel_select_new ();
    assert (sel != NULL);

    hev_task_channel_select_add (sel, chan);

    assert (hev_task_channel_read (chan, &chans[0], cs) == cs);
    hev_task_channel_select_add (sel, chans[0]);

    assert (hev_task_channel_read (chan, &chans[1], cs) == cs);
    hev_task_channel_select_add (sel, chans[1]);

    c = hev_task_channel_select_read (sel, -1);
    assert (c != NULL);
    assert (hev_task_channel_read (c, &v, vs) == vs);
    assert (v == 1234);

    c = hev_task_channel_select_read (sel, -1);
    assert (c != NULL);
    assert (hev_task_channel_read (c, &v, vs) == vs);
    assert (v == 5678);

    c = hev_task_channel_select_read (sel, -1);
    assert (c != NULL);
    assert (hev_task_channel_read (c, &v, vs) == vs);
    assert (v == 9012);

    assert (hev_task_channel_select_read (sel, 0) == NULL);
    assert (hev_task_channel_select_read (sel, 1) == NULL);

    hev_task_channel_select_del (sel, chans[0]);
    hev_task_channel_select_del (sel, chans[1]);
    hev_task_channel_select_del (sel, chan);
    hev_task_channel_destroy (chans[0]);
    hev_task_channel_destroy (chans[1]);
    hev_task_channel_destroy (chan);
    hev_task_channel_select_destroy (sel);
}

static void
task2_entry (void *data)
{
    HevTaskChannel *chan = data;
    HevTaskChannel *chans[4];
    int v;
    const size_t vs = sizeof (v);
    const size_t cs = sizeof (HevTaskChannel *);

    assert (hev_task_channel_new_with_buffers (&chans[0], &chans[2], 2) == 0);
    assert (hev_task_channel_new_with_buffers (&chans[1], &chans[3], 4) == 0);

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
    HevTaskChannel *chan1, *chan2;
    HevTask *task;

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

    hev_task_system_run ();

    hev_task_system_fini ();

    return 0;
}
