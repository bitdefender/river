#include "GenericX86Assembler.h"

#define X86_OPZISE_PREFIX			0x66

#define X86_LOCK_PREFIX				0xF0
#define X86_REPNZ_PREFIX			0xF2
#define X86_REPZ_PREFIX				0xF3
#define X86_REP_PREFIX				0xF3

#define X86_ESSEG_PREFIX			0x26
#define X86_CSSEG_PREFIX			0x2E
#define X86_SSSEG_PREFIX			0x36
#define X86_DSSEG_PREFIX			0x3E
#define X86_FSSEG_PREFIX			0x64
#define X86_GSSEG_PREFIX			0x65

bool GenericX86Assembler::Init(RiverRuntime *rt) {
	runtime = rt;
	return true;
}

bool GeneratePrefixes(const RiverInstruction &ri, BYTE *&px86) {
	if (ri.modifiers & RIVER_MODIFIER_LOCK) {
		*px86 = X86_LOCK_PREFIX;
		px86++;
	}

	if (ri.modifiers & RIVER_MODIFIER_O16) {
		*px86 = X86_OPZISE_PREFIX;
		px86++;
	}

	if (ri.modifiers & RIVER_MODIFIER_REP) {
		*px86 = X86_REP_PREFIX;
		px86++;
	}

	if (ri.modifiers & RIVER_MODIFIER_REPZ) {
		*px86 = X86_REPZ_PREFIX;
		px86++;
	}

	if (ri.modifiers & RIVER_MODIFIER_REPNZ) {
		*px86 = X86_REPNZ_PREFIX;
		px86++;
	}

	switch (ri.modifiers & 0x07) {
	case RIVER_MODIFIER_ESSEG:
		*px86 = X86_ESSEG_PREFIX;
		px86++;
		break;
	case RIVER_MODIFIER_CSSEG:
		*px86 = X86_CSSEG_PREFIX;
		px86++;
		break;
	case RIVER_MODIFIER_SSSEG:
		*px86 = X86_SSSEG_PREFIX;
		px86++;
		break;
	case RIVER_MODIFIER_DSSEG:
		*px86 = X86_DSSEG_PREFIX;
		px86++;
		break;
	case RIVER_MODIFIER_FSSEG:
		*px86 = X86_FSSEG_PREFIX;
		px86++;
		break;
	case RIVER_MODIFIER_GSSEG:
		*px86 = X86_GSSEG_PREFIX;
		px86++;
		break;
	}
	return true;
}

bool ClearPrefixes(const RiverInstruction &ri, BYTE *&px86) {
	if (ri.modifiers & 0x07) {
		px86--;
	}

	if (ri.modifiers & RIVER_MODIFIER_REPNZ) {
		px86--;
	}

	if (ri.modifiers & RIVER_MODIFIER_REPZ) {
		px86--;
	}

	if (ri.modifiers & RIVER_MODIFIER_REP) {
		px86--;
	}

	if (ri.modifiers & RIVER_MODIFIER_LOCK) {
		px86--;
	}

	return true;
}
