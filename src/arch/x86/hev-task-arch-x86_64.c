/*
 ============================================================================
 Name        : hev-task-arch-x86_64.c
 Author      : Heiher <r@hev.cc>
 Copyright   : Copyright (c) 2026 everyone.
 Description : Win64 layout validation of the x86_64 coroutine backend
 ============================================================================
 */

/* The Win64 hev_task_longjmp backend (arch/x86/hev-task-execute-x86_64.s)
 * restores the callee saved registers from the offsets of the MSVC
 * _JUMP_BUFFER, check that the layout it depends on did not change. */

#include <setjmp.h>
#include <stddef.h>

#if defined(_MSC_VER)
#define MSVC_JB_OFF(field, off)                              \
    _Static_assert (offsetof (_JUMP_BUFFER, field) == (off), \
                    #field " offset changed")
#endif

#if defined(_MSC_VER) && defined(_M_X64)

_Static_assert (sizeof (void *) == 8, "Win64 pointers must be 64 bits");
MSVC_JB_OFF (Rbx, 8);
MSVC_JB_OFF (Rsp, 16);
MSVC_JB_OFF (Rbp, 24);
MSVC_JB_OFF (Rsi, 32);
MSVC_JB_OFF (Rdi, 40);
MSVC_JB_OFF (R12, 48);
MSVC_JB_OFF (R13, 56);
MSVC_JB_OFF (R14, 64);
MSVC_JB_OFF (R15, 72);
MSVC_JB_OFF (Rip, 80);
MSVC_JB_OFF (MxCsr, 88);
MSVC_JB_OFF (FpCsr, 92);
MSVC_JB_OFF (Xmm6, 96);
MSVC_JB_OFF (Xmm15, 240);

#endif
