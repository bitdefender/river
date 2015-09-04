#ifndef _X86_ASSEMBLER_H
#define _X86_ASSEMBLER_H

#include "GenericX86Assembler.h"

// also include sub assemblers
#include "NativeX86Assembler.h"
#include "RiverX86Assembler.h"
#include "PreTrackingX86Assembler.h"
#include "TrackingX86Assembler.h"


class X86Assembler : public GenericX86Assembler {
private :
	NativeX86Assembler nAsm;
	RiverX86Assembler rAsm;
	PreTrackingAssembler ptAsm;
	TrackingX86Assembler tAsm;
	// SymbopX86Assembler sAsm;

	void SwitchToRiver(BYTE *&px86, DWORD &instrCounter);
	void SwitchToRiverEsp(BYTE *&px86, DWORD &instrCounter, BYTE repReg);
	void EndRiverConversion(BYTE *&px86, DWORD &pFlags, BYTE &repReg, DWORD &instrCounter);

	bool GenerateTransitions(const RiverInstruction &ri, BYTE *&px86, DWORD &pFlags, BYTE &repReg, DWORD &instrCounter);
	bool Translate(const RiverInstruction &ri, RelocableCodeBuffer &px86, DWORD &pFlags, BYTE &repReg, DWORD &instrCounter);
public :
	virtual bool Init(RiverRuntime *rt);

	virtual bool Assemble(RiverInstruction *pRiver, DWORD dwInstrCount, RelocableCodeBuffer &px86, DWORD flg, DWORD &instrCounter, DWORD &byteCounter);
};



#endif