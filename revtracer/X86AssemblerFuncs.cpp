#include "X86AssemblerFuncs.h"

using namespace rev;

void AssembleUnkInstr(const RiverInstruction &ri, RelocableCodeBuffer &px86, DWORD &pFlags, DWORD &instrCounter) {
	__asm int 3;
}

void AssembleDefaultInstr(const RiverInstruction &ri, RelocableCodeBuffer &px86, DWORD &pFlags, DWORD &instrCounter) {
	*px86.cursor = ri.opCode;
	px86.cursor++;
	instrCounter++;
}

void AssemblePlusRegInstr(const RiverInstruction &ri, RelocableCodeBuffer &px86, DWORD &pFlags, DWORD &instrCounter) {
	unsigned char regName = ri.operands[0].asRegister.name & 0x07; // verify if 8 bit operand
	*px86.cursor = ri.opCode + regName;
	px86.cursor++;
	instrCounter++;
}

/* =========================================== */
/* Operand helpers                             */
/* =========================================== */

void AssembleModRMOp(unsigned int opIdx, const RiverInstruction &ri, RelocableCodeBuffer &px86, BYTE extra) {
	ri.operands[opIdx].asAddress->EncodeTox86(px86.cursor, extra, ri.family, ri.modifiers);
}

void AssembleImmOp(unsigned int opIdx, const RiverInstruction &ri, RelocableCodeBuffer &px86, BYTE immSize) {
	switch (immSize) {
	case RIVER_OPSIZE_8:
		*((BYTE *)px86.cursor) = ri.operands[opIdx].asImm8;
		px86.cursor++;
		break;
	case RIVER_OPSIZE_16:
		*((WORD *)px86.cursor) = (WORD)ri.operands[opIdx].asImm16;
		px86.cursor += 2;
		break;
	case RIVER_OPSIZE_32:
		if (ri.modifiers & RIVER_MODIFIER_O16) {
			*((WORD *)px86.cursor) = (WORD)ri.operands[opIdx].asImm16;
			px86.cursor += 2;
		}
		else {
			*((DWORD *)px86.cursor) = ri.operands[opIdx].asImm32;
			px86.cursor += 4;
		}
		break;
	}
}

void AssembleMoffs(unsigned int opIdx, const RiverInstruction &ri, RelocableCodeBuffer &px86, BYTE immSize) {
	switch (immSize) {
	case RIVER_OPSIZE_8:
		*((BYTE *)px86.cursor) = ri.operands[opIdx].asAddress->disp.d8;
		px86.cursor++;
		break;
	case RIVER_OPSIZE_16:
		*((WORD *)px86.cursor) = (WORD)ri.operands[opIdx].asAddress->disp.d32;
		px86.cursor += 2;
		break;
	case RIVER_OPSIZE_32:
		*((DWORD *)px86.cursor) = ri.operands[opIdx].asAddress->disp.d32;
		px86.cursor += 4;
		break;
	}
}

/* =========================================== */
/* Operand assemblers                          */
/* =========================================== */

void AssembleUnknownOp(const RiverInstruction &ri, RelocableCodeBuffer &px86) {
	__asm int 3;
}

void AssembleNoOp(const RiverInstruction &ri, RelocableCodeBuffer &px86) {
}

void AssembleModRMImm8Op(const RiverInstruction &ri, RelocableCodeBuffer &px86) {
	AssembleModRMOp(0, ri, px86, 0);
	AssembleImmOp(1, ri, px86, RIVER_OPSIZE_8);
}

void AssembleModRMImm32Op(const RiverInstruction &ri, RelocableCodeBuffer &px86) {
	AssembleModRMOp(0, ri, px86, 0);
	AssembleImmOp(1, ri, px86, RIVER_OPSIZE_32);
}

void AssembleRegModRMOp(const RiverInstruction &ri, RelocableCodeBuffer &px86) {
	AssembleModRMOp(1, ri, px86, ri.operands[0].asRegister.name);
}

void AssembleModRMRegOp(const RiverInstruction &ri, RelocableCodeBuffer &px86) {
	AssembleModRMOp(0, ri, px86, ri.operands[1].asRegister.name);
}

void AssembleModRMSregOp(const RiverInstruction &ri, RelocableCodeBuffer &px86) {
	AssembleModRMOp(0, ri, px86, ri.operands[1].asRegister.name - RIVER_REG_SEGMENT);
}


void AssembleSubOpModRMOp(const RiverInstruction &ri, RelocableCodeBuffer &px86) {
	AssembleModRMOp(0, ri, px86, ri.subOpCode);
}

void AssembleSubOpModRMImm8Op(const RiverInstruction &ri, RelocableCodeBuffer &px86) {
	AssembleModRMOp(0, ri, px86, ri.subOpCode);
	AssembleImmOp(1, ri, px86, RIVER_OPSIZE_8);
}

void AssembleSubOpModRMImm32Op(const RiverInstruction &ri, RelocableCodeBuffer &px86) {
	AssembleModRMOp(0, ri, px86, ri.subOpCode);
	AssembleImmOp(1, ri, px86, RIVER_OPSIZE_32);
}

void AssembleRegModRMImm32Op(const RiverInstruction &ri, RelocableCodeBuffer &px86) {
	AssembleModRMOp(1, ri, px86, ri.operands[0].asRegister.name);
	AssembleImmOp(2, ri, px86, RIVER_OPSIZE_32);
}

void AssembleRegModRMImm8Op(const RiverInstruction &ri, RelocableCodeBuffer &px86) {
	AssembleModRMOp(1, ri, px86, ri.operands[0].asRegister.name);
	AssembleImmOp(2, ri, px86, RIVER_OPSIZE_8);
}

void AssembleModRMRegImm8Op(const RiverInstruction &ri, RelocableCodeBuffer &px86) {
	AssembleModRMOp(0, ri, px86, ri.operands[1].asRegister.name);
	AssembleImmOp(2, ri, px86, RIVER_OPSIZE_8);
}
