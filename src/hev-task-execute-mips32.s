/*
 ============================================================================
 Name        : hev-task-execute-mips32.s
 Author      : Heiher <r@hev.cc>
 Copyright   : Copyright (c) 2017 everyone.
 Description :
 ============================================================================
 */

	.globl  hev_task_execute
	.ent    hev_task_execute, 0
	.type   hev_task_execute, @function

hev_task_execute:
	lw	$t0, ($a0)
	sw	$sp, -0x4($t0)
	sw	$ra, -0x8($t0)
	addiu	$sp, $t0, -0x10

	move	$t9, $a2
	jalr	$a2

	lw	$ra, 0x8($sp)
	lw	$sp, 0xc($sp)
	jr	$ra

	.end    hev_task_execute
	.size   hev_task_execute, . - hev_task_execute

