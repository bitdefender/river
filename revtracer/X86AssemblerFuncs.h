#ifndef _X86_ASSEMBLER_FUNCS_
#define _X86_ASSEMBLER_FUNCS_


#include "river.h"
#include "RelocableCodeBuffer.h"

/* Opcode assembler functions */
void AssembleUnkInstr(const RiverInstruction &ri, RelocableCodeBuffer &px86, rev::DWORD &pFlags, rev::DWORD &instrCounter);
void AssembleDefaultInstr(const RiverInstruction &ri, RelocableCodeBuffer &px86, rev::DWORD &pFlags, rev::DWORD &instrCounter);
void AssemblePlusRegInstr(const RiverInstruction &ri, RelocableCodeBuffer &px86, rev::DWORD &pFlags, rev::DWORD &instrCounter);

/* Operand helper funcs */
void AssembleModRMOp(unsigned int opIdx, const RiverInstruction &ri, RelocableCodeBuffer &px86, rev::BYTE extra);
void AssembleImmOp(unsigned int opIdx, const RiverInstruction &ri, RelocableCodeBuffer &px86, rev::BYTE immSize);
void AssembleMoffs(unsigned int opIdx, const RiverInstruction &ri, RelocableCodeBuffer &px86, rev::BYTE immSize);


/* Operand assembler functions */
void AssembleUnknownOp(const RiverInstruction &ri, RelocableCodeBuffer &px86);
void AssembleNoOp(const RiverInstruction &ri, RelocableCodeBuffer &px86);

template <rev::BYTE opIdx, rev::BYTE immSize> void AssembleImmOp(const RiverInstruction &ri, RelocableCodeBuffer &px86) {
	AssembleImmOp(opIdx, ri, px86, immSize);
}

template <int extra> void AssembleModRMOp(const RiverInstruction &ri, RelocableCodeBuffer &px86) {
	AssembleModRMOp(0, ri, px86, extra);
}

void AssembleModRMImm8Op(const RiverInstruction &ri, RelocableCodeBuffer &px86);
void AssembleModRMImm32Op(const RiverInstruction &ri, RelocableCodeBuffer &px86);

void AssembleRegModRMOp(const RiverInstruction &ri, RelocableCodeBuffer &px86);
void AssembleModRMRegOp(const RiverInstruction &ri, RelocableCodeBuffer &px86);
void AssembleSubOpModRMOp(const RiverInstruction &ri, RelocableCodeBuffer &px86);
void AssembleSubOpModRMImm8Op(const RiverInstruction &ri, RelocableCodeBuffer &px86);
void AssembleSubOpModRMImm32Op(const RiverInstruction &ri, RelocableCodeBuffer &px86);

void AssembleRegModRMImm32Op(const RiverInstruction &ri, RelocableCodeBuffer &px86);
void AssembleRegModRMImm8Op(const RiverInstruction &ri, RelocableCodeBuffer &px86);

void AssembleModRMRegImm8Op(const RiverInstruction &ri, RelocableCodeBuffer &px86);

void AssembleRiverAddSubOp(const RiverInstruction &ri, RelocableCodeBuffer &px86);

template <rev::BYTE opIdx> void AssembleMoffs8(const RiverInstruction &ri, RelocableCodeBuffer &px86) {
	AssembleMoffs(opIdx, ri, px86, RIVER_OPSIZE_32);
}

template <rev::BYTE opIdx> void AssembleMoffs32(const RiverInstruction &ri, RelocableCodeBuffer &px86) {
	AssembleMoffs(opIdx, ri, px86, RIVER_OPSIZE_32);
}

#endif