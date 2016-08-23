#ifndef _PRETRACKING_ASSEMBLER_H_
#define _PRETRACKING_ASSEMBLER_H_

#include "GenericX86Assembler.h"

class PreTrackingAssembler : public GenericX86Assembler {
private :
	void AssemblePreTrackMem(RiverAddress *addr, rev::BYTE riverFamily, RelocableCodeBuffer &px86, rev::DWORD &instrCounter);
public :
	virtual bool Translate(const RiverInstruction &ri, RelocableCodeBuffer &px86, rev::DWORD &pFlags, rev::BYTE &currentFamily, rev::BYTE &repReg, rev::DWORD &instrCounter, rev::BYTE outputType);
};

#endif
