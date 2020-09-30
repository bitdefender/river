#ifndef _SYMBOP_SAVE_TRANSLATOR_H_
#define _SYMBOP_SAVE_TRANSLATOR_H_

#include "revtracer.h"
#include "river.h"

class SymbopSaveTranslator {
private :
	RiverCodeGen *codegen;

	void MakePushFlg(nodep::BYTE flags, RiverInstruction *&rOut, nodep::DWORD &instrCount);
	void MakePushReg(RiverRegister reg, RiverInstruction *&rOut, nodep::DWORD &instrCount);
	void MakePushMem(const RiverInstruction &rIn, RiverInstruction *&rOut, nodep::DWORD &instrCount);
public :
	bool Init(RiverCodeGen *cg);

	bool Translate(const RiverInstruction &rIn, RiverInstruction *rMainOut, nodep::DWORD &instrCount);
};


#endif
