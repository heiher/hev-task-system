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
	ldr	x1, [x0]
	mov	x2, sp
	stp	x2, x30, [x1, -0x10]
	mov	sp, x1
	sub	sp, sp, 0x10

	ldp	x1, x0, [x0, 0x08]
	blr	x1

	add	sp, sp, 0x10
	ldp	x2, x30, [sp, -0x10]
	mov	sp, x2
	ret

	.size   hev_task_execute, . - hev_task_execute

