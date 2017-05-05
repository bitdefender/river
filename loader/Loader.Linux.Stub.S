.intel_syntax noprefix
.global shadowStack
.global loaderConfig
.global LoaderInit

.section .init
	xchg esp, shadowStack
	pusha
	pushf
	
	lea eax, jumphere
	push eax
	call LoaderInit
	add esp, 4
	
	popf
	popa
	xchg esp, shadowStack
	jmp [loaderConfig + 8]
jumphere:
