/*
 ============================================================================
 Name        : hev-task-execute-x86_64.s
 Author      : Heiher <r@hev.cc>
 Copyright   : Copyright (c) 2017 everyone.
 Description :
 ============================================================================
 */

	.globl  hev_task_execute
	.type   hev_task_execute, @function

hev_task_execute:
	movq	(%rdi), %rsi
	movq	(%rsp), %rax
	addq	$0x08, %rsp
	movq	%rsp, -0x08(%rsi)
	movq	%rax, -0x10(%rsi)
	movq	%rsi, %rsp
	subq	$0x10, %rsp

	movq	0x08(%rdi), %rax
	movq	0x10(%rdi), %rdi
	callq	*%rax

	addq	$0x10, %rsp
	movq	-0x10(%rsp), %rax
	movq	-0x08(%rsp), %rsp
	jmpq	*%rax

	.size   hev_task_execute, . - hev_task_execute

