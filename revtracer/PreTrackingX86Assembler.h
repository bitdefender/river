#ifndef _PRETRACKING_ASSEMBLER_H_
#define _PRETRACKING_ASSEMBLER_H_

#include "GenericX86Assembler.h"

class PreTrackingAssembler : public GenericX86Assembler {
private :
	static const nodep::BYTE regByte[];

	void SaveUnusedRegister(nodep::BYTE reg, RelocableCodeBuffer &px86);
	void RestoreUnusedRegister(nodep::BYTE reg, RelocableCodeBuffer &px86);

	void AssemblePreTrackAddr(RiverAddress *addr, nodep::BYTE riverFamily, RelocableCodeBuffer &px86, nodep::DWORD &instrCounter);
	void AssemblePreTrackMem(RiverAddress * addr, RelocableCodeBuffer & px86, nodep::DWORD & instrCounter);
public :
	virtual bool Translate(const RiverInstruction &ri, RelocableCodeBuffer &px86, nodep::DWORD &pFlags, nodep::BYTE &currentFamily, nodep::BYTE &repReg, nodep::DWORD &instrCounter, nodep::BYTE outputType);
};

#endif
