#ifndef _NATIVE_X86_ASSEMBLER_H
#define _NATIVE_X86_ASSEMBLER_H

#include "GenericX86Assembler.h"
#include "X86AssemblerFuncs.h"

class NativeX86Assembler : public GenericX86Assembler {
private :
	typedef void(NativeX86Assembler::*AssembleOpcodeFunc)(const RiverInstruction &ri, RelocableCodeBuffer &px86, nodep::DWORD &pFlags, nodep::DWORD &instrCounter);
	typedef void(NativeX86Assembler::*AssembleOperandsFunc)(const RiverInstruction &ri, RelocableCodeBuffer &px86);

	static AssembleOpcodeFunc assemble0xF6Instr[8];
	static AssembleOperandsFunc assemble0xF6Op[8];

	static AssembleOpcodeFunc assemble0xF7Instr[8];
	static AssembleOperandsFunc assemble0xF7Op[8];

	static AssembleOpcodeFunc assemble0xFFInstr[8];
	static AssembleOperandsFunc assemble0xFFOp[8];

	static AssembleOpcodeFunc assemble0x0FC7Instr[8];
	static AssembleOperandsFunc assemble0x0FC7Op[8];

	static AssembleOpcodeFunc assembleOpcodes[2][0x100];
	static AssembleOperandsFunc assembleOperands[2][0x100];

//public :
	//bool Assemble(RiverInstruction *pRiver, nodep::DWORD dwInstrCount, nodep::BYTE *px86, nodep::DWORD flg, nodep::DWORD &instrCounter, nodep::DWORD &byteCounter);
public :
	virtual bool Translate(const RiverInstruction &ri, RelocableCodeBuffer &px86, nodep::DWORD &pFlags, nodep::BYTE &currentFamily, nodep::BYTE &repReg, nodep::DWORD &instrCounter, nodep::BYTE outputType);
private :
	/* opcodes assemblers */
	void AssembleUnkInstr(const RiverInstruction &ri, RelocableCodeBuffer &px86, nodep::DWORD &pFlags, nodep::DWORD &instrCounter);
	void AssembleDefaultInstr(const RiverInstruction &ri, RelocableCodeBuffer &px86, nodep::DWORD &pFlags, nodep::DWORD &instrCounter);
	void AssemblePlusRegInstr(const RiverInstruction &ri, RelocableCodeBuffer &px86, nodep::DWORD &pFlags, nodep::DWORD &instrCounter);

	void AssembleRelJMPInstr(const RiverInstruction &ri, RelocableCodeBuffer &px86, nodep::DWORD &pFlags, nodep::DWORD &instrCounter);
	void AssembleJMPInstr(const RiverInstruction &ri, RelocableCodeBuffer &px86, nodep::DWORD &pFlags, nodep::DWORD &instrCounter);
	void AssembleRelJmpCondInstr(const RiverInstruction &ri, RelocableCodeBuffer &px86, nodep::DWORD &pFlags, nodep::DWORD &instrCounter);
	void AssembleCallInstr(const RiverInstruction &ri, RelocableCodeBuffer &px86, nodep::DWORD &pFlags, nodep::DWORD &instrCounter);
	void AssembleFFCallInstr(const RiverInstruction &ri, RelocableCodeBuffer &px86, nodep::DWORD &pFlags, nodep::DWORD &instrCounter);
	void AssembleFFJumpInstr(const RiverInstruction &ri, RelocableCodeBuffer &px86, nodep::DWORD &pFlags, nodep::DWORD &instrCounter);
	void AssembleRetnInstr(const RiverInstruction &ri, RelocableCodeBuffer &px86, nodep::DWORD &pFlags, nodep::DWORD &instrCounter);
	void AssembleRetnImmInstr(const RiverInstruction &ri, RelocableCodeBuffer &px86, nodep::DWORD &pFlags, nodep::DWORD &instrCounter);

	void AssembleFarJumpInstr(const RiverInstruction &ri, RelocableCodeBuffer &px86, nodep::DWORD &pFlags, nodep::DWORD &instrCounter);
	void AssembleFarJump2(const RiverInstruction &ri, RelocableCodeBuffer &px86, nodep::DWORD &pFlags, nodep::DWORD &instrCounter);
	
	template <nodep::BYTE opcode>
	void AssembleSyscall(const RiverInstruction &ri, RelocableCodeBuffer &px86, nodep::DWORD &pFlags, nodep::DWORD &instrCounter);
	template <nodep::BYTE opcode>
	void AssembleSyscall2(const RiverInstruction &ri, RelocableCodeBuffer &px86, nodep::DWORD &pFlags, nodep::DWORD &instrCounter);

	void AssembleLeaveForSyscall(
		const RiverInstruction &ri,
		RelocableCodeBuffer &px86,
		nodep::DWORD &pFlags,
		nodep::DWORD &instrCounter,
		NativeX86Assembler::AssembleOpcodeFunc opcodeFunc,
		NativeX86Assembler::AssembleOperandsFunc operandsFunc
	);

	/*template <AssembleOpcodeFunc fRiver, AssembleOpcodeFunc fNormal> void AssembleRiverInstr(const RiverInstruction &ri, RelocableCodeBuffer &px86, nodep::DWORD &pFlags, nodep::DWORD &instrCounter) {
		if (ri.family & RIVER_FAMILY_RIVEROP) {
			(this->*fRiver)(ri, px86, pFlags, instrCounter);
		}
		else {
			(this->*fNormal)(ri, px86, pFlags, instrCounter);
		}
	}*/

	template <AssembleOpcodeFunc *fSubOps> void AssembleSubOpInstr(const RiverInstruction &ri, RelocableCodeBuffer &px86, nodep::DWORD &pFlags, nodep::DWORD &instrCounter) {
		(this->*fSubOps[ri.subOpCode])(ri, px86, pFlags, instrCounter);
	}

	void AssembleRiverAddSubInstr(const RiverInstruction &ri, RelocableCodeBuffer &px86, nodep::DWORD &pFlags, nodep::DWORD &instrCounter);

	/* operand helpers */
	/*void AssembleModRMOp(unsigned int opIdx, const RiverInstruction &ri, RelocableCodeBuffer &px86, nodep::BYTE extra);
	void AssembleImmOp(unsigned int opIdx, const RiverInstruction &ri, RelocableCodeBuffer &px86, nodep::BYTE immSize);

	void AssembleMoffs(unsigned int opIdx, const RiverInstruction &ri, RelocableCodeBuffer &px86, nodep::BYTE immSize);*/

	/* Operand assembler functions */
	void AssembleUnknownOp(const RiverInstruction &ri, RelocableCodeBuffer &px86);
	void AssembleNoOp(const RiverInstruction &ri, RelocableCodeBuffer &px86);

	template <nodep::BYTE opIdx, nodep::BYTE immSize> void AssembleImmOp(const RiverInstruction &ri, RelocableCodeBuffer &px86) {
		::AssembleImmOp(opIdx, ri, px86, immSize);
	}

	template <int extra> void AssembleModRMOp(const RiverInstruction &ri, RelocableCodeBuffer &px86) {
		::AssembleModRMOp(0, ri, px86, extra);
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

	template <AssembleOperandsFunc *fSubOps> void AssembleSubOpOp(const RiverInstruction &ri, RelocableCodeBuffer &px86) {
		(this->*fSubOps[ri.subOpCode])(ri, px86);
	}

	template <nodep::BYTE opIdx> void AssembleMoffs8(const RiverInstruction &ri, RelocableCodeBuffer &px86) {
		::AssembleMoffs(opIdx, ri, px86, RIVER_OPSIZE_32);
	}

	template <nodep::BYTE opIdx> void AssembleMoffs32(const RiverInstruction &ri, RelocableCodeBuffer &px86) {
		::AssembleMoffs(opIdx, ri, px86, RIVER_OPSIZE_32);
	}
};

#endif
