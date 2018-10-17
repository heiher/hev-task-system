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

/**
 * HEV_MEMORY_ALLOCATOR_DEFAULT:
 *
 * Get the default memory allocator.
 *
 * Returns: a #HevMemoryAllocator
 *
 * Since: 1.0
 */
#define HEV_MEMORY_ALLOCATOR_DEFAULT (hev_memory_allocator_default ())

/**
 * HEV_MEMORY_ALLOCATOR_ALLOC:
 * @size: bytes
 *
 * Allocate @size bytes memory from the default memory allocator.
 *
 * Returns: memory address
 *
 * Since: 1.0
 */
#define HEV_MEMORY_ALLOCATOR_ALLOC(size) \
    hev_memory_allocator_alloc (HEV_MEMORY_ALLOCATOR_DEFAULT, size)
/**
 * HEV_MEMORY_ALLOCATOR_FREE:
 * @ptr: memory address
 *
 * Free the memory from memory allocator.
 *
 * Since: 1.0
 */
#define HEV_MEMORY_ALLOCATOR_FREE(ptr) \
    hev_memory_allocator_free (HEV_MEMORY_ALLOCATOR_DEFAULT, ptr)

typedef struct _HevMemoryAllocator HevMemoryAllocator;

/**
 * hev_memory_allocator_default:
 *
 * Get the default memory allocator.
 *
 * Returns: a #HevMemoryAllocator
 *
 * Since: 1.0
 */
HevMemoryAllocator *hev_memory_allocator_default (void);

/**
 * hev_memory_allocator_set_default:
 * @allocator: a #HevMemoryAllocator
 *
 * Set the default memory allocator.
 *
 * Returns: The old #HevMemoryAllocator
 *
 * Since: 1.0
 */
HevMemoryAllocator *
hev_memory_allocator_set_default (HevMemoryAllocator *allocator);

/**
 * hev_memory_allocator_new:
 *
 * Creates a new simple memory allocator.
 *
 * Returns: a new #HevMemoryAllocator
 *
 * Since: 1.0
 */
HevMemoryAllocator *hev_memory_allocator_new (void);

/**
 * hev_memory_allocator_ref:
 * @self: a #HevMemoryAllocator
 *
 * Increases the reference count of the @self by one.
 *
 * Returns: a #HevMemoryAllocator
 *
 * Since: 1.0
 */
HevMemoryAllocator *hev_memory_allocator_ref (HevMemoryAllocator *self);

/**
 * hev_memory_allocator_unref:
 * @self: a #HevMemoryAllocator
 *
 * Decreases the reference count of @self. When its reference count
 * drops to 0, the object is finalized (i.e. its memory is freed).
 *
 * Since: 1.0
 */
void hev_memory_allocator_unref (HevMemoryAllocator *self);

/**
 * hev_memory_allocator_alloc:
 * @self: a #HevMemoryAllocator
 * @size: bytes
 *
 * Allocate @size bytes memory from the default memory allocator.
 *
 * Returns: memory address
 *
 * Since: 1.0
 */
void *hev_memory_allocator_alloc (HevMemoryAllocator *self, size_t size);

/**
 * hev_memory_allocator_free:
 * @self: a #HevMemoryAllocator
 * @ptr: memory address
 *
 * Free the memory from memory allocator.
 *
 * Since: 1.0
 */
void hev_memory_allocator_free (HevMemoryAllocator *self, void *ptr);

/**
 * hev_malloc:
 * @size: bytes
 *
 * Allocate @size bytes memory from the default memory allocator.
 *
 * Returns: memory address
 *
 * Since: 1.0
 */
void *hev_malloc (size_t size);

/**
 * hev_malloc0:
 * @size: bytes
 *
 * Allocate @size bytes memory from the default memory allocator.
 * The memroy content will be cleared to zero.
 *
 * Returns: memory address
 *
 * Since: 1.0
 */
void *hev_malloc0 (size_t size);

/**
 * hev_free:
 * @ptr: memory address
 *
 * Free the memory from memory allocator.
 *
 * Since: 1.0
 */
void hev_free (void *ptr);

#endif /* __HEV_MEMORY_ALLOCATOR_H__ */
