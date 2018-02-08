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
	const nodep::DWORD jmpSize = 5;
	const nodep::DWORD loopSize = 2;
	const nodep::DWORD repinitInstrSize = jmpSize + jmpSize + loopSize + jmpSize;
	const nodep::DWORD farloopInstrSize = jmpSize + 3 + loopSize;
	const nodep::DWORD repfiniInstrSize = jmpSize + jmpSize + farloopInstrSize;

	nodep::DWORD actualCodeSize = cursor - repInitCursor - (repinitInstrSize + repfiniInstrSize);

	// fix jumps
	nodep::BYTE *jmpWrapin = repInitCursor + 1;
	*(nodep::DWORD *)(jmpWrapin) += actualCodeSize;

	nodep::BYTE *jmpCodeout = repInitCursor + jmpSize + jmpSize + loopSize + 1;
	*(nodep::DWORD *)(jmpCodeout) += actualCodeSize;

	nodep::BYTE *jmpLoop = repInitCursor + jmpSize + jmpSize + loopSize + jmpSize + actualCodeSize + 1;
	*(nodep::DWORD *)(jmpLoop) -= actualCodeSize;

	nodep::BYTE *jmpInit = cursor - (loopSize + 3 + jmpSize) + 1;
	*(nodep::DWORD *)(jmpInit) -= actualCodeSize;

	needsRepFix = false;
	repInitCursor = NULL;
}


