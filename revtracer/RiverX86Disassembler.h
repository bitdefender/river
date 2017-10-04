#ifndef _RIVER_X86_DISASSEMBLER_H
#define _RIVER_X86_DISASSEMBLER_H

#include "revtracer.h"
#include "river.h"

class RiverCodeGen;

nodep::WORD GetSpecifiers(RiverInstruction &ri);

class RiverX86Disassembler {
private:
	RiverCodeGen *codegen;

	void TrackModifiedRegisters(RiverInstruction &ri);
	/*void TrackUnusedRegisters(RiverInstruction &ri);
	void TrackUnusedRegistersOperand(RiverInstruction &ri, BYTE optype, const RiverOperand &op);*/

	typedef void(RiverX86Disassembler::*DisassembleOpcodeFunc)(nodep::BYTE *&px86, RiverInstruction &ri, nodep::DWORD &flags);
	typedef void(RiverX86Disassembler::*DisassembleExtraFunc)(nodep::BYTE extra, RiverInstruction &ri);
	typedef void(RiverX86Disassembler::*DisassembleOperandsFunc)(nodep::BYTE *&px86, RiverInstruction &ri);

	static DisassembleOpcodeFunc disassembleOpcodes[2][0x100];
	static DisassembleOperandsFunc disassembleOperands[2][0x100];

	static nodep::BYTE testFlags[2][0x100];
	static nodep::BYTE modFlags[2][0x100];
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

	bool Translate(nodep::BYTE *&px86, RiverInstruction &rOut, nodep::DWORD &flags);
private :
	/* opcodes disassemblers */
	void DisassembleUnkInstr(nodep::BYTE *&px86, RiverInstruction &ri, nodep::DWORD &flags);

	template <nodep::WORD modifiers = 0, nodep::WORD tflags = 0> void DisassembleDefaultInstr(nodep::BYTE *&px86, RiverInstruction &ri, nodep::DWORD &flags) {
		ri.opCode = *px86;
		ri.modifiers |= modifiers;
		ri.specifiers = GetSpecifiers(ri);
		flags |= tflags;
		px86++;
	}

	template <nodep::WORD modifiers = 0, nodep::WORD tflags = 0> void DisassembleExtInstr(nodep::BYTE *&px86, RiverInstruction &ri, nodep::DWORD &flags) {
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

	template <nodep::WORD segment> void DisassembleSegInstr(nodep::BYTE *&px86, RiverInstruction &ri, nodep::DWORD &flags) {
		flags |= RIVER_FLAG_PFX;
		ri.modifiers &= 0xF8;
		ri.modifiers |= segment;

		px86++;
	}

	template <nodep::BYTE base, nodep::WORD modifiers = 0> void DisassemblePlusRegInstr(nodep::BYTE *&px86, RiverInstruction &ri, nodep::DWORD &flags) {
		ri.opCode = base;
		ri.modifiers |= modifiers;
		ri.specifiers = GetSpecifiers(ri);
		DisassembleRegOp(0, ri, *px86 - base);
		px86++;
	}

	template <nodep::BYTE regName, nodep::WORD modifiers = 0> void DisassembleDefaultRegInstr(nodep::BYTE *&px86, RiverInstruction &ri, nodep::DWORD &flags) {
		ri.opCode = *px86;
		ri.modifiers |= modifiers;
		ri.specifiers = GetSpecifiers(ri);
		DisassembleRegOp(0, ri, regName);
		px86++;
	}

	template <nodep::BYTE regName, nodep::WORD modifiers = 0> void DisassembleDefaultSecondRegInstr(nodep::BYTE *&px86, RiverInstruction &ri, nodep::DWORD &flags) {
		ri.opCode = *px86;
		ri.modifiers |= modifiers;
		ri.specifiers = GetSpecifiers(ri);
		DisassembleRegOp(1, ri, regName);
		px86++;
	}

	template <nodep::BYTE regName0, nodep::BYTE regName1, nodep::WORD modifiers = 0> void DisassembleDefault2RegInstr(nodep::BYTE *&px86, RiverInstruction &ri, nodep::DWORD &flags) {
		ri.opCode = *px86;
		ri.modifiers |= modifiers;
		ri.specifiers = GetSpecifiers(ri);
		DisassembleRegOp(0, ri, regName0);
		DisassembleRegOp(1, ri, regName1);
		px86++;
	}

	template <nodep::BYTE instrLen> void DisassembleRelJmpInstr(nodep::BYTE *&px86, RiverInstruction &ri, nodep::DWORD &flags) {
		ri.opCode = *px86;
		ri.specifiers = GetSpecifiers(ri);
		ri.opTypes[1] = RIVER_OPTYPE_IMM | RIVER_OPSIZE_32;
		ri.operands[1].asImm32 = (nodep::DWORD)px86 + instrLen;
		flags |= RIVER_FLAG_BRANCH;
		flags |= RIVER_BRANCH_TYPE_IMM;

		switch(ri.opCode) {
		case 0x70: case 0x71: case 0x72: case 0x73:
		case 0x74: case 0x75: case 0x76: case 0x77:
		case 0x78: case 0x79: case 0x7A: case 0x7B:
		case 0x7C: case 0x7D: case 0x7E: case 0x7F:
		case 0xE3: /*jcxz*/
		case 0x80: case 0x81: case 0x82: case 0x83:
		case 0x84: case 0x85: case 0x86: case 0x87:
		case 0x88: case 0x89: case 0x8A: case 0x8B:
		case 0x8C: case 0x8D: case 0x8E: case 0x8F:
				flags |= RIVER_BRANCH_INSTR_JXX;
				break;
		case 0xE9: case 0xEB:
				flags |= RIVER_BRANCH_INSTR_JMP;
				break;
		case 0xE8:
				flags |= RIVER_BRANCH_INSTR_CALL;
				break;
		case 0x34:
				flags |= RIVER_BRANCH_INSTR_SYSCALL;
				break;
		case 0x05:
				break;
		default:
				DEBUG_BREAK;

		}
		px86++;
	}

	void DisassembleRelJmpModRMInstr(nodep::BYTE *&px86, RiverInstruction &ri, nodep::DWORD &flags) {
		nodep::BYTE extra;
		ri.opCode = *px86;
		ri.specifiers = GetSpecifiers(ri);

		px86++;
		DisassembleModRMOp(0, px86, ri, extra);
		switch(RIVER_OPTYPE(ri.opTypes[0])) {
		case RIVER_OPTYPE_REG:
			flags |= RIVER_BRANCH_TYPE_REG;
			break;
		case RIVER_OPTYPE_MEM:
			flags |= RIVER_BRANCH_TYPE_MEM;
			break;
		default:
			DEBUG_BREAK;
		}

		ri.opTypes[1] = RIVER_OPTYPE_IMM | RIVER_OPSIZE_32;
		ri.operands[1].asImm32 = (nodep::DWORD)px86;
		flags |= RIVER_FLAG_BRANCH;

		switch(ri.subOpCode) {
		case 0x02:
			flags |= RIVER_BRANCH_INSTR_CALL;
			break;
		case 0x04:
			flags |= RIVER_BRANCH_INSTR_JMP;
			break;
		default:
			DEBUG_BREAK;
		}
	}

	/*void DisassembleExtInstr(BYTE *&px86, RiverInstruction &ri, DWORD &flags);*/

	template <DisassembleOpcodeFunc *fSubOps> void DisassembleSubOpInstr(nodep::BYTE *&px86, RiverInstruction &ri, nodep::DWORD &flags) {
		ri.subOpCode = (*(px86 + 1) >> 3) & 0x7;
		(this->*fSubOps[ri.subOpCode])(px86, ri, flags);
	}

	/* operand helpers */
	void DisassembleImmOp(nodep::BYTE opIdx, nodep::BYTE *&px86, RiverInstruction &ri, nodep::BYTE immSize);
	void DisassembleRegOp(nodep::BYTE opIdx, RiverInstruction &ri, nodep::BYTE reg);
	void DisassembleModRMOp(nodep::BYTE opIdx, nodep::BYTE *&px86, RiverInstruction &ri, nodep::BYTE &extra);
	void DisassembleSzModRMOp(nodep::BYTE opIdx, nodep::BYTE *&px86, RiverInstruction &ri, nodep::BYTE &extra, nodep::WORD sz);
	void DisassembleMoffs8(nodep::BYTE opIdx, nodep::BYTE *&px86, RiverInstruction &ri);
	void DisassembleMoffs32(nodep::BYTE opIdx, nodep::BYTE *&px86, RiverInstruction &ri);

	/* extra disassemblers */
	void DropExtra(nodep::BYTE extra, RiverInstruction &ri) {}
	void SubOpExtra(nodep::BYTE extra, RiverInstruction &ri) {
		ri.subOpCode = extra;
	}
	template <nodep::BYTE opIdx> void RegExtra(nodep::BYTE extra, RiverInstruction &ri) {
		DisassembleRegOp(opIdx, ri, extra);
	}

	/* operand disassemblers */
	void DisassembleUnkOp(nodep::BYTE *&px86, RiverInstruction &ri);
	void DisassembleNoOp(nodep::BYTE *&px86, RiverInstruction &ri);
	void DisassembleRegModRM(nodep::BYTE *&px86, RiverInstruction &ri);
	void DisassembleModRMReg(nodep::BYTE *&px86, RiverInstruction &ri);
	void DisassembleModRMImm8(nodep::BYTE *&px86, RiverInstruction &ri);
	void DisassembleModRMImm32(nodep::BYTE *&px86, RiverInstruction &ri);
	void DisassembleSubOpModRM(nodep::BYTE *&px86, RiverInstruction &ri);
	void DisassembleSubOpModRMImm8(nodep::BYTE *&px86, RiverInstruction &ri);
	void DisassembleSubOpModRMImm32(nodep::BYTE *&px86, RiverInstruction &ri);
	void DisassembleImm8(nodep::BYTE *&px86, RiverInstruction &ri);
	void DisassembleImm16(nodep::BYTE *&px86, RiverInstruction &ri);
	void DisassembleImm32(nodep::BYTE *&px86, RiverInstruction &ri);

	void DisassembleImm32Imm16(nodep::BYTE *&px86, RiverInstruction &ri);

	void DisassembleRegModRMImm32(nodep::BYTE *&px86, RiverInstruction &ri);
	void DisassembleRegModRMImm8(nodep::BYTE *&px86, RiverInstruction &ri);
	void DisassembleModRMRegImm8(nodep::BYTE *&px86, RiverInstruction &ri);
	
	template <nodep::BYTE opIdx> void DisassembleMoffs8(nodep::BYTE *&px86, RiverInstruction &ri) {
		DisassembleMoffs32(opIdx, px86, ri);
	}

	template <nodep::BYTE opIdx> void DisassembleMoffs32(nodep::BYTE *&px86, RiverInstruction &ri) {
		DisassembleMoffs32(opIdx, px86, ri);
	}

	template <nodep::BYTE opIdx, nodep::BYTE immSize> void DisassembleImmOp(nodep::BYTE *&px86, RiverInstruction &ri) {
		DisassembleImmOp(opIdx, px86, ri, immSize);
	}

	template <nodep::BYTE opIdx> void DisassembleModRM(nodep::BYTE *&px86, RiverInstruction &ri) {
		nodep::BYTE extra;
		DisassembleModRMOp(opIdx, px86, ri, extra);
	}

	template <DisassembleOperandsFunc *fSubOps> void DisassembleSubOpOp(nodep::BYTE *&px86, RiverInstruction &ri) {
		(this->*fSubOps[ri.subOpCode])(px86, ri);
	}

	template <nodep::WORD secondOpSz> void DisassembleRegSzModRM(nodep::BYTE *&px86, RiverInstruction &ri) {
		nodep::BYTE sec;
		DisassembleSzModRMOp(1, px86, ri, sec, secondOpSz);
		DisassembleRegOp(0, ri, sec);
	}

	template <nodep::BYTE opIdx, nodep::BYTE opFlg, nodep::BYTE regName, DisassembleOperandsFunc cont> void DisassembleConstRegOperand(nodep::BYTE *&px86, RiverInstruction &ri) {
		(this->*cont)(px86, ri);

		//ri.opTypes[opIdx] = RIVER_OPTYPE_REG | opFlg;
		//ri.operands[opIdx].asRegister.versioned = codegen->GetCurrentReg(regName);

		DisassembleRegOp(opIdx, ri, regName);
		ri.opTypes[opIdx] |= opFlg;
	}

	template <nodep::BYTE opIdx, nodep::BYTE opFlg, nodep::BYTE base, nodep::BYTE disp8, DisassembleOperandsFunc cont> void DisassembleConstMemOperand(nodep::BYTE *&px86, RiverInstruction &ri);
};

#endif
