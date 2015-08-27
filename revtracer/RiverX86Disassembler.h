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
	/*void TrackUnusedRegisters(RiverInstruction &ri);
	void TrackUnusedRegistersOperand(RiverInstruction &ri, BYTE optype, const RiverOperand &op);*/

	typedef void(RiverX86Disassembler::*DisassembleOpcodeFunc)(BYTE *&px86, RiverInstruction &ri, DWORD &flags);
	typedef void(RiverX86Disassembler::*DisassembleExtraFunc)(BYTE extra, RiverInstruction &ri);
	typedef void(RiverX86Disassembler::*DisassembleOperandsFunc)(BYTE *&px86, RiverInstruction &ri);

	static DisassembleOpcodeFunc disassembleOpcodes[2][0x100];
	static DisassembleOperandsFunc disassembleOperands[2][0x100];

	static BYTE testFlags[2][0x100];
	static BYTE modFlags[2][0x100];
	void TrackFlagUsage(RiverInstruction &ri);

	static DisassembleOpcodeFunc disassemble0xFFInstr[8];
	static DisassembleOperandsFunc disassemble0xFFOp[8];

	static DisassembleOpcodeFunc disassemble0xF6Instr[8];
	static DisassembleOperandsFunc disassemble0xF6Op[8]; 
	
	static DisassembleOpcodeFunc disassemble0xF7Instr[8];
	static DisassembleOperandsFunc disassemble0xF7Op[8];

	static DisassembleOpcodeFunc disassemble0x0FC7Instr[8];
	static DisassembleOperandsFunc disassemble0x0FC7Op[8];
public :
	bool Init(RiverCodeGen *cg);

	bool Translate(BYTE *&px86, RiverInstruction &rOut, DWORD &flags);
private :
	/* opcodes disassemblers */
	void DisassembleUnkInstr(BYTE *&px86, RiverInstruction &ri, DWORD &flags);

	template <WORD modifiers = 0, WORD tflags = 0> void DisassembleDefaultInstr(BYTE *&px86, RiverInstruction &ri, DWORD &flags) {
		ri.opCode = *px86;
		ri.modifiers |= modifiers;
		ri.specifiers = GetSpecifiers(ri);
		flags |= tflags;
		px86++;
	}

	template <WORD modifiers = 0, WORD tflags = 0> void DisassembleExtInstr(BYTE *&px86, RiverInstruction &ri, DWORD &flags) {
		/*ri.opCode = *px86;
		px86++;

		ri.subOpCode = (*px86 >> 3) & 0x07;
		ri.specifiers = GetSpecifiers(ri);*/

		ri.opCode = *px86;
		px86++;
		ri.subOpCode = (*px86 >> 3) & 0x07;
		ri.modifiers |= modifiers;
		ri.specifiers = GetSpecifiers(ri);
		flags |= tflags;
	}

	template <WORD segment> void DisassembleSegInstr(BYTE *&px86, RiverInstruction &ri, DWORD &flags) {
		flags |= RIVER_FLAG_PFX;
		ri.modifiers &= 0xF8;
		ri.modifiers |= segment;

		px86++;
	}

	template <BYTE base, WORD modifiers = 0> void DisassemblePlusRegInstr(BYTE *&px86, RiverInstruction &ri, DWORD &flags) {
		ri.opCode = base;
		ri.modifiers |= modifiers;
		ri.specifiers = GetSpecifiers(ri);
		DisassembleRegOp(0, ri, *px86 - base);
		px86++;
	}

	template <BYTE regName, WORD modifiers = 0> void DisassembleDefaultRegInstr(BYTE *&px86, RiverInstruction &ri, DWORD &flags) {
		ri.opCode = *px86;
		ri.modifiers |= modifiers;
		ri.specifiers = GetSpecifiers(ri);
		DisassembleRegOp(0, ri, regName);
		px86++;
	}

	template <BYTE regName, WORD modifiers = 0> void DisassembleDefaultSecondRegInstr(BYTE *&px86, RiverInstruction &ri, DWORD &flags) {
		ri.opCode = *px86;
		ri.modifiers |= modifiers;
		ri.specifiers = GetSpecifiers(ri);
		DisassembleRegOp(1, ri, regName);
		px86++;
	}

	template <BYTE regName0, BYTE regName1, WORD modifiers = 0> void DisassembleDefault2RegInstr(BYTE *&px86, RiverInstruction &ri, DWORD &flags) {
		ri.opCode = *px86;
		ri.modifiers |= modifiers;
		ri.specifiers = GetSpecifiers(ri);
		DisassembleRegOp(0, ri, regName0);
		DisassembleRegOp(1, ri, regName1);
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

	void DisassembleRelJmpModRMInstr(BYTE *&px86, RiverInstruction &ri, DWORD &flags) {
		BYTE extra;
		ri.opCode = *px86;
		ri.specifiers = GetSpecifiers(ri);

		px86++;
		DisassembleModRMOp(0, px86, ri, extra);

		ri.opTypes[1] = RIVER_OPTYPE_IMM | RIVER_OPSIZE_32;
		ri.operands[1].asImm32 = (DWORD)px86;
		flags |= RIVER_FLAG_BRANCH;
	}

	/*void DisassembleExtInstr(BYTE *&px86, RiverInstruction &ri, DWORD &flags);*/

	template <DisassembleOpcodeFunc *fSubOps> void DisassembleSubOpInstr(BYTE *&px86, RiverInstruction &ri, DWORD &flags) {
		ri.subOpCode = (*(px86 + 1) >> 3) & 0x7;
		(this->*fSubOps[ri.subOpCode])(px86, ri, flags);
	}

	/* operand helpers */
	void DisassembleImmOp(BYTE opIdx, BYTE *&px86, RiverInstruction &ri, BYTE immSize);
	void DisassembleRegOp(BYTE opIdx, RiverInstruction &ri, BYTE reg);
	void DisassembleModRMOp(BYTE opIdx, BYTE *&px86, RiverInstruction &ri, BYTE &extra);
	void DisassembleSzModRMOp(BYTE opIdx, BYTE *&px86, RiverInstruction &ri, BYTE &extra, WORD sz);
	void DisassembleMoffs8(BYTE opIdx, BYTE *&px86, RiverInstruction &ri);
	void DisassembleMoffs32(BYTE opIdx, BYTE *&px86, RiverInstruction &ri);

	/* extra disassemblers */
	void DropExtra(BYTE extra, RiverInstruction &ri) {}
	void SubOpExtra(BYTE extra, RiverInstruction &ri) {
		ri.subOpCode = extra;
	}
	template <BYTE opIdx> void RegExtra(BYTE extra, RiverInstruction &ri) {
		DisassembleRegOp(opIdx, ri, extra);
	}

	/* operand disassemblers */
	void DisassembleUnkOp(BYTE *&px86, RiverInstruction &ri);
	void DisassembleNoOp(BYTE *&px86, RiverInstruction &ri);
	void DisassembleRegModRM(BYTE *&px86, RiverInstruction &ri);
	void DisassembleModRMReg(BYTE *&px86, RiverInstruction &ri);
	void DisassembleModRMImm8(BYTE *&px86, RiverInstruction &ri);
	void DisassembleModRMImm32(BYTE *&px86, RiverInstruction &ri);
	void DisassembleSubOpModRM(BYTE *&px86, RiverInstruction &ri);
	void DisassembleSubOpModRMImm8(BYTE *&px86, RiverInstruction &ri);
	void DisassembleSubOpModRMImm32(BYTE *&px86, RiverInstruction &ri);
	void DisassembleImm8(BYTE *&px86, RiverInstruction &ri);
	void DisassembleImm16(BYTE *&px86, RiverInstruction &ri); 
	void DisassembleImm32(BYTE *&px86, RiverInstruction &ri);

	void DisassembleImm32Imm16(BYTE *&px86, RiverInstruction &ri);

	void DisassembleRegModRMImm32(BYTE *&px86, RiverInstruction &ri);
	void DisassembleRegModRMImm8(BYTE *&px86, RiverInstruction &ri);
	void DisassembleModRMRegImm8(BYTE *&px86, RiverInstruction &ri);
	
	template <BYTE opIdx> void DisassembleMoffs8(BYTE *&px86, RiverInstruction &ri) {
		DisassembleMoffs32(opIdx, px86, ri);
	}

	template <BYTE opIdx> void DisassembleMoffs32(BYTE *&px86, RiverInstruction &ri) {
		DisassembleMoffs32(opIdx, px86, ri);
	}

	template <BYTE opIdx, BYTE immSize> void DisassembleImmOp(BYTE *&px86, RiverInstruction &ri) {
		DisassembleImmOp(opIdx, px86, ri, immSize);
	}

	template <BYTE opIdx> void DisassembleModRM(BYTE *&px86, RiverInstruction &ri) {
		BYTE extra;
		DisassembleModRMOp(opIdx, px86, ri, extra);
	}

	template <DisassembleOperandsFunc *fSubOps> void DisassembleSubOpOp(BYTE *&px86, RiverInstruction &ri) {
		(this->*fSubOps[ri.subOpCode])(px86, ri);
	}

	template <WORD secondOpSz> void DisassembleRegSzModRM(BYTE *&px86, RiverInstruction &ri) {
		BYTE sec;
		DisassembleSzModRMOp(1, px86, ri, sec, secondOpSz);
		DisassembleRegOp(0, ri, sec);
	}

	template <BYTE opIdx, BYTE opFlg, BYTE regName, DisassembleOperandsFunc cont> void DisassembleConstRegOperand(BYTE *&px86, RiverInstruction &ri) {
		(this->*cont)(px86, ri);

		//ri.opTypes[opIdx] = RIVER_OPTYPE_REG | opFlg;
		//ri.operands[opIdx].asRegister.versioned = codegen->GetCurrentReg(regName);

		DisassembleRegOp(opIdx, ri, regName);
		ri.opTypes[opIdx] |= opFlg;
	}

	template <BYTE opIdx, BYTE opFlg, BYTE base, BYTE disp8, DisassembleOperandsFunc cont> void DisassembleConstMemOperand(BYTE *&px86, RiverInstruction &ri) {
		(this->*cont)(px86, ri);

		RiverAddress32 *rAddr = (RiverAddress32 *)codegen->AllocAddr(ri.modifiers); //new RiverAddress;
		
		rAddr->scaleAndSegment = 0;
		rAddr->type = RIVER_ADDR_BASE | RIVER_ADDR_DIRTY;
		rAddr->type |= (disp8 != 0) ? RIVER_ADDR_DISP8 : 0;
		
		BYTE opType = RIVER_OPTYPE_MEM | opFlg;
		if (RIVER_MODIFIER_O8 & ri.modifiers) {
			opType |= RIVER_OPSIZE_8;
		}
		else if (RIVER_MODIFIER_O16 & ri.modifiers) {
			opType |= RIVER_OPSIZE_16;
		}

		rAddr->base.versioned = codegen->GetCurrentReg(base);
		rAddr->disp.d8 = disp8;

		ri.opTypes[opIdx] = opType;
		ri.operands[opIdx].asAddress = rAddr;
	}
};

#endif
