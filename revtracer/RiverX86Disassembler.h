#ifndef _RIVER_X86_DISASSEMBLER_H
#define _RIVER_X86_DISASSEMBLER_H

#include "extern.h"
#include "river.h"

class RiverCodeGen;

WORD GetSpecifiers(RiverInstruction &ri);

class RiverX86Disassembler {
private:
	RiverCodeGen *codegen;

	void TrackModifiedRegisters(RiverInstruction &ri);

	typedef void(RiverX86Disassembler::*DisassembleOpcodeFunc)(BYTE *&px86, RiverInstruction &ri, DWORD &flags);
	typedef void(RiverX86Disassembler::*DisassembleOperandsFunc)(BYTE *&px86, RiverInstruction &ri);

	static DisassembleOpcodeFunc disassembleOpcodes[2][0x100];
	static DisassembleOperandsFunc disassembleOperands[2][0x100];
public :
	bool Init(RiverCodeGen *cg);

	bool Translate(BYTE *&px86, RiverInstruction &rOut, DWORD &flags);
private :
	/* opcodes disassemblers */
	void DisassembleUnkInstr(BYTE *&px86, RiverInstruction &ri, DWORD &flags);

	template <int modifiers = 0, int tflags = 0> void DisassembleDefaultInstr(BYTE *&px86, RiverInstruction &ri, DWORD &flags) {
		ri.opCode = *px86;
		ri.modifiers |= modifiers;
		ri.specifiers = GetSpecifiers(ri);
		flags |= tflags;
		px86++;
	}

	template <BYTE base> void DisassemblePlusRegInstr(BYTE *&px86, RiverInstruction &ri, DWORD &flags) {
		ri.opCode = base;
		ri.specifiers = GetSpecifiers(ri);
		DisassembleRegOp(0, ri, *px86 - base);
		px86++;
	}

	template <BYTE instrLen> void DisassembleRelJmpInstr(BYTE *&px86, RiverInstruction &ri, DWORD &flags) {
		ri.opCode = *px86;
		ri.specifiers = GetSpecifiers(ri);
		ri.opTypes[1] = RIVER_OPTYPE_IMM | RIVER_OPSIZE_32;
		ri.operands[1].asImm32 = (DWORD)px86 + instrLen;
		flags |= RIVER_FLAG_BRANCH;
		px86++;
	}

	void DisassembleExtInstr(BYTE *&px86, RiverInstruction &ri, DWORD &flags);

	/* operand helpers */
	void DisassembleImmOp(BYTE opIdx, BYTE *&px86, RiverInstruction &ri, BYTE immSize);
	void DisassembleRegOp(BYTE opIdx, RiverInstruction &ri, BYTE reg);
	void DisassembleModRMOp(BYTE opIdx, BYTE *&px86, RiverInstruction &ri, BYTE &extra);

	/* operand assemblers */
	void DisassembleUnkOp(BYTE *&px86, RiverInstruction &ri);
	void DisassembleNoOp(BYTE *&px86, RiverInstruction &ri);
	void DisassembleRegModRM(BYTE *&px86, RiverInstruction &ri);
	void DisassembleModRMReg(BYTE *&px86, RiverInstruction &ri);
	void DisassembleModRMImm8(BYTE *&px86, RiverInstruction &ri);
	void DisassembleImm8(BYTE *&px86, RiverInstruction &ri);
	void DisassembleImm32(BYTE *&px86, RiverInstruction &ri);
};

#endif
