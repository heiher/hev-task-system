/*
 ============================================================================
 Name        : hev-task-execute-x86_64.s
 Author      : Heiher <r@hev.cc>
 Copyright   : Copyright (c) 2017 - 2025 everyone.
 Description :
 ============================================================================
 */

#include "asm.h"

/* The Windows x64 ABI passes arguments in rcx/rdx and requires 0x20 bytes
 * of shadow space (0x28 keeps 16-byte alignment after the pushed return
 * address slot); key on the OS, never on the compiler. */
#if defined(__MSYS__) || (defined(_WIN32) && !defined(__CYGWIN__))
#define REG_ARG0  %rcx
#define REG_ARG1  %rdx
#define STACK_PAD 0x28
#else
#define REG_ARG0  %rdi
#define REG_ARG1  %rsi
#define STACK_PAD 0x08
#endif

NESTED(hev_task_execute)
    movq  %rsp, %rax
    movq  (REG_ARG0), %rsp
    pushq %rax
    subq  $STACK_PAD, %rsp

    callq *REG_ARG1

    movq  STACK_PAD(%rsp), %rsp
    retq
END(hev_task_execute)

#if defined(_WIN32) && !defined(__MSYS__) && !defined(__CYGWIN__)
NESTED(hev_task_longjmp)
    testl   %edx, %edx
    jne     1f
    incl    %edx
1:
    movl    %edx, %eax
    movq    0x08(%rcx), %rbx
    movq    0x20(%rcx), %rsi
    movq    0x28(%rcx), %rdi
    movq    0x30(%rcx), %r12
    movq    0x38(%rcx), %r13
    movq    0x40(%rcx), %r14
    movq    0x48(%rcx), %r15
    ldmxcsr 0x58(%rcx)
    fnclex
    fldcw   0x5c(%rcx)
    movdqa  0x60(%rcx), %xmm6
    movdqa  0x70(%rcx), %xmm7
    movdqa  0x80(%rcx), %xmm8
    movdqa  0x90(%rcx), %xmm9
    movdqa  0xa0(%rcx), %xmm10
    movdqa  0xb0(%rcx), %xmm11
    movdqa  0xc0(%rcx), %xmm12
    movdqa  0xd0(%rcx), %xmm13
    movdqa  0xe0(%rcx), %xmm14
    movdqa  0xf0(%rcx), %xmm15
    movq    0x50(%rcx), %rdx
    movq    0x18(%rcx), %rbp
    movq    0x10(%rcx), %rsp
    jmpq    *%rdx
END(hev_task_longjmp)
#endif
