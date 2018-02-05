#ifndef _RIVER_REP_TRANSLATOR_
#define _RIVER_REP_TRANSLATOR_

#include "river.h"

class RiverCodeGen;

class RiverRepTranslator {
	public:
		bool Init(RiverCodeGen *cg);
		bool Translate(const RiverInstruction &rIn, RiverInstruction *rOut, nodep::DWORD &instrCount);
	private:
		RiverCodeGen *codegen;
		void TranslateDefault(const RiverInstruction &rIn, RiverInstruction *rOut, nodep::DWORD &instrCount);
		void TranslateCommon(const RiverInstruction &rIn, RiverInstruction *rOut, nodep::DWORD &instrCount, nodep::DWORD riverModifier);
};

#endif
