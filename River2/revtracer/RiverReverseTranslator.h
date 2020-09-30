#ifndef _RIVER_REVERSE_TRANSLATOR_H
#define _RIVER_REVERSE_TRANSLATOR_H

#include "revtracer.h"
#include "river.h"

using namespace rev;

class RiverCodeGen;

class RiverReverseTranslator {
private :
	RiverCodeGen *codegen;
	typedef void(RiverReverseTranslator::*TranslateOpcodeFunc)(RiverInstruction &rOut, const RiverInstruction &rIn);

	static TranslateOpcodeFunc translateOpcodes[2][0x100];
public :
	bool Init(RiverCodeGen *cg);
	bool Translate(const RiverInstruction &rIn, RiverInstruction &rOut);
private :
	void TranslateUnk(RiverInstruction &rOut, const RiverInstruction &rIn);
	void TranslatePushReg(RiverInstruction &rOut, const RiverInstruction &rIn);
	void TranslatePopReg(RiverInstruction &rOut, const RiverInstruction &rIn);
	void TranslatePushf(RiverInstruction &rOut, const RiverInstruction &rIn);
	void TranslatePopf(RiverInstruction &rOut, const RiverInstruction &rIn);
	void TranslatePushModRM(RiverInstruction &rOut, const RiverInstruction &rIn);
	void TranslatePopModRM(RiverInstruction &rOut, const RiverInstruction &rIn);
	void Translate0x83(RiverInstruction &rOut, const RiverInstruction &rIn);
};

#endif
