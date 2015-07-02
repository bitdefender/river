#ifndef _RIVER_X86_ASSEMBLER_H
#define _RIVER_X86_ASSEMBLER_H

#include "extern.h"
#include "river.h"
#include "Runtime.h"


class RiverX86Assembler {
private :
	RiverRuntime *runtime;

	bool needsRAFix;
	BYTE *rvAddress;

	void SwitchToRiver(BYTE *&px86, DWORD &instrCounter);
	void SwitchToRiverEsp(BYTE *&px86, DWORD &instrCounter, BYTE repReg);
	void EndRiverConversion(BYTE *&px86, DWORD &pFlags, BYTE &repReg, DWORD &instrCounter);

	typedef void(RiverX86Assembler::*AssembleOpcodeFunc)(const RiverInstruction &ri, BYTE *&px86, DWORD &pFlags, DWORD &instrCounter);
	typedef void(RiverX86Assembler::*AssembleOperandsFunc)(const RiverInstruction &ri, BYTE *&px86);

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

	bool GenerateTransitions(const RiverInstruction &ri, BYTE *&px86, DWORD &pFlags, BYTE &repReg, DWORD &instrCounter);
	bool GeneratePrefixes(const RiverInstruction &ri, BYTE *&px86);
	bool ClearPrefixes(const RiverInstruction &ri, BYTE *&px86);
	bool Translate(const RiverInstruction &ri, BYTE *&px86, DWORD &pFlags, BYTE &repReg, DWORD &instrCounter);
public :
	bool Init(RiverRuntime *rt);
	bool Assemble(RiverInstruction *pRiver, DWORD dwInstrCount, BYTE *px86, DWORD flg, DWORD &instrCounter, DWORD &byteCounter);

	void CopyFix(BYTE *dst, BYTE *src) {
		if (needsRAFix) {
			DWORD offset = (rvAddress - src);
			DWORD adjust = src - dst;

			*(DWORD *)(&src[offset]) += adjust;
		}
	}
private :
	/* opcodes assemblers */
	void AssembleUnkInstr(const RiverInstruction &ri, BYTE *&px86, DWORD &pFlags, DWORD &instrCounter);
	void AssembleDefaultInstr(const RiverInstruction &ri, BYTE *&px86, DWORD &pFlags, DWORD &instrCounter);
	void AssemblePlusRegInstr(const RiverInstruction &ri, BYTE *&px86, DWORD &pFlags, DWORD &instrCounter);

	void AssembleRelJMPInstr(const RiverInstruction &ri, BYTE *&px86, DWORD &pFlags, DWORD &instrCounter);
	void AssembleJMPInstr(const RiverInstruction &ri, BYTE *&px86, DWORD &pFlags, DWORD &instrCounter);
	void AssembleRelJmpCondInstr(const RiverInstruction &ri, BYTE *&px86, DWORD &pFlags, DWORD &instrCounter);
	void AssembleCallInstr(const RiverInstruction &ri, BYTE *&px86, DWORD &pFlags, DWORD &instrCounter);
	void AssembleFFCallInstr(const RiverInstruction &ri, BYTE *&px86, DWORD &pFlags, DWORD &instrCounter);
	void AssembleFFJumpInstr(const RiverInstruction &ri, BYTE *&px86, DWORD &pFlags, DWORD &instrCounter);
	void AssembleRetnInstr(const RiverInstruction &ri, BYTE *&px86, DWORD &pFlags, DWORD &instrCounter);
	void AssembleRetnImmInstr(const RiverInstruction &ri, BYTE *&px86, DWORD &pFlags, DWORD &instrCounter);
	void AssembleSyscall(const RiverInstruction &ri, BYTE *&px86, DWORD &pFlags, DWORD &instrCounter);
	void AssembleSyscall2(const RiverInstruction &ri, BYTE *&px86, DWORD &pFlags, DWORD &instrCounter);

	void AssembleLeaveForSyscall(
		const RiverInstruction &ri,
		BYTE *&px86,
		DWORD &pFlags,
		DWORD &instrCounter,
		RiverX86Assembler::AssembleOpcodeFunc opcodeFunc,
		RiverX86Assembler::AssembleOperandsFunc operandsFunc
	);

	template <AssembleOpcodeFunc fRiver, AssembleOpcodeFunc fNormal> void AssembleRiverInstr(const RiverInstruction &ri, BYTE *&px86, DWORD &pFlags, DWORD &instrCounter) {
		if (ri.family & RIVER_FAMILY_RIVEROP) {
			(this->*fRiver)(ri, px86, pFlags, instrCounter);
		}
		else {
			(this->*fNormal)(ri, px86, pFlags, instrCounter);
		}
	}

	template <AssembleOpcodeFunc *fSubOps> void AssembleSubOpInstr(const RiverInstruction &ri, BYTE *&px86, DWORD &pFlags, DWORD &instrCounter) {
		(this->*fSubOps[ri.subOpCode])(ri, px86, pFlags, instrCounter);
	}

	void AssembleRiverAddSubInstr(const RiverInstruction &ri, BYTE *&px86, DWORD &pFlags, DWORD &instrCounter);

	/* operand helpers */
	void AssembleModRMOp(unsigned int opIdx, const RiverInstruction &ri, BYTE *&px86, BYTE extra);
	void AssembleImmOp(unsigned int opIdx, const RiverInstruction &ri, BYTE *&px86, BYTE immSize);

	void AssembleMoffs(unsigned int opIdx, const RiverInstruction &ri, BYTE *&px86, BYTE immSize);

	/* operand assemblers */
	void AssembleUnknownOp(const RiverInstruction &ri, BYTE *&px86);
	void AssembleNoOp(const RiverInstruction &ri, BYTE *&px86);

	template <BYTE opIdx, BYTE immSize> void AssembleImmOp(const RiverInstruction &ri, BYTE *&px86) {
		AssembleImmOp(opIdx, ri, px86, immSize);
	}

	template <int extra> void AssembleModRMOp(const RiverInstruction &ri, BYTE *&px86) {
		AssembleModRMOp(0, ri, px86, extra);
	}

	void AssembleModRMImm8Op(const RiverInstruction &ri, BYTE *&px86);
	void AssembleModRMImm32Op(const RiverInstruction &ri, BYTE *&px86);

	void AssembleRegModRMOp(const RiverInstruction &ri, BYTE *&px86);
	void AssembleModRMRegOp(const RiverInstruction &ri, BYTE *&px86);
	void AssembleSubOpModRMOp(const RiverInstruction &ri, BYTE *&px86);
	void AssembleSubOpModRMImm8Op(const RiverInstruction &ri, BYTE *&px86);
	void AssembleSubOpModRMImm32Op(const RiverInstruction &ri, BYTE *&px86);

	void AssembleRegModRMImm32Op(const RiverInstruction &ri, BYTE *&px86);
	void AssembleRegModRMImm8Op(const RiverInstruction &ri, BYTE *&px86);

	void AssembleModRMRegImm8Op(const RiverInstruction &ri, BYTE *&px86);

	template <AssembleOperandsFunc fRiver, AssembleOperandsFunc fNormal> void AssembleRiverOp(const RiverInstruction &ri, BYTE *&px86) {
		if (ri.family & RIVER_FAMILY_RIVEROP) {
			(this->*fRiver)(ri, px86);
		} else {
			(this->*fNormal)(ri, px86);
		}
	}

	template <AssembleOperandsFunc *fSubOps> void AssembleSubOpOp(const RiverInstruction &ri, BYTE *&px86) {
		(this->*fSubOps[ri.subOpCode])(ri, px86);
	}

	void AssembleRiverAddSubOp(const RiverInstruction &ri, BYTE *&px86);

	template <BYTE opIdx> void AssembleMoffs8(const RiverInstruction &ri, BYTE *&px86) {
		AssembleMoffs(opIdx, ri, px86, RIVER_OPSIZE_32);
	}

	template <BYTE opIdx> void AssembleMoffs32(const RiverInstruction &ri, BYTE *&px86) {
		AssembleMoffs(opIdx, ri, px86, RIVER_OPSIZE_32);
	}
};


#endif
