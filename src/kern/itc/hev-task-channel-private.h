/*
 ============================================================================
 Name        : hev-task-channel-private.h
 Author      : Heiher <r@hev.cc>
 Copyright   : Copyright (c) 2019 everyone.
 Description : Channel
 ============================================================================
 */

#ifndef __HEV_TASK_CHANNEL_PRIVATE_H__
#define __HEV_TASK_CHANNEL_PRIVATE_H__

#ifdef __cplusplus
extern "C" {
#endif

#define MAX_BUFFER_SIZE (16384)

typedef union _HevTaskChannelData HevTaskChannelData;
typedef struct _HevTaskChannelBuffer HevTaskChannelBuffer;

union _HevTaskChannelData
{
    char b[MAX_BUFFER_SIZE];
    short h[0];
    int w[0];
    long long d[0];
};

struct _HevTaskChannelBuffer
{
    size_t size;
    HevTaskChannelData *data;
};

struct _HevTaskChannel
{
    HevTaskChannel *peer;

    HevTask *task;

    unsigned int rd_idx;
    unsigned int wr_idx;
    unsigned int use_count;
    unsigned int max_count;
    unsigned int ref_count;

    HevTaskChannelBuffer buffers[0];
};

static inline int
hev_task_channel_is_active (HevTaskChannel *self)
{
    return READ_ONCE (self->peer) != NULL;
}

static inline int
hev_task_channel_is_readable (HevTaskChannel *self)
{
    return READ_ONCE (self->use_count) != 0;
}

static inline int
hev_task_channel_is_writable (HevTaskChannel *peer)
{
    return READ_ONCE (peer->use_count) < READ_ONCE (peer->max_count);
}

#ifdef __cplusplus
}
#endif

#endif /* __HEV_TASK_CHANNEL_PRIVATE_H__ */
