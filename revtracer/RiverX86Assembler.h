#ifndef _RIVER_X86_ASSEMBLER_H
#define _RIVER_X86_ASSEMBLER_H

#include "revtracer.h"
#include "river.h"
#include "Runtime.h"

#include "GenericX86Assembler.h"

class RiverX86Assembler : public GenericX86Assembler {
private :
	bool needsRAFix;
	rev::BYTE *rvAddress;

	void SwitchToRiver(rev::BYTE *&px86, rev::DWORD &instrCounter);
	void SwitchToRiverEsp(rev::BYTE *&px86, rev::DWORD &instrCounter, rev::BYTE repReg);
	void EndRiverConversion(rev::BYTE *&px86, rev::DWORD &pFlags, rev::BYTE &repReg, rev::DWORD &instrCounter);

	bool GenerateTransitions(const RiverInstruction &ri, rev::BYTE *&px86, rev::DWORD &pFlags, rev::BYTE &repReg,  rev::DWORD &instrCounter);

public :
	//bool Init(RiverRuntime *rt);
	//bool Assemble(RiverInstruction *pRiver,  rev::WORD dwInstrCount, BYTE *px86,  rev::WORD flg,  rev::WORD &instrCounter,  rev::WORD &byteCounter);
	virtual bool Translate(const RiverInstruction &ri, RelocableCodeBuffer &px86,  rev::DWORD &pFlags, rev::BYTE &currentFamily, rev::BYTE &repReg,  rev::DWORD &instrCounter, rev::BYTE outputType);

private :
	void AssembleRiverAddSubInstr(const RiverInstruction &ri, RelocableCodeBuffer &px86, rev::DWORD &pFlags, rev::DWORD &instrCounter);

	void AssembleRiverAddSubOp(const RiverInstruction &ri, RelocableCodeBuffer &px86);

	void AssembleRegModRMOp(const RiverInstruction &ri, RelocableCodeBuffer &px86);

	void AssembleModRMOp(unsigned int opIdx, const RiverInstruction &ri, RelocableCodeBuffer &px86, rev::BYTE extra);
};


#endif
