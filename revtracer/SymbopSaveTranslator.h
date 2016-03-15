#ifndef _SYMBOP_SAVE_TRANSLATOR_H_
#define _SYMBOP_SAVE_TRANSLATOR_H_

#include "revtracer.h"
#include "river.h"

using namespace rev;

class SymbopSaveTranslator {
private :
	RiverCodeGen *codegen;

	void CopyInstruction(RiverInstruction &rOut, const RiverInstruction &rIn);

	void MakePushFlg(BYTE flags, RiverInstruction *&rOut, DWORD &instrCount);
	void MakePushReg(RiverRegister reg, RiverInstruction *&rOut, DWORD &instrCount);
	void MakePushMem(const RiverInstruction &rIn, RiverInstruction *&rOut, DWORD &instrCount);
public :
	bool Init(RiverCodeGen *cg);

	bool Translate(const RiverInstruction &rIn, RiverInstruction *rMainOut, DWORD &instrCount);
};


#endif
