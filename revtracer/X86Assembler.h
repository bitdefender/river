#ifndef _X86_ASSEMBLER_H
#define _X86_ASSEMBLER_H

#include "GenericX86Assembler.h"

// also include sub assemblers
#include "NativeX86Assembler.h"
#include "RiverX86Assembler.h"
#include "PreTrackingX86Assembler.h"
#include "TrackingX86Assembler.h"
#include "RiverTrackingX86Assembler.h"


class X86Assembler : public GenericX86Assembler {
private :
	NativeX86Assembler nAsm;
	RiverX86Assembler rAsm;
	PreTrackingAssembler ptAsm;
	TrackingX86Assembler tAsm;
	RiverTrackingX86Assembler rtAsm;
	// SymbopX86Assembler sAsm;

	void SwitchToRiver(BYTE *&px86, DWORD &instrCounter);
	void SwitchToRiverEsp(BYTE *&px86, DWORD &instrCounter, BYTE repReg);
	void EndRiverConversion(RelocableCodeBuffer &px86, DWORD &pFlags, BYTE &currentFamily, BYTE &repReg, DWORD &instrCounter);

	void SwitchEspWithReg(RelocableCodeBuffer &px86, DWORD &instrCounter, BYTE repReg, DWORD dwStack);
	void SwitchToStack(RelocableCodeBuffer &px86, DWORD &instrCounter, DWORD dwStack);
	bool SwitchToNative(RelocableCodeBuffer &px86, BYTE &currentFamily, BYTE repReg, DWORD &instrCounter, DWORD dwStack);

	bool GenerateTransitions(const RiverInstruction &ri, RelocableCodeBuffer &px86, DWORD &pFlags, BYTE &currentFamily, BYTE &repReg, DWORD &instrCounter);

	void AssembleTrackingEnter(RelocableCodeBuffer &px86, DWORD &instrCounter);
	void AssembleTrackingLeave(RelocableCodeBuffer &px86, DWORD &instrCounter);

	//bool GenerateTransitions(const RiverInstruction &ri, BYTE *&px86, DWORD &pFlags, BYTE &repReg, DWORD &instrCounter);
	bool Translate(const RiverInstruction &ri, RelocableCodeBuffer &px86, DWORD &pFlags, BYTE &currentFamily, BYTE &repReg, DWORD &instrCounter);
public :
	virtual bool Init(RiverRuntime *rt);

	virtual bool Assemble(RiverInstruction *pRiver, DWORD dwInstrCount, RelocableCodeBuffer &px86, DWORD flg, DWORD &instrCounter, DWORD &byteCounter);
	bool AssembleTracking(RiverInstruction *pRiver, DWORD dwInstrCount, RelocableCodeBuffer &px86, DWORD flg, DWORD &instrCounter, DWORD &byteCounter);
};



#endif