	.file	"wsm_ep_cache.c"
	.section	.rodata.str1.1,"aMS",@progbits,1
.LC0:
	.string	"mmap failed: %s\n"
	.text
	.p2align 4,,15
	.globl	fill_mem
	.type	fill_mem, @function
fill_mem:
.LFB130:
	.cfi_startproc
	subq	$8, %rsp
	.cfi_def_cfa_offset 16
	movq	size_in_bytes(%rip), %rsi
	xorl	%r9d, %r9d
	xorl	%r8d, %r8d
	xorl	%edi, %edi
	movl	$262176, %ecx
	movl	$3, %edx
	call	mmap
	cmpq	$-1, %rax
	movq	%rax, memory(%rip)
	je	.L6
	movq	size_in_bytes(%rip), %rdx
	movl	$-1, %esi
	movq	%rax, %rdi
	call	memset
	movq	size_in_bytes(%rip), %rsi
	movq	memory(%rip), %rdi
	movl	$1, %edx
	call	fill_memory
.L3:
	xorl	%eax, %eax
	addq	$8, %rsp
	.cfi_remember_state
	.cfi_def_cfa_offset 8
	ret
	.p2align 4,,10
	.p2align 3
.L6:
	.cfi_restore_state
	call	__errno_location
	movl	(%rax), %edi
	call	strerror
	movq	stderr(%rip), %rdi
	movq	%rax, %rcx
	movl	$.LC0, %edx
	movl	$1, %esi
	xorl	%eax, %eax
	call	__fprintf_chk
	jmp	.L3
	.cfi_endproc
.LFE130:
	.size	fill_mem, .-fill_mem
	.section	.rodata.str1.8,"aMS",@progbits,1
	.align 8
.LC1:
	.string	"Nb 64 bits loads from memory = %ld\n"
	.align 8
.LC2:
	.string	"perf_event_open failed for remote cache forward: %s\n"
	.align 8
.LC3:
	.string	"remote cache forward count (core event: OFF_CORE_RESPONSE_0:REMOTE_CACHE_FWD)"
	.section	.rodata.str1.1
.LC4:
	.string	"%-80s = %15ld\n"
.LC5:
	.string	"%p\n"
	.text
	.p2align 4,,15
	.globl	read_memory
	.type	read_memory, @function
read_memory:
.LFB131:
	.cfi_startproc
	pushq	%r12
	.cfi_def_cfa_offset 16
	.cfi_offset 12, -16
	movl	$.LC1, %esi
	movl	$1, %edi
	xorl	%eax, %eax
	pushq	%rbp
	.cfi_def_cfa_offset 24
	.cfi_offset 6, -24
	pushq	%rbx
	.cfi_def_cfa_offset 32
	.cfi_offset 3, -32
	subq	$112, %rsp
	.cfi_def_cfa_offset 144
	movq	size_in_bytes(%rip), %rbx
	shrq	$3, %rbx
	movq	%rbx, %rdx
	call	__printf_chk
	leaq	16(%rsp), %rsi
	xorl	%eax, %eax
	movl	$12, %ecx
	xorl	%r9d, %r9d
	xorl	%edx, %edx
	movl	$-1, %r8d
	movq	%rsi, %rdi
	rep stosq
	movl	$298, %edi
	movl	$96, 20(%rsp)
	movl	$4, 16(%rsp)
	movq	$5439927, 24(%rsp)
	movq	$4147, 72(%rsp)
	movb	$97, 56(%rsp)
	movb	$7, %cl
	call	syscall
	cmpl	$-1, %eax
	movq	%rax, %r12
	je	.L13
	xorl	%edx, %edx
	movl	%eax, %edi
	movl	$9219, %esi
	xorl	%eax, %eax
	call	ioctl
	xorl	%edx, %edx
	xorl	%eax, %eax
	movl	$9216, %esi
	movl	%r12d, %edi
	call	ioctl
	testq	%rbx, %rbx
	movq	memory(%rip), %rbp
	jle	.L10
	.p2align 4,,10
	.p2align 3
.L11:
	movq	0(%rbp), %rax
	subq	$32, %rbx
	testq	%rbx, %rbx
	movq	(%rax), %rax
	movq	(%rax), %rax
	movq	(%rax), %rax
	movq	(%rax), %rax
	movq	(%rax), %rax
	movq	(%rax), %rax
	movq	(%rax), %rax
	movq	(%rax), %rax
	movq	(%rax), %rax
	movq	(%rax), %rax
	movq	(%rax), %rax
	movq	(%rax), %rax
	movq	(%rax), %rax
	movq	(%rax), %rax
	movq	(%rax), %rax
	movq	(%rax), %rax
	movq	(%rax), %rax
	movq	(%rax), %rax
	movq	(%rax), %rax
	movq	(%rax), %rax
	movq	(%rax), %rax
	movq	(%rax), %rax
	movq	(%rax), %rax
	movq	(%rax), %rax
	movq	(%rax), %rax
	movq	(%rax), %rax
	movq	(%rax), %rax
	movq	(%rax), %rdx
	movq	(%rdx), %rdx
	movq	(%rdx), %rdx
	movq	(%rdx), %rbp
	jg	.L11
.L10:
	movl	%r12d, %edi
	xorl	%edx, %edx
	movl	$9217, %esi
	xorl	%eax, %eax
	call	ioctl
	leaq	8(%rsp), %rsi
	movl	%r12d, %edi
	movl	$8, %edx
	call	read
	movq	8(%rsp), %rcx
	movl	$.LC3, %edx
	movl	$.LC4, %esi
	movl	$1, %edi
	xorl	%eax, %eax
	call	__printf_chk
	movq	%rbp, %rdx
	movl	$.LC5, %esi
	movl	$1, %edi
	xorl	%eax, %eax
	call	__printf_chk
	addq	$112, %rsp
	.cfi_remember_state
	.cfi_def_cfa_offset 32
	xorl	%eax, %eax
	popq	%rbx
	.cfi_def_cfa_offset 24
	popq	%rbp
	.cfi_def_cfa_offset 16
	popq	%r12
	.cfi_def_cfa_offset 8
	ret
	.p2align 4,,10
	.p2align 3
.L13:
	.cfi_restore_state
	call	__errno_location
	movl	(%rax), %edi
	call	strerror
	movl	$.LC2, %esi
	movq	%rax, %rdx
	movl	$1, %edi
	xorl	%eax, %eax
	call	__printf_chk
	addq	$112, %rsp
	.cfi_def_cfa_offset 32
	xorl	%eax, %eax
	popq	%rbx
	.cfi_def_cfa_offset 24
	popq	%rbp
	.cfi_def_cfa_offset 16
	popq	%r12
	.cfi_def_cfa_offset 8
	ret
	.cfi_endproc
.LFE131:
	.size	read_memory, .-read_memory
	.section	.rodata.str1.8
	.align 8
.LC6:
	.string	"Error setting affinity to write thread: %d\n"
	.align 8
.LC7:
	.string	"Error creating thread for core 0: %d\n"
	.align 8
.LC8:
	.string	"Error joining fill memory thread: %d\n"
	.align 8
.LC9:
	.string	"Error setting affinity to read thread: %d\n"
	.align 8
.LC10:
	.string	"Error joining read memory thread: %d\n"
	.section	.text.startup,"ax",@progbits
	.p2align 4,,15
	.globl	main
	.type	main, @function
main:
.LFB132:
	.cfi_startproc
	subq	$216, %rsp
	.cfi_def_cfa_offset 224
	leaq	144(%rsp), %rdi
	movq	%fs:40, %rax
	movq	%rax, 200(%rsp)
	xorl	%eax, %eax
	call	pthread_attr_init
	leaq	144(%rsp), %rdi
	xorl	%esi, %esi
	call	pthread_attr_setdetachstate
	leaq	16(%rsp), %rdi
	xorl	%eax, %eax
	call	CPU_ZERO
	leaq	16(%rsp), %rsi
	movl	$7, %edi
	xorl	%eax, %eax
	call	CPU_SET
	leaq	16(%rsp), %rdx
	leaq	144(%rsp), %rdi
	xorl	%eax, %eax
	movl	$128, %esi
	call	pthread_attr_setaffinity_np
	testl	%eax, %eax
	jne	.L30
.L15:
	leaq	144(%rsp), %rsi
	xorl	%ecx, %ecx
	movl	$fill_mem, %edx
	movq	%rsp, %rdi
	call	pthread_create
	testl	%eax, %eax
	js	.L31
.L16:
	movq	(%rsp), %rdi
	leaq	8(%rsp), %rsi
	call	pthread_join
	testl	%eax, %eax
	js	.L32
	leaq	144(%rsp), %rdi
	call	pthread_attr_init
	leaq	144(%rsp), %rdi
	xorl	%esi, %esi
	call	pthread_attr_setdetachstate
	leaq	16(%rsp), %rdi
	xorl	%eax, %eax
	call	CPU_ZERO
	leaq	16(%rsp), %rsi
	movl	$7, %edi
	xorl	%eax, %eax
	call	CPU_SET
	leaq	16(%rsp), %rdx
	leaq	144(%rsp), %rdi
	xorl	%eax, %eax
	movl	$128, %esi
	call	pthread_attr_setaffinity_np
	testl	%eax, %eax
	jne	.L33
.L18:
	leaq	144(%rsp), %rsi
	xorl	%ecx, %ecx
	movl	$read_memory, %edx
	movq	%rsp, %rdi
	call	pthread_create
	testl	%eax, %eax
	js	.L34
.L19:
	movq	(%rsp), %rdi
	leaq	8(%rsp), %rsi
	call	pthread_join
	testl	%eax, %eax
	js	.L35
	xorl	%eax, %eax
	movq	200(%rsp), %rcx
	xorq	%fs:40, %rcx
	jne	.L36
	addq	$216, %rsp
	.cfi_remember_state
	.cfi_def_cfa_offset 8
	ret
.L30:
	.cfi_restore_state
	movl	%eax, %edx
	movl	$.LC6, %esi
	movl	$1, %edi
	xorl	%eax, %eax
	call	__printf_chk
	jmp	.L15
.L33:
	movl	%eax, %edx
	movl	$.LC9, %esi
	movl	$1, %edi
	xorl	%eax, %eax
	call	__printf_chk
	jmp	.L18
.L31:
	movl	%eax, %edx
	movl	$.LC7, %esi
	movl	$1, %edi
	xorl	%eax, %eax
	call	__printf_chk
	jmp	.L16
.L34:
	movl	%eax, %edx
	movl	$.LC7, %esi
	movl	$1, %edi
	xorl	%eax, %eax
	call	__printf_chk
	jmp	.L19
.L36:
	call	__stack_chk_fail
.L32:
	movl	%eax, %edx
	movl	$.LC8, %esi
.L29:
	movl	$1, %edi
	xorl	%eax, %eax
	call	__printf_chk
	orl	$-1, %edi
	call	exit
.L35:
	movl	%eax, %edx
	movl	$.LC10, %esi
	jmp	.L29
	.cfi_endproc
.LFE132:
	.size	main, .-main
	.globl	size_in_bytes
	.data
	.align 8
	.type	size_in_bytes, @object
	.size	size_in_bytes, 8
size_in_bytes:
	.quad	262144
	.comm	memory,8,8
	.ident	"GCC: (Ubuntu/Linaro 4.8.1-10ubuntu9) 4.8.1"
	.section	.note.GNU-stack,"",@progbits
