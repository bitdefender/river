#include "RelocableCodeBuffer.h"

using namespace rev;

RelocableCodeBuffer::RelocableCodeBuffer() {
	buffer = NULL;
	Reset();
}

void RelocableCodeBuffer::Init(BYTE *buff) {
	buffer = buff;
	Reset();
}

void RelocableCodeBuffer::Reset() {
	needsRAFix = false;
	rvAddress = NULL;
	cursor = buffer;
}

void RelocableCodeBuffer::SetRelocation(BYTE *reloc) {
	needsRAFix = true;
	rvAddress = reloc;
}

void RelocableCodeBuffer::CopyToFixed(BYTE *dst) const {
	rev_memcpy(dst, buffer, cursor - buffer);
	if (needsRAFix) {
		DWORD offset = (rvAddress - buffer);

		*(DWORD *)(&dst[offset]) -= (DWORD)buffer;
		*(DWORD *)(&dst[offset]) += (DWORD)dst;
	}
}