#include "RiverRepTranslator.h"
#include "CodeGen.h"
#include "TranslatorUtil.h"

bool RiverRepTranslator::Init(RiverCodeGen *cg) {
	codegen = cg;
	return true;
}

bool RiverRepTranslator::Translate(const RiverInstruction &rIn, RiverInstruction *rOut, nodep::DWORD &instrCount) {
	if (rIn.modifiers & RIVER_MODIFIER_REP ||
			rIn.modifiers & RIVER_MODIFIER_REPZ ||
			rIn.modifiers & RIVER_MODIFIER_REPNZ) {
		//translate rep
	} else {
		CopyInstruction(codegen, rOut[0], rIn);
		instrCount += 1;
	}
	return true;
}

