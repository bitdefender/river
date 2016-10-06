#ifndef _RIVER_META_TRANSLATOR_H
#define _RIVER_META_TRANSLATOR_H

#include "revtracer.h"
#include "river.h"

class RiverCodeGen;

class RiverMetaTranslator {
private: 
	RiverCodeGen *codegen;

	void CopyInstruction(RiverInstruction &rOut, const RiverInstruction &rIn);

	typedef void(RiverMetaTranslator::*TranslateOpcodeFunc)(RiverInstruction *rOut, const RiverInstruction &rIn, rev::DWORD &instrCount);

	static TranslateOpcodeFunc translateOpcodes[2][0x100];
	static TranslateOpcodeFunc translate0xFFOp[8];
public:
	bool Init(RiverCodeGen *cg);
	void Translate(const RiverInstruction &rIn, RiverInstruction *rOut, rev::DWORD &instrCount);

private:
	void MakeAddNoFlagsRegImm8(RiverInstruction *rOut, const RiverRegister &reg, unsigned char offset, rev::BYTE family, rev::DWORD addr);
	void MakeSubNoFlagsRegImm8(RiverInstruction *rOut, const RiverRegister &reg, unsigned char offset, rev::BYTE family, rev::DWORD addr);

	void MakeMovRegMem32(RiverInstruction *rOut, const RiverRegister &reg, const RiverAddress &mem, rev::BYTE family, rev::DWORD addr);
	void MakeMovMemReg32(RiverInstruction *rOut, const RiverAddress &mem, const RiverRegister &reg, rev::BYTE family, rev::DWORD addr);
	void MakeMovMemMem32(RiverInstruction *rOut, const RiverAddress &memd, const RiverAddress &mems, rev::BYTE family, rev::DWORD addr);


	void MakeMovMemImm32(RiverInstruction *rOut, const RiverAddress &mem, rev::DWORD imm, rev::BYTE family, rev::DWORD addr);

	void TranslateUnk(RiverInstruction *rOut, const RiverInstruction &rIn, rev::DWORD &instrCount);
	void TranslateDefault(RiverInstruction *rOut, const RiverInstruction &rIn, rev::DWORD &instrCount);
	void TranslatePushReg(RiverInstruction *rOut, const RiverInstruction &rIn, rev::DWORD &instrCount);
	void TranslatePushMem(RiverInstruction *rOut, const RiverInstruction &rIn, rev::DWORD &instrCount);
	void TranslatePusha(RiverInstruction *rOut, const RiverInstruction &rIn, rev::DWORD &instrCount);

	void TranslatePopReg(RiverInstruction *rOut, const RiverInstruction &rIn, rev::DWORD &instrCount);
	void TranslatePopMem(RiverInstruction *rOut, const RiverInstruction &rIn, rev::DWORD &instrCount);

	void TranslateCall(RiverInstruction *rOut, const RiverInstruction &rIn, rev::DWORD &instrCount);
	void TranslateRet(RiverInstruction *rOut, const RiverInstruction &rIn, rev::DWORD &instrCount);
	void TranslateRetn(RiverInstruction *rOut, const RiverInstruction &rIn, rev::DWORD &instrCount);

	template <TranslateOpcodeFunc *fSubOps> void TranslateSubOp(RiverInstruction *rOut, const RiverInstruction &rIn, rev::DWORD &instrCount) {
		(this->*fSubOps[rIn.subOpCode])(rOut, rIn, instrCount);
	}
};

#endif
