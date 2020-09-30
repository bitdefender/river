#ifndef _RIVER_X86_ASSEMBLER_H
#define _RIVER_X86_ASSEMBLER_H

#include "revtracer.h"
#include "river.h"
#include "Runtime.h"

#include "GenericX86Assembler.h"

class RiverX86Assembler : public GenericX86Assembler {
private :
	bool needsRAFix;
	nodep::BYTE *rvAddress;

	void SwitchToRiver(nodep::BYTE *&px86, nodep::DWORD &instrCounter);
	void SwitchToRiverEsp(nodep::BYTE *&px86, nodep::DWORD &instrCounter, nodep::BYTE repReg);
	void EndRiverConversion(nodep::BYTE *&px86, nodep::DWORD &pFlags, nodep::BYTE &repReg, nodep::DWORD &instrCounter);

	bool GenerateTransitions(const RiverInstruction &ri, nodep::BYTE *&px86, nodep::DWORD &pFlags, nodep::BYTE &repReg,  nodep::DWORD &instrCounter);

public :
	//bool Init(RiverRuntime *rt);
	//bool Assemble(RiverInstruction *pRiver,  nodep::WORD dwInstrCount, BYTE *px86,  nodep::WORD flg,  nodep::WORD &instrCounter,  nodep::WORD &byteCounter);
	virtual bool Translate(const RiverInstruction &ri, RelocableCodeBuffer &px86,  nodep::DWORD &pFlags, nodep::BYTE &currentFamily, nodep::BYTE &repReg,  nodep::DWORD &instrCounter, nodep::BYTE outputType);

private :
	void AssembleRiverAddSubInstr(const RiverInstruction &ri, RelocableCodeBuffer &px86, nodep::DWORD &pFlags, nodep::DWORD &instrCounter);

	void AssembleRiverAddSubOp(const RiverInstruction &ri, RelocableCodeBuffer &px86);

	void AssembleRegModRMOp(const RiverInstruction &ri, RelocableCodeBuffer &px86);

	void AssembleModRMOp(unsigned int opIdx, const RiverInstruction &ri, RelocableCodeBuffer &px86, nodep::BYTE extra);
};


#endif
