	.globl call_feeny 	
	
	.globl label_code
	.globl label_code_end
	
	.globl code_placeholder
	.globl code_placeholder_end
	.globl goto_code
	.globl goto_code_end
	.globl branch_code
	.globl branch_code_end
	

	
call_feeny:
	movq %rdi, %rax
	movq heap(%rip), %rdi 
	movq heap(%rip), %rsi 
	movq operand_top(%rip), %rdx 
	movq sp(%rip), %rcx 
	call *%rax
	movq %rsi, heap(%rip) 
	movq %rdx, operand_top(%rip) 
	movq %rcx, sp(%rip) 
	ret


label_code:

label_code_end:


code_placeholder:
	leaq code_placeholder_end(%rip), %rax
	movq $0xcafebabecafebabe, %r8
	movq %rax, (%r8)
	movq $0xbabecafebabecafe, %rax
	ret
code_placeholder_end:

goto_code:
	leaq after_trap(%rip), %rax
	movq $0xcafebabecafebabe, %r8
	movq %rax, (%r8)
	movq $0xbabecafebabecafe, %rax
	ret
after_trap:
	movq $0xcafebabecafebabe, %r8
	jmp *%r8
goto_code_end:


branch_code:
	leaq after_trap2(%rip), %rax
	movq $0xcafebabecafebabe, %r8
	movq %rax, (%r8)
	movq $0xbabecafebabecafe, %rax
	ret
after_trap2:
	subq $8, %rdx
	cmpq $2, (%rdx)
	je branch_code_end
	movq $0xcafebabecafebabe, %r8
	jmp *%r8

branch_code_end:
