#ifndef _RIVER_X86_DISASSEMBLER_H
#define _RIVER_X86_DISASSEMBLER_H

#include "revtracer.h"
#include "river.h"

class RiverCodeGen;

rev::WORD GetSpecifiers(RiverInstruction &ri);

class RiverX86Disassembler {
private:
	RiverCodeGen *codegen;

	void TrackModifiedRegisters(RiverInstruction &ri);
	/*void TrackUnusedRegisters(RiverInstruction &ri);
	void TrackUnusedRegistersOperand(RiverInstruction &ri, BYTE optype, const RiverOperand &op);*/

	typedef void(RiverX86Disassembler::*DisassembleOpcodeFunc)(rev::BYTE *&px86, RiverInstruction &ri, rev::DWORD &flags);
	typedef void(RiverX86Disassembler::*DisassembleExtraFunc)(rev::BYTE extra, RiverInstruction &ri);
	typedef void(RiverX86Disassembler::*DisassembleOperandsFunc)(rev::BYTE *&px86, RiverInstruction &ri);

	static DisassembleOpcodeFunc disassembleOpcodes[2][0x100];
	static DisassembleOperandsFunc disassembleOperands[2][0x100];

	static rev::BYTE testFlags[2][0x100];
	static rev::BYTE modFlags[2][0x100];
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

	bool Translate(rev::BYTE *&px86, RiverInstruction &rOut, rev::DWORD &flags);
private :
	/* opcodes disassemblers */
	void DisassembleUnkInstr(rev::BYTE *&px86, RiverInstruction &ri, rev::DWORD &flags);

	template <rev::WORD modifiers = 0, rev::WORD tflags = 0> void DisassembleDefaultInstr(rev::BYTE *&px86, RiverInstruction &ri, rev::DWORD &flags) {
		ri.opCode = *px86;
		ri.modifiers |= modifiers;
		ri.specifiers = GetSpecifiers(ri);
		flags |= tflags;
		px86++;
	}

	template <rev::WORD modifiers = 0, rev::WORD tflags = 0> void DisassembleExtInstr(rev::BYTE *&px86, RiverInstruction &ri, rev::DWORD &flags) {
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

	template <rev::WORD segment> void DisassembleSegInstr(rev::BYTE *&px86, RiverInstruction &ri, rev::DWORD &flags) {
		flags |= RIVER_FLAG_PFX;
		ri.modifiers &= 0xF8;
		ri.modifiers |= segment;

		px86++;
	}

	template <rev::BYTE base, rev::WORD modifiers = 0> void DisassemblePlusRegInstr(rev::BYTE *&px86, RiverInstruction &ri, rev::DWORD &flags) {
		ri.opCode = base;
		ri.modifiers |= modifiers;
		ri.specifiers = GetSpecifiers(ri);
		DisassembleRegOp(0, ri, *px86 - base);
		px86++;
	}

	template <rev::BYTE regName, rev::WORD modifiers = 0> void DisassembleDefaultRegInstr(rev::BYTE *&px86, RiverInstruction &ri, rev::DWORD &flags) {
		ri.opCode = *px86;
		ri.modifiers |= modifiers;
		ri.specifiers = GetSpecifiers(ri);
		DisassembleRegOp(0, ri, regName);
		px86++;
	}

	template <rev::BYTE regName, rev::WORD modifiers = 0> void DisassembleDefaultSecondRegInstr(rev::BYTE *&px86, RiverInstruction &ri, rev::DWORD &flags) {
		ri.opCode = *px86;
		ri.modifiers |= modifiers;
		ri.specifiers = GetSpecifiers(ri);
		DisassembleRegOp(1, ri, regName);
		px86++;
	}

	template <rev::BYTE regName0, rev::BYTE regName1, rev::WORD modifiers = 0> void DisassembleDefault2RegInstr(rev::BYTE *&px86, RiverInstruction &ri, rev::DWORD &flags) {
		ri.opCode = *px86;
		ri.modifiers |= modifiers;
		ri.specifiers = GetSpecifiers(ri);
		DisassembleRegOp(0, ri, regName0);
		DisassembleRegOp(1, ri, regName1);
		px86++;
	}

	template <rev::BYTE instrLen> void DisassembleRelJmpInstr(rev::BYTE *&px86, RiverInstruction &ri, rev::DWORD &flags) {
		ri.opCode = *px86;
		ri.specifiers = GetSpecifiers(ri);
		ri.opTypes[1] = RIVER_OPTYPE_IMM | RIVER_OPSIZE_32;
		ri.operands[1].asImm32 = (rev::DWORD)px86 + instrLen;
		flags |= RIVER_FLAG_BRANCH;
		px86++;
	}

	void DisassembleRelJmpModRMInstr(rev::BYTE *&px86, RiverInstruction &ri, rev::DWORD &flags) {
		rev::BYTE extra;
		ri.opCode = *px86;
		ri.specifiers = GetSpecifiers(ri);

		px86++;
		DisassembleModRMOp(0, px86, ri, extra);

		ri.opTypes[1] = RIVER_OPTYPE_IMM | RIVER_OPSIZE_32;
		ri.operands[1].asImm32 = (rev::DWORD)px86;
		flags |= RIVER_FLAG_BRANCH;
	}

	/*void DisassembleExtInstr(BYTE *&px86, RiverInstruction &ri, DWORD &flags);*/

	template <DisassembleOpcodeFunc *fSubOps> void DisassembleSubOpInstr(rev::BYTE *&px86, RiverInstruction &ri, rev::DWORD &flags) {
		ri.subOpCode = (*(px86 + 1) >> 3) & 0x7;
		(this->*fSubOps[ri.subOpCode])(px86, ri, flags);
	}

	/* operand helpers */
	void DisassembleImmOp(rev::BYTE opIdx, rev::BYTE *&px86, RiverInstruction &ri, rev::BYTE immSize);
	void DisassembleRegOp(rev::BYTE opIdx, RiverInstruction &ri, rev::BYTE reg);
	void DisassembleModRMOp(rev::BYTE opIdx, rev::BYTE *&px86, RiverInstruction &ri, rev::BYTE &extra);
	void DisassembleSzModRMOp(rev::BYTE opIdx, rev::BYTE *&px86, RiverInstruction &ri, rev::BYTE &extra, rev::WORD sz);
	void DisassembleMoffs8(rev::BYTE opIdx, rev::BYTE *&px86, RiverInstruction &ri);
	void DisassembleMoffs32(rev::BYTE opIdx, rev::BYTE *&px86, RiverInstruction &ri);

	/* extra disassemblers */
	void DropExtra(rev::BYTE extra, RiverInstruction &ri) {}
	void SubOpExtra(rev::BYTE extra, RiverInstruction &ri) {
		ri.subOpCode = extra;
	}
	template <rev::BYTE opIdx> void RegExtra(rev::BYTE extra, RiverInstruction &ri) {
		DisassembleRegOp(opIdx, ri, extra);
	}

	/* operand disassemblers */
	void DisassembleUnkOp(rev::BYTE *&px86, RiverInstruction &ri);
	void DisassembleNoOp(rev::BYTE *&px86, RiverInstruction &ri);
	void DisassembleRegModRM(rev::BYTE *&px86, RiverInstruction &ri);
	void DisassembleModRMReg(rev::BYTE *&px86, RiverInstruction &ri);
	void DisassembleModRMImm8(rev::BYTE *&px86, RiverInstruction &ri);
	void DisassembleModRMImm32(rev::BYTE *&px86, RiverInstruction &ri);
	void DisassembleSubOpModRM(rev::BYTE *&px86, RiverInstruction &ri);
	void DisassembleSubOpModRMImm8(rev::BYTE *&px86, RiverInstruction &ri);
	void DisassembleSubOpModRMImm32(rev::BYTE *&px86, RiverInstruction &ri);
	void DisassembleImm8(rev::BYTE *&px86, RiverInstruction &ri);
	void DisassembleImm16(rev::BYTE *&px86, RiverInstruction &ri);
	void DisassembleImm32(rev::BYTE *&px86, RiverInstruction &ri);

	void DisassembleImm32Imm16(rev::BYTE *&px86, RiverInstruction &ri);

	void DisassembleRegModRMImm32(rev::BYTE *&px86, RiverInstruction &ri);
	void DisassembleRegModRMImm8(rev::BYTE *&px86, RiverInstruction &ri);
	void DisassembleModRMRegImm8(rev::BYTE *&px86, RiverInstruction &ri);
	
	template <rev::BYTE opIdx> void DisassembleMoffs8(rev::BYTE *&px86, RiverInstruction &ri) {
		DisassembleMoffs32(opIdx, px86, ri);
	}

	template <rev::BYTE opIdx> void DisassembleMoffs32(rev::BYTE *&px86, RiverInstruction &ri) {
		DisassembleMoffs32(opIdx, px86, ri);
	}

	template <rev::BYTE opIdx, rev::BYTE immSize> void DisassembleImmOp(rev::BYTE *&px86, RiverInstruction &ri) {
		DisassembleImmOp(opIdx, px86, ri, immSize);
	}

	template <rev::BYTE opIdx> void DisassembleModRM(rev::BYTE *&px86, RiverInstruction &ri) {
		rev::BYTE extra;
		DisassembleModRMOp(opIdx, px86, ri, extra);
	}

	template <DisassembleOperandsFunc *fSubOps> void DisassembleSubOpOp(rev::BYTE *&px86, RiverInstruction &ri) {
		(this->*fSubOps[ri.subOpCode])(px86, ri);
	}

	template <rev::WORD secondOpSz> void DisassembleRegSzModRM(rev::BYTE *&px86, RiverInstruction &ri) {
		rev::BYTE sec;
		DisassembleSzModRMOp(1, px86, ri, sec, secondOpSz);
		DisassembleRegOp(0, ri, sec);
	}

	template <rev::BYTE opIdx, rev::BYTE opFlg, rev::BYTE regName, DisassembleOperandsFunc cont> void DisassembleConstRegOperand(rev::BYTE *&px86, RiverInstruction &ri) {
		(this->*cont)(px86, ri);

		//ri.opTypes[opIdx] = RIVER_OPTYPE_REG | opFlg;
		//ri.operands[opIdx].asRegister.versioned = codegen->GetCurrentReg(regName);

		DisassembleRegOp(opIdx, ri, regName);
		ri.opTypes[opIdx] |= opFlg;
	}

	template <rev::BYTE opIdx, rev::BYTE opFlg, rev::BYTE base, rev::BYTE disp8, DisassembleOperandsFunc cont> void DisassembleConstMemOperand(rev::BYTE *&px86, RiverInstruction &ri);
};

#endif
