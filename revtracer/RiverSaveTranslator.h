#ifndef _RIVER_SAVE_TRANSLATOR_H
#define _RIVER_SAVE_TRANSLATOR_H

#include "revtracer.h"
#include "river.h"

class RiverCodeGen;

class RiverSaveTranslator {
private :
	RiverCodeGen *codegen;

	typedef void(RiverSaveTranslator::*TranslateOpcodeFunc)(RiverInstruction *rOut, const RiverInstruction &rIn, nodep::DWORD &instrCount);
	
	static TranslateOpcodeFunc translateOpcodes[2][0x100];
	static TranslateOpcodeFunc translate0xF7[8], translate0xFF[8], translate0x0FC7[8];

public :
	bool Init(RiverCodeGen *cg);
	bool Translate(const RiverInstruction &rIn, RiverInstruction *rOut, nodep::DWORD &instrCount);

private :
	/* Translation helpers */
	void MakeSaveFlags(RiverInstruction *rOut);
	void MakeSaveReg(RiverInstruction *rOut, const RiverRegister &reg, unsigned short familyFlag);
	void MakeSaveMem(RiverInstruction *rOut, const RiverAddress &mem, unsigned short auxFlags, const RiverInstruction &rIn);
	void MakeSaveMemOffset(RiverInstruction *rOut, const RiverAddress &mem, int offset, unsigned short auxFlags, const RiverInstruction &rIn);
	void MakeAddNoFlagsRegImm8(RiverInstruction *rOut, const RiverRegister &reg, unsigned char offset, unsigned short auxFlags);
	void MakeSubNoFlagsRegImm8(RiverInstruction *rOut, const RiverRegister &reg, unsigned char offset, unsigned short auxFlags);
	void MakeSaveOp(RiverInstruction *rOut, unsigned char opType, const RiverOperand &op, const RiverInstruction &rIn);
	void MakeSaveAtxSP(RiverInstruction *rOut, const RiverInstruction &rIn);

	void SaveOperands(RiverInstruction *rOut, const RiverInstruction &rIn, nodep::DWORD &instrCount);

	/* Opcode translators */
	void TranslateUnk(RiverInstruction *rOut, const RiverInstruction &rIn, nodep::DWORD &instrCount);
	void TranslateDefault(RiverInstruction *rOut, const RiverInstruction &rIn, nodep::DWORD &instrCount);
	void TranslateSaveCPUID(RiverInstruction *rOut, const RiverInstruction &rIn, nodep::DWORD &instrCount);
	void TranslateSaveCMPXCHG8B(RiverInstruction *rOut, const RiverInstruction &rIn, nodep::DWORD &instrCount);

	template <TranslateOpcodeFunc subOp> void TranslateRep(RiverInstruction *rOut, const RiverInstruction &rIn, nodep::DWORD &instrCount) {
		if ((rIn.modifiers & RIVER_MODIFIER_REP) || (rIn.modifiers & RIVER_MODIFIER_REPZ) || (rIn.modifiers & RIVER_MODIFIER_REPNZ)) {
			RiverRegister tmpReg;

			tmpReg.name = RIVER_REG_xCX;
			MakeSaveReg(rOut, tmpReg, 0);
			instrCount++;
			rOut++;
		}

		(this->*subOp)(rOut, rIn, instrCount);
	}

	template <TranslateOpcodeFunc *fSubOps> void TranslateSubOp(RiverInstruction *rOut, const RiverInstruction &rIn, nodep::DWORD &instrCount) {
		(this->*fSubOps[rIn.subOpCode])(rOut, rIn, instrCount);
	}
};

#endif
