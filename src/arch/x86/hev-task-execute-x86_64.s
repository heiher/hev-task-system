/*
 ============================================================================
 Name        : hev-task-execute-x86_64.s
 Author      : Heiher <r@hev.cc>
 Copyright   : Copyright (c) 2017 - 2018 everyone.
 Description :
 ============================================================================
 */

#include "asm.h"

NESTED(hev_task_execute)
    movq  %rsp, %rax
    movq  (%rdi), %rsp
    subq  $0x8, %rsp
    pushq %rax

    callq  *%rsi

    popq  %rsp
    retq
END(hev_task_execute)
