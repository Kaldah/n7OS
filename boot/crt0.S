/*
 * crt0.S
 *
 * Copyright (C) 2003-2007 by Simon Nieuviarts
 *
 * Kernel entry point.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */
#include <n7OS/start.h>

	.text
	.comm   first_stack,FIRST_STACK_SIZE,32
	.comm   multiboot_magic,4,4
	.comm   multiboot_info,4,4

	/* The entry point must be before the multiboot header. */
.global entry
entry:
	/* Setup the code segment. */
	jmp	entry2

	/* The multiboot header. */
	.p2align 2
0:	.long	0x1BADB002		/* magic number. */
	.long	0x00010002		/* flags. */
	.long	-0x1BADB002-0x00010002	/* checksum. */
	.long	0b
	.long	_start
	.long	_edata
	.long	__bss_end
	.long	entry

	/*
	 * This global label is here to realign the debugger on the beginning
	 * of the code
	 */
entry2:
	movl	%eax,multiboot_magic	/* It can be verified later. */
	movl	%ebx,multiboot_info	/* It can be used later. */

	/* We have to set up a stack. */
	leal	first_stack,%esp
	addl	$FIRST_STACK_SIZE,%esp
	xorl	%ebp,%ebp

	/* Clear the flags. */
	pushl	$0
	popfl
	/* Note that the Interrupt Flag is cleared : interrupts are disabled. */

	/* Blank all uninitialized memory */
	movl	$_edata,%edi
	xorl	%eax,%eax
0:	movl	%eax,(%edi)
	addl	$4,%edi
	cmpl	$mem_heap_end,%edi
	jb	0b

	/* Jumps into the kernel starting code. */
	call	kernel_start

	/* This function should never exit. */
hang:
	jmp hang

