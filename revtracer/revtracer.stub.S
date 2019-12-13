.intel_syntax noprefix
.global _RevtracerPerform
.global _shadowStack
.global _TracerInitialization
.global _revtracerConfig

.text
_RevtracerPerform:
	xchg esp, _shadowStack
	pusha
	pushf
	
	call _TracerInitialization
	
	popf
	popa
	xchg esp, _shadowStack
	jmp [_revtracerConfig + 0]
