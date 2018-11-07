/*
 ============================================================================
 Name        : hev-memory-allocator-api.c
 Author      : Heiher <r@hev.cc>
 Copyright   : Copyright (c) 2018 everyone.
 Description : Memory allocator APIs
 ============================================================================
 */

#include <string.h>

#include "hev-memory-allocator.h"

void *
hev_malloc (size_t size)
{
    HevMemoryAllocator *allocator;

    allocator = hev_memory_allocator_default ();
    return hev_memory_allocator_alloc (allocator, size);
}

void *
hev_malloc0 (size_t size)
{
    HevMemoryAllocator *allocator;
    void *data;

    allocator = hev_memory_allocator_default ();
    data = hev_memory_allocator_alloc (allocator, size);
    if (data)
        memset (data, 0, size);
    return data;
}

void *
hev_realloc (void *ptr, size_t size)
{
    HevMemoryAllocator *allocator;

    allocator = hev_memory_allocator_default ();
    return hev_memory_allocator_realloc (allocator, ptr, size);
}

void
hev_free (void *ptr)
{
    HevMemoryAllocator *allocator;

    allocator = hev_memory_allocator_default ();
    hev_memory_allocator_free (allocator, ptr);
}
