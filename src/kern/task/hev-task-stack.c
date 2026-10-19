/*
 ============================================================================
 Name        : hev-task-stack.c
 Author      : Heiher <r@hev.cc>
 Copyright   : Copyright (c) 2026 everyone.
 Description :
 ============================================================================
 */

#include <stdint.h>
#include <unistd.h>
#include <sys/mman.h>

#include "lib/misc/hev-compiler.h"

#include "hev-task-stack.h"

/*
 * Keep a margin below sp for the red zone and callee frames. It must cover
 * every frame still live below the caller while madvise runs, i.e. the
 * return addresses of hev_task_stack_release and the madvise wrapper.
 */
#define STACK_RELEASE_MARGIN (512)

/* MADV_FREE is lazy, only use it where MADV_DONTNEED doesn't drop pages. */
#if defined(MADV_FREE) && !defined(__linux__)
#define STACK_MADV_RELEASE MADV_FREE
#elif defined(MADV_DONTNEED)
#define STACK_MADV_RELEASE MADV_DONTNEED
#endif

void
hev_task_stack_discard (uintptr_t start, void *sp)
{
#ifdef STACK_MADV_RELEASE
    uintptr_t page_size = getpagesize ();
    uintptr_t end;

    start = ALIGN_UP (start, page_size);
    end = ALIGN_DOWN ((uintptr_t)sp - STACK_RELEASE_MARGIN, page_size);
    if (end > start)
        madvise ((void *)start, end - start, STACK_MADV_RELEASE);
#endif
}
