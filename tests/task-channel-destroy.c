/*
 ============================================================================
 Name        : task-channel-destroy.c
 Author      : Heiher <r@hev.cc>
 Copyright   : Copyright (c) 2025 everyone.
 Description : Task Channel Destroy Test
 ============================================================================
 */

#include <stddef.h>
#include <assert.h>

#include <hev-task.h>
#include <hev-task-channel.h>
#include <hev-task-system.h>

static HevTaskChannel *chan1, *chan2;

static void
task_reader_entry (void *data)
{
    int val;

    /* Blocks until the writer produces; leaves after consuming. */
    assert (hev_task_channel_read (chan1, &val, sizeof (val)) ==
            sizeof (val));
    assert (val == 42);
}

static void
task_writer_entry (void *data)
{
    int val = 42;

    assert (hev_task_channel_write (chan2, &val, sizeof (val)) ==
            sizeof (val));

    /* Wait until the reader consumed and exited, then destroy. The
     * stale waiter pointer must not be woken. */
    hev_task_sleep (50);
    hev_task_channel_destroy (chan1);
}

int
main (int argc, char *argv[])
{
    HevTask *task_reader, *task_writer;

    assert (hev_task_system_init () == 0);
    assert (hev_task_channel_new (&chan1, &chan2) == 0);

    task_reader = hev_task_new (-1);
    assert (task_reader);
    hev_task_ref (task_reader);
    hev_task_run (task_reader, task_reader_entry, NULL);

    task_writer = hev_task_new (-1);
    assert (task_writer);
    hev_task_run (task_writer, task_writer_entry, NULL);

    hev_task_system_run ();

    hev_task_channel_destroy (chan2);
    hev_task_unref (task_reader);

    hev_task_system_fini ();
    return 0;
}
