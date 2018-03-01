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
	movq	(%rdi), %rcx
	movq	(%rsp), %rax
	addq	$0x08, %rsp
	movq	%rsp, -0x08(%rcx)
	movq	%rax, -0x10(%rcx)
	movq	%rcx, %rsp
	subq	$0x10, %rsp

	callq	*%rsi

	movq	(%rsp), %rax
	movq	0x8(%rsp), %rsp
	jmpq	*%rax

	.size   hev_task_execute, . - hev_task_execute

