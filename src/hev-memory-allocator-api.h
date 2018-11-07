/*
 ============================================================================
 Name        : hev-memory-allocator-api.h
 Author      : Heiher <r@hev.cc>
 Copyright   : Copyright (c) 2018 everyone.
 Description : Memory allocator APIs
 ============================================================================
 */

#ifndef __HEV_MEMORY_ALLOCATOR_API_H__
#define __HEV_MEMORY_ALLOCATOR_API_H__

#include <stddef.h>

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

#endif /* __HEV_MEMORY_ALLOCATOR_API_H__ */
