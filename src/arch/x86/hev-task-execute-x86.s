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
    mov  0x4(%esp), %edx
    mov  (%edx), %ecx
    mov  (%esp), %eax
    add  $0x4, %esp
    mov  %esp, -0x4(%ecx)
    mov  %eax, -0x8(%ecx)
    mov  0x4(%esp), %eax
    mov  %ecx, %esp
    sub  $0x10, %esp

    mov  %edx, (%esp)
    call  *%eax

    mov  0x8(%esp), %eax
    mov  0xc(%esp), %esp
    jmp  *%eax
END(hev_task_execute)
