/*
 ============================================================================
 Name        : hev-object.c
 Author      : hev <r@hev.cc>
 Copyright   : Copyright (c) 2024 - 2026 hev
 Description : Object
 ============================================================================
 */

#include <hev-object.h>

#include "lib/misc/hev-compiler.h"

static void
_hev_object_destruct (HevObject *self)
{
}

static void
_hev_object_ref (HevObject *self, int ref)
{
    HevObjectClass *kptr = HEV_OBJECT_GET_CLASS (self);

    if (ref) {
        self->ref_count++;
        return;
    }

    self->ref_count--;
    if (self->ref_count)
        return;

    kptr->destruct (self);
}

EXPORT_SYMBOL HevObjectClass *
hev_object_class (void)
{
    static HevObjectClass klass;
    HevObjectClass *kptr = &klass;

    if (!kptr->name) {
        kptr->name = "HevObject";
        kptr->destruct = _hev_object_destruct;
        kptr->ref = _hev_object_ref;
    }

    return kptr;
}

EXPORT_SYMBOL int
hev_object_construct (HevObject *self)
{
    self->klass = HEV_OBJECT_TYPE;
    self->ref_count = 1;

    return 0;
}
