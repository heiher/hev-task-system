/*
 ============================================================================
 Name        : hev-task-execute-x86.s
 Author      : Heiher <r@hev.cc>
 Copyright   : Copyright (c) 2017 - 2018 everyone.
 Description :
 ============================================================================
 */

#include "asm.h"

NESTED(hev_task_execute)
    mov  %esp, %eax
    mov  0x4(%esp), %edx
    mov  0x8(%esp), %ecx
    mov  (%edx), %esp
    sub  $0x8, %esp
    push %eax

    push %edx
    call  *%ecx

    mov  0x4(%esp), %esp
    ret
END(hev_task_execute)

#if defined(_WIN32) && !defined(__MSYS__) && !defined(__CYGWIN__)
.def @feat.00
.scl 3
.type 0
.endef
.globl @feat.00
.set @feat.00, 1

NESTED(hev_task_longjmp)
    movl 0x4(%esp), %edx
    movl 0x8(%esp), %eax
    testl %eax, %eax
    jnz 1f
    incl %eax
1:
    movl 0x0(%edx), %ebp
    movl 0x4(%edx), %ebx
    movl 0x8(%edx), %edi
    movl 0xc(%edx), %esi
    movl 0x18(%edx), %ecx
    movl %ecx, %fs:0
    movl 0x10(%edx), %esp
    addl $0x4, %esp
    jmpl *0x14(%edx)
END(hev_task_longjmp)
#endif
