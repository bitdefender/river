#include "RelocableCodeBuffer.h"

using namespace rev;

RelocableCodeBuffer::RelocableCodeBuffer() {
	buffer = NULL;
	Reset();
}

void RelocableCodeBuffer::Init(nodep::BYTE *buff) {
	buffer = buff;
	Reset();
}

void RelocableCodeBuffer::Reset() {
	needsRAFix = needsRepFix = false;
	rvAddress = NULL;
	repInitCursor = NULL;
	cursor = buffer;
}

void RelocableCodeBuffer::SetRelocation(nodep::BYTE *reloc) {
	needsRAFix = true;
	rvAddress = reloc;
}

void RelocableCodeBuffer::CopyToFixed(nodep::BYTE *dst) const {
	rev_memcpy(dst, buffer, cursor - buffer);
	if (needsRAFix) {
		nodep::DWORD offset = (rvAddress - buffer);

		*(nodep::DWORD *)(&dst[offset]) -= (nodep::DWORD)buffer;
		*(nodep::DWORD *)(&dst[offset]) += (nodep::DWORD)dst;
	}
}
 void RelocableCodeBuffer::MarkRepInit() {
	 needsRepFix = true;
	 repInitCursor = cursor;
 }

void RelocableCodeBuffer::MarkRepFini() {
	nodep::DWORD actualCodeSize = cursor - repInitCursor - 17 /*3 * jmp imm32 + loop imm8*/;

	// fix jumps
	nodep::BYTE *jmpRepFiniImm = repInitCursor + 5 /*jmp imm32*/ + 2/*loop imm8*/ + 1; /*jmp imm32*/
	*(nodep::DWORD *)(jmpRepFiniImm) = actualCodeSize + 5 /*jmp loop*/;

	nodep::BYTE *jmpLoopImm = repInitCursor + 5 /*jmp imm32*/ + 2 /*loop imm8*/ + 5 /*jmp imm32*/ + actualCodeSize + 1;
	*(nodep::DWORD *)(jmpLoopImm) = -1 * (5 /*jmp imm32*/ + actualCodeSize + 5 /*repinit*/ + 2 /*loop imm8*/);
	needsRepFix = false;
	repInitCursor = NULL;
}


