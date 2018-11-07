/*
 ============================================================================
 Name        : hev-memory-allocator.c
 Author      : Heiher <r@hev.cc>
 Copyright   : Copyright (c) 2013 everyone.
 Description : Memory allocator
 ============================================================================
 */

#include <stdlib.h>
#include <string.h>

#ifdef ENABLE_PTHREAD
#include <pthread.h>
#endif

#include "hev-memory-allocator.h"
#include "mm/simple/hev-memory-allocator-simple.h"

#ifdef ENABLE_PTHREAD
static pthread_key_t key;
static pthread_once_t key_once = PTHREAD_ONCE_INIT;

static void pthread_key_creator (void);
#else
static HevMemoryAllocator *default_allocator;
#endif

HevMemoryAllocator *
hev_memory_allocator_default (void)
{
#ifdef ENABLE_PTHREAD
    HevMemoryAllocator *default_allocator;

    pthread_once (&key_once, pthread_key_creator);

    default_allocator = pthread_getspecific (key);
    if (!default_allocator) {
        default_allocator = hev_memory_allocator_simple_new ();
        pthread_setspecific (key, default_allocator);
    }
#else
    if (!default_allocator)
        default_allocator = hev_memory_allocator_simple_new ();
#endif

    return default_allocator;
}

HevMemoryAllocator *
hev_memory_allocator_set_default (HevMemoryAllocator *allocator)
{
    HevMemoryAllocator *old_allocator;

#ifdef ENABLE_PTHREAD
    pthread_once (&key_once, pthread_key_creator);

    old_allocator = pthread_getspecific (key);
    pthread_setspecific (key, allocator);
#else
    old_allocator = default_allocator;
    default_allocator = allocator;
#endif

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

#ifdef ENABLE_PTHREAD
static void
pthread_key_creator (void)
{
    pthread_key_create (&key, NULL);
}
#endif

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
