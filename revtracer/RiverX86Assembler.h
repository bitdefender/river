#ifndef _RIVER_X86_ASSEMBLER_H
#define _RIVER_X86_ASSEMBLER_H

#include "extern.h"
#include "river.h"
#include "Runtime.h"


class RiverX86Assembler {
private :
	RiverRuntime *runtime;

	void SwitchToRiver(BYTE *&px86, DWORD &instrCounter);
	void SwitchToRiverEsp(BYTE *&px86, DWORD &instrCounter);
	void EndRiverConversion(BYTE *&px86, DWORD &pFlags, DWORD &instrCounter);

	typedef void(RiverX86Assembler::*AssembleOpcodeFunc)(const RiverInstruction &ri, BYTE *&px86, DWORD &pFlags, DWORD &instrCounter);
	typedef void(RiverX86Assembler::*AssembleOperandsFunc)(const RiverInstruction &ri, BYTE *&px86);

	static AssembleOpcodeFunc assemble0xFFInstr[8];
	static AssembleOperandsFunc assemble0xFFOp[8];

	static AssembleOpcodeFunc assembleOpcodes[2][0x100];
	static AssembleOperandsFunc assembleOperands[2][0x100];
public :
	bool Init(RiverRuntime *rt);
	bool Translate(const RiverInstruction &ri, BYTE *&px86, DWORD &pFlags, DWORD &instrCounter);

	bool Assemble(RiverInstruction *pRiver, DWORD dwInstrCount, BYTE *px86, DWORD flg, DWORD &instrCounter, DWORD &byteCounter);

private :
	/* opcodes assemblers */
	void AssembleUnkInstr(const RiverInstruction &ri, BYTE *&px86, DWORD &pFlags, DWORD &instrCounter);
	void AssembleDefaultInstr(const RiverInstruction &ri, BYTE *&px86, DWORD &pFlags, DWORD &instrCounter);
	void AssemblePlusRegInstr(const RiverInstruction &ri, BYTE *&px86, DWORD &pFlags, DWORD &instrCounter);

	void AssembleRelJMPInstr(const RiverInstruction &ri, BYTE *&px86, DWORD &pFlags, DWORD &instrCounter);
	void AssembleRelJmpCondInstr(const RiverInstruction &ri, BYTE *&px86, DWORD &pFlags, DWORD &instrCounter);
	void AssembleRetnInstr(const RiverInstruction &ri, BYTE *&px86, DWORD &pFlags, DWORD &instrCounter);

	template <AssembleOpcodeFunc fRiver, AssembleOpcodeFunc fNormal> void AssembleRiverInstr(const RiverInstruction &ri, BYTE *&px86, DWORD &pFlags, DWORD &instrCounter) {
		if (ri.modifiers & RIVER_MODIFIER_RIVEROP) {
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

	/* operand assemblers */
	void AssembleUnknownOp(const RiverInstruction &ri, BYTE *&px86);
	void AssembleNoOp(const RiverInstruction &ri, BYTE *&px86);

	template <int extra> void AssembleModRMOp(const RiverInstruction &ri, BYTE *&px86) {
		AssembleModRMOp(0, ri, px86, extra);
	}

	void AssembleRegModRMOp(const RiverInstruction &ri, BYTE *&px86);
	void AssembleModRMRegOp(const RiverInstruction &ri, BYTE *&px86);
	void AssembleSubOpModRMImm8Op(const RiverInstruction &ri, BYTE *&px86);

	template <AssembleOperandsFunc fRiver, AssembleOperandsFunc fNormal> void AssembleRiverOp(const RiverInstruction &ri, BYTE *&px86) {
		if (ri.modifiers & RIVER_MODIFIER_RIVEROP) {
			(this->*fRiver)(ri, px86);
		}
		else {
			(this->*fNormal)(ri, px86);
		}
	}

	template <AssembleOperandsFunc *fSubOps> void AssembleSubOpOp(const RiverInstruction &ri, BYTE *&px86) {
		(this->*fSubOps[ri.subOpCode])(ri, px86);
	}

	void AssembleRiverAddSubOp(const RiverInstruction &ri, BYTE *&px86);
};


#endif
