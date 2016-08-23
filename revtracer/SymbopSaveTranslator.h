#ifndef _SYMBOP_SAVE_TRANSLATOR_H_
#define _SYMBOP_SAVE_TRANSLATOR_H_

#include "revtracer.h"
#include "river.h"

class SymbopSaveTranslator {
private :
	RiverCodeGen *codegen;

	void CopyInstruction(RiverInstruction &rOut, const RiverInstruction &rIn);

	void MakePushFlg(rev::BYTE flags, RiverInstruction *&rOut, rev::DWORD &instrCount);
	void MakePushReg(RiverRegister reg, RiverInstruction *&rOut, rev::DWORD &instrCount);
	void MakePushMem(const RiverInstruction &rIn, RiverInstruction *&rOut, rev::DWORD &instrCount);
public :
	bool Init(RiverCodeGen *cg);

	bool Translate(const RiverInstruction &rIn, RiverInstruction *rMainOut, rev::DWORD &instrCount);
};


#endif
