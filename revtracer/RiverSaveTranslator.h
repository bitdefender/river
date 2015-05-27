#ifndef _RIVER_SAVE_TRANSLATOR_H
#define _RIVER_SAVE_TRANSLATOR_H

#include "extern.h"
#include "river.h"

class RiverCodeGen;

class RiverSaveTranslator {
private :
	RiverCodeGen *codegen;

	void RiverSaveTranslator::CopyInstruction(RiverInstruction &rOut, const RiverInstruction &rIn);

	typedef void(RiverSaveTranslator::*TranslateOpcodeFunc)(RiverInstruction *rOut, const RiverInstruction &rIn, DWORD &instrCount);
	
	static TranslateOpcodeFunc translateOpcodes[2][0x100];
	static TranslateOpcodeFunc translate0xF7[8], translate0xFF[8], translate0x0FC7[8];

public :
	bool Init(RiverCodeGen *cg);
	void Translate(const RiverInstruction &rIn, RiverInstruction *rOut, DWORD &instrCount);

private :
	/* Translation helpers */
	void MakeSaveFlags(RiverInstruction *rOut);
	void MakeSaveReg(RiverInstruction *rOut, const RiverRegister &reg, unsigned short auxFlags);
	void MakeSaveMem(RiverInstruction *rOut, const RiverAddress &mem, unsigned short auxFlags);
	void MakeSaveMemOffset(RiverInstruction *rOut, const RiverAddress &mem, int offset, unsigned short auxFlags);
	void MakeAddNoFlagsRegImm8(RiverInstruction *rOut, const RiverRegister &reg, unsigned char offset, unsigned short auxFlags);
	void MakeSubNoFlagsRegImm8(RiverInstruction *rOut, const RiverRegister &reg, unsigned char offset, unsigned short auxFlags);
	void MakeSaveOp(RiverInstruction *rOut, unsigned char opType, const RiverOperand &op);
	void MakeSaveAtxSP(RiverInstruction *rOut);

	void SaveOperands(RiverInstruction *rOut, const RiverInstruction &rIn, DWORD &instrCount);

	/* Opcode translators */
	void TranslateUnk(RiverInstruction *rOut, const RiverInstruction &rIn, DWORD &instrCount);
	void TranslateDefault(RiverInstruction *rOut, const RiverInstruction &rIn, DWORD &instrCount);
	void TranslatePush(RiverInstruction *rOut, const RiverInstruction &rIn, DWORD &instrCount);
	void TranslatePop(RiverInstruction *rOut, const RiverInstruction &rIn, DWORD &instrCount);
	void TranslateRetn(RiverInstruction *rOut, const RiverInstruction &rIn, DWORD &instrCount);
	void TranslateSavexAX(RiverInstruction *rOut, const RiverInstruction &rIn, DWORD &instrCount);
	void TranslateSavexDX(RiverInstruction *rOut, const RiverInstruction &rIn, DWORD &instrCount);
	void TranslateSavexAXxDX(RiverInstruction *rOut, const RiverInstruction &rIn, DWORD &instrCount);
	void TranslateSavexSPxBP(RiverInstruction *rOut, const RiverInstruction &rIn, DWORD &instrCount);
	void TranslateSavexSIxDI(RiverInstruction *rOut, const RiverInstruction &rIn, DWORD &instrCount);
	void TranslateSaveCPUID(RiverInstruction *rOut, const RiverInstruction &rIn, DWORD &instrCount);
	void TranslateSaveCMPXCHG8B(RiverInstruction *rOut, const RiverInstruction &rIn, DWORD &instrCount);

	template <TranslateOpcodeFunc *fSubOps> void TranslateSubOp(RiverInstruction *rOut, const RiverInstruction &rIn, DWORD &instrCount) {
		(this->*fSubOps[rIn.subOpCode])(rOut, rIn, instrCount);
	}
};

#endif
