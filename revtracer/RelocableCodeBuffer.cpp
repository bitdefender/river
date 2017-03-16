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
	needsRAFix = false;
	rvAddress = NULL;
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