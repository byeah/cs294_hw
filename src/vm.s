	.globl call_feeny 	
	
	.globl label_code
	.globl label_code_end
	.globl code_placeholder
	.globl code_placeholder_end
	.globl goto_code
	.globl goto_code_end
	.globl branch_code
	.globl branch_code_end
	.globl lit_code
	.globl lit_code_end
	.globl set_local_code
	.globl set_local_code_end
	.globl get_local_code
	.globl get_local_code_end
	.globl set_global_code
	.globl set_global_code_end
	.globl get_global_code
	.globl get_global_code_end
	.globl drop_code
	.globl drop_code_end
	.globl return_code
	.globl return_code_end
	.globl call_code
	.globl call_code_end
	.globl call_slot_code
	.globl call_slot_code_end
	.globl array_code
	.globl array_code_end
	.globl call_init_code
	.globl call_init_code_end
	.globl object_code
	.globl object_code_end
	
call_feeny:
	movq %rdi, %rax
	movq heap(%rip), %rdi 
	leaq heap, %r8
	movq 8(%r8), %rsi 
	movq operand_top(%rip), %rdx 
	movq sp(%rip), %rcx 
	call *%rax
	movq %rdi, heap(%rip) 
	leaq heap, %r8
	movq %rsi, 8(%r8)
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


lit_code:
	movq $0xcafebabecafebabe, %r8
	movq %r8, (%rdx)
	addq $8,%rdx
	
lit_code_end:

set_local_code:
	movq -8(%rdx), %r8
	movq $0xcafebabecafebabe, %r9
	movq %r8, 32(%rcx,%r9,8)
set_local_code_end:

get_local_code:
	movq $0xcafebabecafebabe, %r9
	movq 32(%rcx,%r9,8),%r8
	movq %r8,(%rdx)
	addq $8,%rdx
get_local_code_end:

set_global_code:
	movq -8(%rdx), %r8
	movq $0xcafebabecafebabe, %r9
	movq %r8, (%r9)
set_global_code_end:

get_global_code:
	movq $0xcafebabecafebabe, %r9
	movq (%r9),%r8
	movq %r8,(%rdx)
	addq $8,%rdx
get_global_code_end:

drop_code:
	subq $8,%rdx
drop_code_end:

return_code:
	movq $0xcafebabecafebabe, %r8
	movq %rcx, (%r8)
	movq (%rcx), %rcx
	cmpq $0, %rcx
	je finish_program
	movq $-2,%rax
	jmp 24(%rcx)
finish_program:
	movq $0, %rax
	ret
return_code_end:

call_code:
	movq $0xcafebabecafebabe, %r8
	movq (%r8), %r9
	cmpq $0,%r9
	jne call_func
	leaq call_code_end(%rip), %rax
	movq $0xcafebabecafebabe, %r8
	movq %rax, (%r8)
	movq $0xbabecafebabecafe, %rax
	ret
call_func:
	movq $0xcafebabecafebabe, %r8
	movq (%r8),%r10
	movq %rcx, (%r10)
	leaq call_code_end(%rip), %r11
	movq %r11, 24(%rcx)
	movq %r10,%rcx
	movq $0xcafebabecafebabe, %r11
	addq %r11, %r10
	movq %r10, (%r8)
	jmp %r9
call_code_end:


call_slot_code:
	movq $0xcafebabecafebabe, %r8
	movq $0, %r9
	leaq (%r9,%r8,8), %r10
	movq %rdx, %r8
	subq %r10, %r8
	movq (%r8), %r9
	movq %r9, %r10
	andq $7, %r10
	movq $0xcafebabecafebabe, %r11 
	cmpq $0, %r10
	je int
	leaq call_slot_code_end(%rip), %rax
	movq $0xcafebabecafebabe, %r8
	movq %rax, (%r8)
	movq $0xbabecafebabecafe, %rax
	ret
int:
	mov (%r11), %ax
	movq -8(%rdx), %r10
	subq $8, %rdx
	cmp $0x6461, %ax
	je l1
	cmp $0x7573, %ax
	je l2
	cmp $0x756d, %ax
	je l3
	cmp $0x6964, %ax
	je l4
	cmp $0x6f6d, %ax
	je l5
	cmp $0x746c, %ax
	je l6
	cmp $0x7467, %ax
	je l7
	cmp $0x656c, %ax
	je l8
	cmp $0x6567, %ax
	je l9
	cmp $0x7165, %ax
	je l10
l1:
	addq %r10, %r9
	movq %r9, -8(%rdx)
	jmp call_slot_code_end
l2:
	subq %r10, %r9
	movq %r9, -8(%rdx)
	jmp call_slot_code_end
l3:
	shrq $3, %r10
	imulq %r10, %r9
	movq %r9, -8(%rdx)
	jmp call_slot_code_end
l4:
	movq %rdx, %r11
	xorq %rdx, %rdx
	movq %r9, %rax
	cqo
	idivq %r10
	movq %r11, %rdx
	shlq $3, %rax
	movq %rax, -8(%rdx)
	jmp call_slot_code_end
l5:
	movq %rdx, %r11
	xorq %rdx, %rdx
	movq %r9, %rax
	cqo
	idivq %r10
	movq %rdx, -8(%r11)
	movq %r11, %rdx
	jmp call_slot_code_end
l6:
	cmpq %r10, %r9
	jl lt
	movq $2, -8(%rdx)
	jmp call_slot_code_end
lt:
	movq $0, -8(%rdx)
	jmp call_slot_code_end
l7:
	cmpq %r10, %r9
	jg gt
	movq $2, -8(%rdx)
	jmp call_slot_code_end
gt:
	movq $0, -8(%rdx)
	jmp call_slot_code_end
l8:
	cmpq %r10, %r9
	jle le
	movq $2, -8(%rdx)
	jmp call_slot_code_end
le:
	movq $0, -8(%rdx)
	jmp call_slot_code_end
l9:
	cmpq %r10, %r9
	jge ge
	movq $2, -8(%rdx)
	jmp call_slot_code_end
ge:
	movq $0, -8(%rdx)
	jmp call_slot_code_end
l10:
	cmpq %r10, %r9
	je equal 
	movq $2, -8(%rdx)
	jmp call_slot_code_end
equal:
	movq $0, -8(%rdx)
	jmp call_slot_code_end
call_slot_code_end:

array_code:
	movq -16(%rdx), %r8
	shrq $3, %r8
	movq $16,%r10
	leaq (%r10,%r8,8), %r9
	movq %rsi, %r10
	addq %r10,%r9
	cmpq $0x100000, %r9
	jle alloc_array
	leaq array_code_end(%rip), %rax
	movq $0xcafebabecafebabe, %r8
	movq %rax, (%r8)
	movq $0xbabecafebabecafe, %rax
	ret
alloc_array:
	movq -8(%rdx), %r9
	movq $2, (%rdi,%rsi)
	movq %r8,8(%rdi,%rsi)
	leaq (%rdi,%rsi), %r10
	incq %r10
	subq $8, %rdx
	movq %r10, -8(%rdx)
	movq $0, %r10
	addq $16, %rsi
for_array:
	cmpq %r8,%r10
	jge array_code_end
	movq %r9,(%rdi,%rsi)
	incq %r10
	addq $8,%rsi
	jmp for_array	
array_code_end:

call_init_code:
	movq -8(%rdx), %r8
	subq $8, %rdx
	movq $0xcafebabecafebabe, %r9
	movq %r8, 32(%rcx,%r9)
call_init_code_end:

object_code:
	movq $0xcafebabecafebabe, %r8  
	movq $16,%r10
	leaq (%r10,%r8,8), %r9		  
	movq %rsi, %r10
	addq %r10,%r9
	cmpq $0x100000, %r9
	jle alloc_object
	leaq object_code_end(%rip), %rax
	movq $0xcafebabecafebabe, %r8
	movq %rax, (%r8)
	movq $0xbabecafebabecafe, %rax
	ret
alloc_object:
	movq $0xcafebabecafebabe,%r9 
	movq %r9, (%rdi,%rsi)
	movq $0xcafebabecafebabe,%r9 
	incq %r8
	movq $0,%r10
	
for_object1:
	cmpq %r8,%r10
	jge end_for
	movq -8(%rdx), %r11
	subq $8, %rdx
	movq %r11, (%r9,%r10,8)
	incq %r10
	jmp for_object1
end_for:
	cmpq $2, %r11
	je endif
	decq %r11
endif:
	movq %r11, 8(%rdi,%rsi)
	leaq (%rdi,%rsi), %r10
	incq %r10
	addq $8,%rdx
	movq %r10, -8(%rdx) 
	addq $16, %rsi
	decq %r8
	movq $0, %r10
	leaq -8(%r9,%r8,8),%r9
for_object2:
	cmpq %r8,%r10
	jge object_code_end
	movq (%r9), %r11
	movq %r11,(%rdi,%rsi)
	incq %r10
	addq $8,%rsi
	subq $8,%r9
	jmp for_object2
	
object_code_end:



