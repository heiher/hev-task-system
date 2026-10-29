/*
 ============================================================================
 Name        : hev-memory-allocator.c
 Author      : Heiher <r@hev.cc>
 Copyright   : Copyright (c) 2013 - 2024 everyone.
 Description : Memory allocator
 ============================================================================
 */

#include <stdlib.h>
#include <string.h>

#include "lib/misc/hev-compiler.h"
#include "mem/simple/hev-memory-allocator-simple.h"

#include "hev-memory-allocator.h"

static __thread HevMemoryAllocator *tls_default_allocator;

HevMemoryAllocator *
hev_memory_allocator_default (void)
{
    HevMemoryAllocator *allocator;

    allocator = tls_default_allocator;
    if (allocator)
        return allocator;

    allocator = hev_memory_allocator_simple_new ();
    tls_default_allocator = allocator;

    return allocator;
}

HevMemoryAllocator *
hev_memory_allocator_set_default (HevMemoryAllocator *allocator)
{
    HevMemoryAllocator *old_allocator;

    old_allocator = tls_default_allocator;
    tls_default_allocator = allocator;

    return old_allocator;
}

HevMemoryAllocator *
hev_memory_allocator_ref (HevMemoryAllocator *self)
{
    self->ref_count++;
    return self;
}

void
hev_memory_allocator_unref (HevMemoryAllocator *self)
{
    self->ref_count--;
    if (0 < self->ref_count)
        return;

    if (self->destroy)
        self->destroy (self);
    free (self);
}

void *
hev_memory_allocator_alloc (HevMemoryAllocator *self, size_t size)
{
    return self->alloc (self, size);
}

void *
hev_memory_allocator_realloc (HevMemoryAllocator *self, void *ptr, size_t size)
{
    return self->realloc (self, ptr, size);
}

void
hev_memory_allocator_free (HevMemoryAllocator *self, void *ptr)
{
    return self->free (self, ptr);
}
