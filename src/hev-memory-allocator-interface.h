/*
 ============================================================================
 Name        : hev-memory-allocator-interface.h
 Author      : Heiher <r@hev.cc>
 Copyright   : Copyright (c) 2017 everyone.
 Description : Memory allocator interface
 ============================================================================
 */

#ifndef __HEV_MEMORY_ALLOCATOR_INTERFACE_H__
#define __HEV_MEMORY_ALLOCATOR_INTERFACE_H__

typedef void *(*HevMemoryAllocatorAlloc) (HevMemoryAllocator *self,
                                          size_t size);
typedef void (*HevMemoryAllocatorFree) (HevMemoryAllocator *self, void *ptr);
typedef void (*HevMemoryAllocatorDestroy) (HevMemoryAllocator *self);

struct _HevMemoryAllocator
{
    HevMemoryAllocatorAlloc alloc;
    HevMemoryAllocatorFree free;
    HevMemoryAllocatorDestroy destroy;

    unsigned int ref_count;
};

#endif /* __HEV_MEMORY_ALLOCATOR_INTERFACE_H__ */
