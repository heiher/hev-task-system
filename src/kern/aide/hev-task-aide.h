/*
 ============================================================================
 Name        : hev-task-aide.h
 Author      : Heiher <r@hev.cc>
 Copyright   : Copyright (c) 2022 - 2026 everyone.
 Description : Aide
 ============================================================================
 */

#ifndef __HEV_TASK_AIDE_H__
#define __HEV_TASK_AIDE_H__

typedef struct _HevTaskAide HevTaskAide;
typedef struct _HevTaskAideWork HevTaskAideWork;
typedef void (*HevTaskAideWorkHandler) (unsigned int revents, void *data);

struct _HevTaskAideWork
{
    int fd;
    unsigned int events;
    HevTaskAideWorkHandler handler;
    void *data;
};

HevTaskAide *hev_task_aide_get (void);
void hev_task_aide_put (HevTaskAide *self);

int hev_task_aide_is_orphan (HevTaskAide *self);

int hev_task_aide_add (HevTaskAide *self, HevTaskAideWork *work);
int hev_task_aide_del (HevTaskAide *self, HevTaskAideWork *work);

#endif /* __HEV_TASK_AIDE_H__ */
