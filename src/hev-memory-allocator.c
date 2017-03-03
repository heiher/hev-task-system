/*
 ============================================================================
 Name        : hev-memory-allocator.c
 Author      : Heiher <r@hev.cc>
 Copyright   : Copyright (c) 2013 everyone.
 Description : Memory allocator
 ============================================================================
 */

#include <string.h>

#ifdef ENABLE_PTHREAD
# include <pthread.h>
#endif

#include "hev-memory-allocator.h"
#include "hev-memory-allocator-interface.h"

#ifdef ENABLE_PTHREAD
static pthread_key_t key;
static pthread_once_t key_once = PTHREAD_ONCE_INIT;

static void pthread_key_creator (void);
#else
static HevMemoryAllocator *default_allocator;
#endif

static void * _hev_memory_allocator_alloc (HevMemoryAllocator *self, size_t size);
static void _hev_memory_allocator_free (HevMemoryAllocator *self, void *ptr);

HevMemoryAllocator *
hev_memory_allocator_default (void)
{
#ifdef ENABLE_PTHREAD
	HevMemoryAllocator *default_allocator;

	pthread_once (&key_once, pthread_key_creator);

	default_allocator = pthread_getspecific (key);
	if (!default_allocator) {
		default_allocator = hev_memory_allocator_new ();
		pthread_setspecific (key, default_allocator);
	}
#else
	if (!default_allocator)
		default_allocator = hev_memory_allocator_new ();
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
hev_memory_allocator_new (void)
{
	HevMemoryAllocator *self = NULL;

	self = malloc (sizeof (HevMemoryAllocator));
	if (!self)
		return NULL;

	self->ref_count = 1;
	self->alloc = _hev_memory_allocator_alloc;
	self->free = _hev_memory_allocator_free;
	self->destroy = NULL;

	return self;
}

HevMemoryAllocator *
hev_memory_allocator_ref (HevMemoryAllocator *self)
{
	self->ref_count ++;
	return self;
}

void
hev_memory_allocator_unref (HevMemoryAllocator *self)
{
	self->ref_count --;
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

static void *
_hev_memory_allocator_alloc (HevMemoryAllocator *self, size_t size)
{
	return malloc (size);
}

void *
hev_memory_allocator_alloc (HevMemoryAllocator *self, size_t size)
{
	return self->alloc (self, size);
}

static void
_hev_memory_allocator_free (HevMemoryAllocator *self, void *ptr)
{
	free (ptr);
}

void
hev_memory_allocator_free (HevMemoryAllocator *self, void *ptr)
{
	return self->free (self, ptr);
}

void *
hev_malloc (size_t size)
{
	return HEV_MEMORY_ALLOCATOR_ALLOC (size);
}

void *
hev_malloc0 (size_t size)
{
	void *data = HEV_MEMORY_ALLOCATOR_ALLOC (size);
	if (data)
		memset (data, 0, size);
	return data;
}

void
hev_free (void *ptr)
{
	HEV_MEMORY_ALLOCATOR_FREE (ptr);
}

