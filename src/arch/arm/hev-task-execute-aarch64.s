/*
 ============================================================================
 Name        : hev-task-execute-aarch64.s
 Author      : Heiher <r@hev.cc>
 Copyright   : Copyright (c) 2017 - 2021 everyone.
 Description :
 ============================================================================
 */

#include "asm.h"

NESTED(hev_task_execute)
#if defined(_WIN32)
    .seh_proc hev_task_execute
    stp  x29, x30, [sp, #-0x10]!
    .seh_save_fplr_x 16
    mov  x29, sp
    .seh_set_fp
    .seh_endprologue
    ldr  x3, [x0]
    mov  sp, x3
    blr  x1
    .seh_startepilogue
    mov  sp, x29
    .seh_set_fp
    ldp  x29, x30, [sp], #0x10
    .seh_save_fplr_x 16
    .seh_endepilogue
    ret
    .seh_endproc
#else
NESTED(hev_task_execute)
    ldr  x3, [x0]
    mov  x4, sp
    stp  x4, x30, [x3, -0x10]
    sub  sp, x3, 0x10

    blr  x1

    ldp  x4, x30, [sp]
    mov  sp, x4
    ret
#endif
END(hev_task_execute)

#if defined(_WIN32)
NESTED(hev_task_longjmp)
    mov  x9, x0
    cmp  w1, #0
    csinc w1, w1, wzr, ne

    ldr  x19, [x9, #0x10]
    ldr  x20, [x9, #0x18]
    ldr  x21, [x9, #0x20]
    ldr  x22, [x9, #0x28]
    ldr  x23, [x9, #0x30]
    ldr  x24, [x9, #0x38]
    ldr  x25, [x9, #0x40]
    ldr  x26, [x9, #0x48]
    ldr  x27, [x9, #0x50]
    ldr  x28, [x9, #0x58]
    ldr  x29, [x9, #0x60]
    ldr  w10, [x9, #0x78]
    ldr  w11, [x9, #0x7c]
    ldr  d8,  [x9, #0x80]
    ldr  d9,  [x9, #0x88]
    ldr  d10, [x9, #0x90]
    ldr  d11, [x9, #0x98]
    ldr  d12, [x9, #0xa0]
    ldr  d13, [x9, #0xa8]
    ldr  d14, [x9, #0xb0]
    ldr  d15, [x9, #0xb8]
    msr  fpcr, x10
    msr  fpsr, x11

    ldr  x10, [x9, #0x70]
    ldr  x30, [x9, #0x68]
    mov  w0, w1
    mov  sp, x10
    ret
END(hev_task_msvc_longjmp)
#endif
