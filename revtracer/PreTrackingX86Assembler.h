#ifndef _PRETRACKING_ASSEMBLER_H_
#define _PRETRACKING_ASSEMBLER_H_

#include "GenericX86Assembler.h"

class PreTrackingAssembler : public GenericX86Assembler {
private :
	void AssemblePreTrackMem(const RiverAddress *addr, RelocableCodeBuffer &px86, DWORD &instrCounter);
public :
	virtual bool Translate(const RiverInstruction &ri, RelocableCodeBuffer &px86, DWORD &pFlags, BYTE &repReg, DWORD &instrCounter);
};

#endif
