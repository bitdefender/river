#ifndef _GENERIC_X86_ASSEMBLER_H
#define _GENERIC_X86_ASSEMBLER_H

#include "extern.h"
#include "river.h"
#include "Runtime.h"

#include "RelocableCodeBuffer.h"

class GenericX86Assembler {
protected :
	RiverRuntime *runtime;

public:
	virtual bool Init(RiverRuntime *rt);
	virtual bool Translate(const RiverInstruction &ri, RelocableCodeBuffer &px86, DWORD &pFlags, BYTE &repReg, DWORD &instrCounter) = 0;
};

bool GeneratePrefixes(const RiverInstruction &ri, BYTE *&px86);
bool ClearPrefixes(const RiverInstruction &ri, BYTE *&px86);

#endif