#ifndef _RIVER_X86_ASSEMBLER_H
#define _RIVER_X86_ASSEMBLER_H

#include "revtracer.h"
#include "river.h"
#include "Runtime.h"

#include "GenericX86Assembler.h"

using namespace rev;

class RiverX86Assembler : public GenericX86Assembler {
private :
	bool needsRAFix;
	BYTE *rvAddress;

	void SwitchToRiver(BYTE *&px86, DWORD &instrCounter);
	void SwitchToRiverEsp(BYTE *&px86, DWORD &instrCounter, BYTE repReg);
	void EndRiverConversion(BYTE *&px86, DWORD &pFlags, BYTE &repReg, DWORD &instrCounter);

	bool GenerateTransitions(const RiverInstruction &ri, BYTE *&px86, DWORD &pFlags, BYTE &repReg, DWORD &instrCounter);

public :
	//bool Init(RiverRuntime *rt);
	//bool Assemble(RiverInstruction *pRiver, DWORD dwInstrCount, BYTE *px86, DWORD flg, DWORD &instrCounter, DWORD &byteCounter);
	virtual bool Translate(const RiverInstruction &ri, RelocableCodeBuffer &px86, DWORD &pFlags, BYTE &currentFamily, BYTE &repReg, DWORD &instrCounter);

private :
	void AssembleRiverAddSubInstr(const RiverInstruction &ri, RelocableCodeBuffer &px86, DWORD &pFlags, DWORD &instrCounter);

	void AssembleRiverAddSubOp(const RiverInstruction &ri, RelocableCodeBuffer &px86);

	void AssembleRegModRMOp(const RiverInstruction &ri, RelocableCodeBuffer &px86);

	void AssembleModRMOp(unsigned int opIdx, const RiverInstruction &ri, RelocableCodeBuffer &px86, BYTE extra);
};


#endif
