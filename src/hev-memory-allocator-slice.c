/*
 ============================================================================
 Name        : hev-memory-allocator-slice.c
 Author      : Heiher <r@hev.cc>
 Copyright   : Copyright (c) 2017 everyone.
 Description : Memory allocator sliced
 ============================================================================
 */

#include <stdio.h>
#include <string.h>

#include "hev-memory-allocator-slice.h"
#include "hev-memory-allocator-interface.h"

#define MAX_CACHED_SLICE_SIZE	(8192)
#define MAX_CACHED_SLICE_COUNT	(1000)

typedef struct _HevMemorySlice HevMemorySlice;

struct _HevMemoryAllocatorSlice
{
	HevMemoryAllocator base;

	unsigned int cached_count;
	HevMemorySlice *cached_mslices[MAX_CACHED_SLICE_SIZE];
};

struct _HevMemorySlice
{
	HevMemorySlice *next;
	HevMemorySlice **owner;
};

static void * _hev_memory_allocator_alloc (HevMemoryAllocator *self, size_t size);
static void _hev_memory_allocator_free (HevMemoryAllocator *self, void *ptr);
static void _hev_memory_allocator_destroy (HevMemoryAllocator *self);

HevMemoryAllocator *
hev_memory_allocator_slice_new (void)
{
	HevMemoryAllocator *allocator = NULL;
	HevMemoryAllocatorSlice *self = NULL;

	allocator = malloc (sizeof (HevMemoryAllocatorSlice));
	if (!allocator)
		return NULL;

	allocator->ref_count = 1;
	allocator->alloc = _hev_memory_allocator_alloc;
	allocator->free = _hev_memory_allocator_free;
	allocator->destroy = _hev_memory_allocator_destroy;

	self = (HevMemoryAllocatorSlice *) allocator;
	self->cached_count = 0;
	memset (self->cached_mslices, 0, sizeof (self->cached_mslices));

	return allocator;
}

static void *
_hev_memory_allocator_alloc (HevMemoryAllocator *allocator, size_t size)
{
	HevMemoryAllocatorSlice *self = (HevMemoryAllocatorSlice *) allocator;
	HevMemorySlice *slice, **owner;

	switch (size) {
	case 0:
		return NULL;
	case 1 ... MAX_CACHED_SLICE_SIZE:
		owner = &self->cached_mslices[size - 1];
		break;
	default:
#ifdef _DEBUG
		printf ("default alloc size: %lu\n", size);
#endif
		slice = malloc (sizeof (HevMemorySlice) + size);
		slice->owner = NULL;
		return slice + 1;
	}

	if (*owner) {
		slice = *owner;
		*owner = slice->next;
		self->cached_count --;
	} else {
#ifdef _DEBUG
		printf ("alloc size: %lu\n", size);
#endif
		slice = malloc (sizeof (HevMemorySlice) + size);
		if (!slice)
			return NULL;

		slice->owner = owner;
	}

	return slice + 1;
}

static void
_hev_memory_allocator_free (HevMemoryAllocator *allocator, void *ptr)
{
	HevMemoryAllocatorSlice *self = (HevMemoryAllocatorSlice *) allocator;
	HevMemorySlice *slice = (HevMemorySlice *) ptr - 1;

	if (!slice->owner || self->cached_count >= MAX_CACHED_SLICE_COUNT) {
		free (slice);
	} else {
		slice->next = *slice->owner;
		*slice->owner = slice;
		self->cached_count ++;
#ifdef _DEBUG
		printf ("cached_count: %u\n", self->cached_count);
#endif
	}
}

static void
_hev_memory_allocator_destroy (HevMemoryAllocator *allocator)
{
	HevMemoryAllocatorSlice *self = (HevMemoryAllocatorSlice *) allocator;
	unsigned int i;

	for (i=0; i<MAX_CACHED_SLICE_SIZE; i++) {
		HevMemorySlice *iter;

		for (iter=self->cached_mslices[i]; iter;) {
			HevMemorySlice *next = iter->next;
			free (iter);
			iter = next;
		}
	}
}

