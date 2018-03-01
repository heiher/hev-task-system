/*
 ============================================================================
 Name        : hev-task-execute-aarch64.s
 Author      : Heiher <r@hev.cc>
 Copyright   : Copyright (c) 2017 everyone.
 Description :
 ============================================================================
 */

	.globl  hev_task_execute
	.type   hev_task_execute, %function

hev_task_execute:
	ldr	x3, [x0]
	mov	x4, sp
	stp	x4, x30, [x3, -0x10]
	mov	sp, x3
	sub	sp, sp, 0x10

	blr	x1

	ldp	x4, x30, [sp]
	mov	sp, x4
	ret

	.size   hev_task_execute, . - hev_task_execute

