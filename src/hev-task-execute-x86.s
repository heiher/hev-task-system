/*
 ============================================================================
 Name        : hev-task-execute-x86.s
 Author      : Heiher <r@hev.cc>
 Copyright   : Copyright (c) 2017 everyone.
 Description :
 ============================================================================
 */

	.globl  hev_task_execute
	.type   hev_task_execute, @function

hev_task_execute:
	mov	0x4(%esp), %edx
	mov	(%edx), %ecx
	mov	(%esp), %eax
	add	$0x4, %esp
	mov	%esp, -0x4(%ecx)
	mov	%eax, -0x8(%ecx)
	mov	0x4(%esp), %ebx
	mov	0x8(%esp), %eax
	mov	%ecx, %esp
	sub	$0x10, %esp

	mov	%ebx, 0x4(%esp)
	mov	%edx, (%esp)
	call	*%eax

	mov	0x8(%esp), %eax
	mov	0xc(%esp), %esp
	jmp	*%eax

	.size   hev_task_execute, . - hev_task_execute

