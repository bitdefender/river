#ifndef _GENERIC_X86_ASSEMBLER_H
#define _GENERIC_X86_ASSEMBLER_H

#include "extern.h"
#include "river.h"
#include "Runtime.h"

class GenericX86Assembler {
protected :
	RiverRuntime *runtime;
public :
	bool Init(RiverRuntime *rt);
	virtual bool Assemble(RiverInstruction *pRiver, DWORD dwInstrCount, BYTE *px86, DWORD flg, DWORD &instrCounter, DWORD &byteCounter) = 0;
};

#endif