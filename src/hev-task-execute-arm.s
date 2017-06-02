/*
 ============================================================================
 Name        : hev-task-execute-arm.s
 Author      : Heiher <r@hev.cc>
 Copyright   : Copyright (c) 2017 everyone.
 Description :
 ============================================================================
 */

	.globl  hev_task_execute
	.type   hev_task_execute, %function

hev_task_execute:
	ldr	r3, [r0]
	str	sp, [r3, #-0x4]
	str	lr, [r3, #-0x8]
	mov	sp, r3
	sub	sp, #0x8

	blx	r2

	ldr	lr, [sp]
	ldr	sp, [sp, #0x4]
	bx	lr

	.size   hev_task_execute, . - hev_task_execute

