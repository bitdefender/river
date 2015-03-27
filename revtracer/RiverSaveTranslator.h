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
public :
	bool Init(RiverCodeGen *cg);
	void Translate(const RiverInstruction &rIn, RiverInstruction *rOut, DWORD &instrCount);

private :
	/* Translation helpers */
	void MakeSaveFlags(RiverInstruction *rOut);
	void MakeSaveReg(RiverInstruction *rOut, const RiverRegister &reg, unsigned short auxFlags);
	void MakeSaveMem(RiverInstruction *rOut, const RiverAddress &mem, unsigned short auxFlags);
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
};

#endif
