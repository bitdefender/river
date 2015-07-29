#ifndef _RIVER_META_TRANSLATOR_H
#define _RIVER_META_TRANSLATOR_H

#include "extern.h"
#include "river.h"

class RiverCodeGen;

class RiverMetaTranslator {
private: 
	RiverCodeGen *codegen;

	void RiverMetaTranslator::CopyInstruction(RiverInstruction &rOut, const RiverInstruction &rIn);

	typedef void(RiverMetaTranslator::*TranslateOpcodeFunc)(RiverInstruction *rOut, const RiverInstruction &rIn, DWORD &instrCount);

public:
	bool Init(RiverCodeGen *cg);
	void Translate(const RiverInstruction &rIn, RiverInstruction *rOut, DWORD &instrCount);

private:
	void MakeAddNoFlagsRegImm8(RiverInstruction *rOut, const RiverRegister &reg, unsigned char offset, unsigned short auxFlags);
	void MakeSubNoFlagsRegImm8(RiverInstruction *rOut, const RiverRegister &reg, unsigned char offset, unsigned short auxFlags);

	void MakeMovRegMem32(RiverInstruction *rOut, const RiverRegister &reg);
	void MakeMovMemReg32(RiverInstruction *rOut, const RiverRegister &reg);
};

#endif