#ifndef _RIVER_META_TRANSLATOR_H
#define _RIVER_META_TRANSLATOR_H

#include "revtracer.h"
#include "river.h"

class RiverCodeGen;

class RiverMetaTranslator {
private: 
	RiverCodeGen *codegen;

	void CopyInstruction(RiverInstruction &rOut, const RiverInstruction &rIn);

	typedef void(RiverMetaTranslator::*TranslateOpcodeFunc)(RiverInstruction *rOut, const RiverInstruction &rIn, nodep::DWORD &instrCount);

	static TranslateOpcodeFunc translateOpcodes[2][0x100];
	static TranslateOpcodeFunc translate0xFFOp[8];
public:
	bool Init(RiverCodeGen *cg);
	void Translate(const RiverInstruction &rIn, RiverInstruction *rOut, nodep::DWORD &instrCount);

private:
	void MakeAddNoFlagsRegImm8(RiverInstruction *rOut, const RiverRegister &reg, unsigned char offset, nodep::BYTE family, nodep::DWORD addr);
	void MakeSubNoFlagsRegImm8(RiverInstruction *rOut, const RiverRegister &reg, unsigned char offset, nodep::BYTE family, nodep::DWORD addr);

	void MakeMovRegMem32(RiverInstruction *rOut, const RiverRegister &reg, const RiverAddress &mem, nodep::BYTE family, nodep::DWORD addr);
	void MakeMovMemReg32(RiverInstruction *rOut, const RiverAddress &mem, const RiverRegister &reg, nodep::BYTE family, nodep::DWORD addr);
	void MakeMovMemMem32(RiverInstruction *rOut, const RiverAddress &memd, const RiverAddress &mems, nodep::BYTE family, nodep::DWORD addr);


	void MakeMovMemImm32(RiverInstruction *rOut, const RiverAddress &mem, nodep::DWORD imm, nodep::BYTE family, nodep::DWORD addr);

	void TranslateUnk(RiverInstruction *rOut, const RiverInstruction &rIn, nodep::DWORD &instrCount);
	void TranslateDefault(RiverInstruction *rOut, const RiverInstruction &rIn, nodep::DWORD &instrCount);
	void TranslatePushReg(RiverInstruction *rOut, const RiverInstruction &rIn, nodep::DWORD &instrCount);
	void TranslatePushMem(RiverInstruction *rOut, const RiverInstruction &rIn, nodep::DWORD &instrCount);
	void TranslatePusha(RiverInstruction *rOut, const RiverInstruction &rIn, nodep::DWORD &instrCount);

	void TranslatePopReg(RiverInstruction *rOut, const RiverInstruction &rIn, nodep::DWORD &instrCount);
	void TranslatePopMem(RiverInstruction *rOut, const RiverInstruction &rIn, nodep::DWORD &instrCount);

	void TranslateCall(RiverInstruction *rOut, const RiverInstruction &rIn, nodep::DWORD &instrCount);
	void TranslateRet(RiverInstruction *rOut, const RiverInstruction &rIn, nodep::DWORD &instrCount);
	void TranslateRetn(RiverInstruction *rOut, const RiverInstruction &rIn, nodep::DWORD &instrCount);

	template <TranslateOpcodeFunc *fSubOps> void TranslateSubOp(RiverInstruction *rOut, const RiverInstruction &rIn, nodep::DWORD &instrCount) {
		(this->*fSubOps[rIn.subOpCode])(rOut, rIn, instrCount);
	}
};

#endif
