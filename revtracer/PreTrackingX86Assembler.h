#ifndef _PRETRACKING_ASSEMBLER_H_
#define _PRETRACKING_ASSEMBLER_H_

#include "GenericX86Assembler.h"

class PreTrackingAssembler : public GenericX86Assembler {
private :
	void AssemblePreTrackMem(RiverAddress *addr, bool saveVal, nodep::BYTE riverFamily, RelocableCodeBuffer &px86, nodep::DWORD &instrCounter);
public :
	virtual bool Translate(const RiverInstruction &ri, RelocableCodeBuffer &px86, nodep::DWORD &pFlags, nodep::BYTE &currentFamily, nodep::BYTE &repReg, nodep::DWORD &instrCounter, nodep::BYTE outputType);
};

#endif
