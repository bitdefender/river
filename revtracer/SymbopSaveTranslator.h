#ifndef _SYMBOP_SAVE_TRANSLATOR_H_
#define _SYMBOP_SAVE_TRANSLATOR_H_

#include "revtracer.h"
#include "river.h"

class SymbopSaveTranslator {
private :
	RiverCodeGen *codegen;

	void CopyInstruction(RiverInstruction &rOut, const RiverInstruction &rIn);

	void MakePushFlg(rev::BYTE flags, RiverInstruction *&rOut, rev::DWORD &instrCount, rev::DWORD index);
	void MakePushReg(RiverRegister reg, RiverInstruction *&rOut, rev::DWORD &instrCount, rev::DWORD index);
	void MakePushMem(const RiverInstruction &rIn, RiverInstruction *&rOut, rev::DWORD &instrCount, rev::DWORD index);
public :
	bool Init(RiverCodeGen *cg);

	bool Translate(const RiverInstruction &rIn, RiverInstruction *rMainOut, rev::DWORD &instrCount);
};


#endif
