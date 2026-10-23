/*
 ============================================================================
 Name        : hev-task-arch-x86.c
 Author      : Heiher <r@hev.cc>
 Copyright   : Copyright (c) 2026 everyone.
 Description : Win32 layout validation of the x86 coroutine backend
 ============================================================================
 */

/* The Win32 hev_task_longjmp backend (arch/x86/hev-task-execute-x86.s)
 * restores the callee saved registers from the offsets of the MSVC
 * _JUMP_BUFFER, check that the layout it depends on did not change. */

#include <setjmp.h>
#include <stddef.h>

#if defined(_MSC_VER)
#define MSVC_JB_OFF(field, off)                              \
    _Static_assert (offsetof (_JUMP_BUFFER, field) == (off), \
                    #field " offset changed")
#endif

#if defined(_MSC_VER) && defined(_M_IX86)

_Static_assert (sizeof (void *) == 4, "Win32 pointers must be 32 bits");
_Static_assert (sizeof (jmp_buf) >= 64, "x86 jmp_buf is unexpectedly short");
MSVC_JB_OFF (Ebp, 0);
MSVC_JB_OFF (Ebx, 4);
MSVC_JB_OFF (Edi, 8);
MSVC_JB_OFF (Esi, 12);
MSVC_JB_OFF (Esp, 16);
MSVC_JB_OFF (Eip, 20);
MSVC_JB_OFF (Registration, 24);

#endif
