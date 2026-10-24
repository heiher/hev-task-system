/*
 ============================================================================
 Name        : hev-memory-allocator.c
 Author      : Heiher <r@hev.cc>
 Copyright   : Copyright (c) 2013 - 2024 everyone.
 Description : Memory allocator
 ============================================================================
 */

#include "lib/misc/hev-compiler.h"

#include <stdlib.h>
#include <string.h>
#if defined(HEV_MSVC_ABI)
#include <windows.h>
#endif

#include "mem/simple/hev-memory-allocator-simple.h"

#include "hev-memory-allocator.h"

#if defined(HEV_MSVC_ABI)

static SRWLOCK key_lock = SRWLOCK_INIT;
static DWORD key = FLS_OUT_OF_INDEXES;

static DWORD
hev_memory_allocator_get_key (void)
{
    DWORD result;

    AcquireSRWLockExclusive (&key_lock);
    if (key == FLS_OUT_OF_INDEXES)
        key = FlsAlloc (NULL);
    result = key;
    ReleaseSRWLockExclusive (&key_lock);

    return result;
}

void
hev_memory_allocator_thread_key_fini (void)
{
    AcquireSRWLockExclusive (&key_lock);
    if (key != FLS_OUT_OF_INDEXES) {
        FlsFree (key);
        key = FLS_OUT_OF_INDEXES;
    }
    ReleaseSRWLockExclusive (&key_lock);
}

EXPORT_SYMBOL HevMemoryAllocator *
hev_memory_allocator_default (void)
{
    HevMemoryAllocator *allocator;
    DWORD tls = hev_memory_allocator_get_key ();

    if (tls == FLS_OUT_OF_INDEXES)
        return NULL;

    allocator = (HevMemoryAllocator *)FlsGetValue (tls);
    if (!allocator) {
        allocator = hev_memory_allocator_simple_new ();
        if (!allocator)
            return NULL;
        if (!FlsSetValue (tls, allocator)) {
            hev_memory_allocator_unref (allocator);
            return NULL;
        }
    }

    return allocator;
}

EXPORT_SYMBOL HevMemoryAllocator *
hev_memory_allocator_set_default (HevMemoryAllocator *allocator)
{
    HevMemoryAllocator *old_allocator;
    DWORD tls = hev_memory_allocator_get_key ();

    if (tls == FLS_OUT_OF_INDEXES)
        return NULL;

    old_allocator = (HevMemoryAllocator *)FlsGetValue (tls);
    if (!FlsSetValue (tls, allocator))
        return NULL;

    return old_allocator;
}

#else /* POSIX */

static __thread HevMemoryAllocator *tls_default_allocator HEV_TLS_MODEL;

EXPORT_SYMBOL HevMemoryAllocator *
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

EXPORT_SYMBOL HevMemoryAllocator *
hev_memory_allocator_set_default (HevMemoryAllocator *allocator)
{
    HevMemoryAllocator *old_allocator;

    old_allocator = tls_default_allocator;
    tls_default_allocator = allocator;

    return old_allocator;
}

#endif /* HEV_MSVC_ABI */

EXPORT_SYMBOL HevMemoryAllocator *
hev_memory_allocator_ref (HevMemoryAllocator *self)
{
    self->ref_count++;
    return self;
}

EXPORT_SYMBOL void
hev_memory_allocator_unref (HevMemoryAllocator *self)
{
    self->ref_count--;
    if (0 < self->ref_count)
        return;

    if (self->destroy)
        self->destroy (self);
    free (self);
}

EXPORT_SYMBOL void *
hev_memory_allocator_alloc (HevMemoryAllocator *self, size_t size)
{
    return self->alloc (self, size);
}

EXPORT_SYMBOL void *
hev_memory_allocator_realloc (HevMemoryAllocator *self, void *ptr, size_t size)
{
    return self->realloc (self, ptr, size);
}

EXPORT_SYMBOL void
hev_memory_allocator_free (HevMemoryAllocator *self, void *ptr)
{
    return self->free (self, ptr);
}