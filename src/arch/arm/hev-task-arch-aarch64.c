/*
 ============================================================================
 Name        : hev-task-arch-aarch64.c
 Author      : Heiher <r@hev.cc>
 Copyright   : Copyright (c) 2026 everyone.
 Description : ARM64 layout validation of the aarch64 coroutine backend
 ============================================================================
 */

/* The ARM64 hev_task_longjmp backend (arch/arm/hev-task-execute-aarch64.s)
 * restores the callee saved registers from the offsets of the MSVC
 * _JUMP_BUFFER, check that the layout it depends on did not change.
 *
 * ARM64EC is not native ARM64: it uses the x64-compatible ABI, its
 * _JUMP_BUFFER layout and unwind model differ, so it is rejected by the
 * backend selection (kern/task/hev-task-execute.S). */

#include <setjmp.h>
#include <stddef.h>

#if defined(_MSC_VER)
#define MSVC_JB_OFF(field, off)                              \
    _Static_assert (offsetof (_JUMP_BUFFER, field) == (off), \
                    #field " offset changed")
#endif

#if defined(_MSC_VER) && defined(_M_ARM64) && !defined(_M_ARM64EC)

_Static_assert (sizeof (void *) == 8, "ARM64 pointers must be 64 bits");
_Static_assert (sizeof (_JUMP_BUFFER) == 192,
                "ARM64 _JUMP_BUFFER size changed");
_Static_assert (sizeof (jmp_buf) == 192, "ARM64 jmp_buf size changed");
MSVC_JB_OFF (Frame, 0);
MSVC_JB_OFF (Reserved, 8);
MSVC_JB_OFF (X19, 16);
MSVC_JB_OFF (X20, 24);
MSVC_JB_OFF (X21, 32);
MSVC_JB_OFF (X22, 40);
MSVC_JB_OFF (X23, 48);
MSVC_JB_OFF (X24, 56);
MSVC_JB_OFF (X25, 64);
MSVC_JB_OFF (X26, 72);
MSVC_JB_OFF (X27, 80);
MSVC_JB_OFF (X28, 88);
MSVC_JB_OFF (Fp, 96);
MSVC_JB_OFF (Lr, 104);
MSVC_JB_OFF (Sp, 112);
MSVC_JB_OFF (Fpcr, 120);
MSVC_JB_OFF (Fpsr, 124);
MSVC_JB_OFF (D, 128);
_Static_assert (sizeof (((_JUMP_BUFFER *)0)->D) == 64,
                "ARM64 D8-D15 size changed");

#endif
