/*
 ============================================================================
 Name        : hev-memory-allocator.h
 Author      : Heiher <r@hev.cc>
 Copyright   : Copyright (c) 2017 everyone.
 Description : Memory allocator
 ============================================================================
 */

#ifndef __HEV_MEMORY_ALLOCATOR_H__
#define __HEV_MEMORY_ALLOCATOR_H__

#include <stdlib.h>

#define HEV_MEMORY_ALLOCATOR_DEFAULT		(hev_memory_allocator_default ())
#define HEV_MEMORY_ALLOCATOR_ALLOC(size) \
	hev_memory_allocator_alloc (HEV_MEMORY_ALLOCATOR_DEFAULT, size)
#define HEV_MEMORY_ALLOCATOR_FREE(ptr) \
	hev_memory_allocator_free (HEV_MEMORY_ALLOCATOR_DEFAULT, ptr)

typedef struct _HevMemoryAllocator HevMemoryAllocator;

HevMemoryAllocator * hev_memory_allocator_default (void);
HevMemoryAllocator * hev_memory_allocator_set_default (HevMemoryAllocator *allocator);

HevMemoryAllocator * hev_memory_allocator_new (void);

HevMemoryAllocator * hev_memory_allocator_ref (HevMemoryAllocator *self);
void hev_memory_allocator_unref (HevMemoryAllocator *self);

void * hev_memory_allocator_alloc (HevMemoryAllocator *self, size_t size);
void hev_memory_allocator_free (HevMemoryAllocator *self, void *ptr);

void * hev_malloc (size_t size);
void * hev_malloc0 (size_t size);
void hev_free (void *ptr);

#endif /* __HEV_MEMORY_ALLOCATOR_H__ */

