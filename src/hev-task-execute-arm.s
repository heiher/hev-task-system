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
	ldr	r1, [r0]
	str	sp, [r1, #-0x4]
	str	lr, [r1, #-0x8]
	mov	sp, r1
	sub	sp, #0x8

	ldr	r1, [r0, #0x4]
	ldr	r0, [r0, #0x8]
	blx	r1

	ldr	lr, [sp]
	ldr	sp, [sp, #0x4]
	bx	lr

	.size   hev_task_execute, . - hev_task_execute

