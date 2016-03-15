#ifndef _PRETRACKING_ASSEMBLER_H_
#define _PRETRACKING_ASSEMBLER_H_

#include "GenericX86Assembler.h"

class PreTrackingAssembler : public GenericX86Assembler {
private :
	void AssemblePreTrackMem(RiverAddress *addr, BYTE riverFamily, RelocableCodeBuffer &px86, DWORD &instrCounter);
public :
	virtual bool Translate(const RiverInstruction &ri, RelocableCodeBuffer &px86, DWORD &pFlags, BYTE &currentFamily, BYTE &repReg, DWORD &instrCounter, BYTE outputType);
};

#endif
