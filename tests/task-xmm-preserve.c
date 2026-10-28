/*
 ============================================================================
 Name        : task-xmm-preserve.c
 Author      : Heiher <r@hev.cc>
 Copyright   : Copyright (c) 2026 everyone.
 Description : Task XMM Preserve Test
 ============================================================================
 */

#include <assert.h>
#include <stdint.h>

#include <hev-task.h>
#include <hev-task-system.h>

#if defined(__clang__) && defined(__x86_64__) && defined(_WIN32)
#define XMM_TEST 1
#define TASK_COUNT 4
#define SWITCH_COUNT 50

/* XMM6-15 are nonvolatile on Windows x64 and saved in jmp_buf by _setjmp;
 * patterns must be loaded from the task function itself: a helper that
 * declares XMM6-15 clobbered restores its own copies in its epilogue. */
static void
task_entry (void *data)
{
    uintptr_t id = (uintptr_t)data;
    uint64_t base = (id << 40) | 0x5A5A000000000000ULL;
    uint64_t got[20], pat[20];

    for (int i = 0; i < SWITCH_COUNT; i++) {
        uint64_t want = base | ((uint64_t)i << 8);

        for (int j = 0; j < 16; j++)
            pat[j] = want + j;

        __asm__ volatile ("movdqu 0 (%0), %%xmm6\n\t"
                          "movdqu 16 (%0), %%xmm7\n\t"
                          "movdqu 32 (%0), %%xmm8\n\t"
                          "movdqu 48 (%0), %%xmm9\n\t"
                          "movdqu 64 (%0), %%xmm10\n\t"
                          "movdqu 80 (%0), %%xmm11\n\t"
                          "movdqu 96 (%0), %%xmm12\n\t"
                          "movdqu 112 (%0), %%xmm13\n\t"
                          "movdqu 128 (%0), %%xmm14\n\t"
                          "movdqu 144 (%0), %%xmm15\n\t"
                          :
                          : "r"(pat)
                          : "xmm6", "xmm7", "xmm8", "xmm9", "xmm10", "xmm11",
                            "xmm12", "xmm13", "xmm14", "xmm15", "memory");

        hev_task_yield (HEV_TASK_YIELD);

        __asm__ volatile ("movdqu %%xmm6, 0 (%0)\n\t"
                          "movdqu %%xmm7, 16 (%0)\n\t"
                          "movdqu %%xmm8, 32 (%0)\n\t"
                          "movdqu %%xmm9, 48 (%0)\n\t"
                          "movdqu %%xmm10, 64 (%0)\n\t"
                          "movdqu %%xmm11, 80 (%0)\n\t"
                          "movdqu %%xmm12, 96 (%0)\n\t"
                          "movdqu %%xmm13, 112 (%0)\n\t"
                          "movdqu %%xmm14, 128 (%0)\n\t"
                          "movdqu %%xmm15, 144 (%0)\n\t"
                          :
                          : "r"(got)
                          : "memory");

        for (int j = 0; j < 10; j++)
            assert (got[j] == want + j);
    }
}
#endif

#if defined(__clang__) && defined(__aarch64__) && defined(_WIN32)
#define XMM_TEST 1
#define TASK_COUNT 4
#define SWITCH_COUNT 50

/* d8-d15 are nonvolatile on Windows ARM64 and saved in jmp_buf by _setjmp;
 * patterns must be loaded from the task function itself, as above. */
static void
task_entry (void *data)
{
    uintptr_t id = (uintptr_t)data;
    uint64_t base = (id << 40) | 0x5A5A000000000000ULL;
    uint64_t got[8], pat[8];

    for (int i = 0; i < SWITCH_COUNT; i++) {
        uint64_t want = base | ((uint64_t)i << 8);

        for (int j = 0; j < 8; j++)
            pat[j] = want + j;

        __asm__ volatile ("ldr d8,  [%0, #0]\n\t"
                          "ldr d9,  [%0, #8]\n\t"
                          "ldr d10, [%0, #16]\n\t"
                          "ldr d11, [%0, #24]\n\t"
                          "ldr d12, [%0, #32]\n\t"
                          "ldr d13, [%0, #40]\n\t"
                          "ldr d14, [%0, #48]\n\t"
                          "ldr d15, [%0, #56]\n\t"
                          :
                          : "r"(pat)
                          : "d8", "d9", "d10", "d11", "d12", "d13", "d14",
                            "d15", "memory");

        hev_task_yield (HEV_TASK_YIELD);

        __asm__ volatile ("str d8,  [%0, #0]\n\t"
                          "str d9,  [%0, #8]\n\t"
                          "str d10, [%0, #16]\n\t"
                          "str d11, [%0, #24]\n\t"
                          "str d12, [%0, #32]\n\t"
                          "str d13, [%0, #40]\n\t"
                          "str d14, [%0, #48]\n\t"
                          "str d15, [%0, #56]\n\t"
                          :
                          : "r"(got)
                          : "memory");

        for (int j = 0; j < 8; j++)
            assert (got[j] == want + j);
    }
}
#endif

int
main (void)
{
#if defined(XMM_TEST)
    hev_task_system_init ();

    for (uintptr_t i = 0; i < TASK_COUNT; i++) {
        HevTask *task = hev_task_new (-1);
        assert (task);
        hev_task_run (task, task_entry, (void *)i);
    }

    hev_task_system_run ();
    hev_task_system_fini ();
#endif

    return 0;
}
