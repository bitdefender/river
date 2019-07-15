.intel_syntax noprefix
.global RevtracerPerform
.global shadowStack
.global TracerInitialization
.global revtracerConfig

.text
RevtracerPerform:
	xchg esp, shadowStack
	pusha
	pushf
	
	call TracerInitialization
	
	popf
	popa
	xchg esp, shadowStack
	jmp [revtracerConfig + 0]
