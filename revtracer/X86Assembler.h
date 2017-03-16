#ifndef _X86_ASSEMBLER_H
#define _X86_ASSEMBLER_H

#include "GenericX86Assembler.h"

// also include sub assemblers
#include "NativeX86Assembler.h"
#include "RiverX86Assembler.h"
#include "PreTrackingX86Assembler.h"
#include "TrackingX86Assembler.h"
#include "RiverTrackingX86Assembler.h"


class X86Assembler /*: public GenericX86Assembler*/ {
private :
	RiverRuntime *runtime;

	NativeX86Assembler nAsm;
	RiverX86Assembler rAsm;
	PreTrackingAssembler ptAsm;
	TrackingX86Assembler tAsm;
	RiverTrackingX86Assembler rtAsm;

	void SwitchToRiver(nodep::BYTE *&px86, nodep::DWORD &instrCounter);
	void SwitchToRiverEsp(nodep::BYTE *&px86, nodep::DWORD &instrCounter, nodep::BYTE repReg);
	void EndRiverConversion(RelocableCodeBuffer &px86, nodep::DWORD &pFlags, nodep::BYTE &currentFamily, nodep::BYTE &repReg, nodep::DWORD &instrCounter);

	void SwitchEspWithReg(RelocableCodeBuffer &px86, nodep::DWORD &instrCounter, nodep::BYTE repReg, nodep::DWORD dwStack);
	void SwitchToStack(RelocableCodeBuffer &px86, nodep::DWORD &instrCounter, nodep::DWORD dwStack);
	bool SwitchToNative(RelocableCodeBuffer &px86, nodep::BYTE &currentFamily, nodep::BYTE repReg, nodep::DWORD &instrCounter, nodep::DWORD dwStack);

	bool GenerateTransitionsNative(const RiverInstruction &ri, RelocableCodeBuffer &px86, nodep::DWORD &pFlags, nodep::BYTE &currentFamily, nodep::BYTE &repReg, nodep::DWORD &instrCounter);
	bool GenerateTransitionsTracking(const RiverInstruction &ri, RelocableCodeBuffer &px86, nodep::DWORD &pFlags, nodep::BYTE &currentFamily, nodep::BYTE &repReg, nodep::DWORD &instrCounter);

	void AssembleTrackingEnter(RelocableCodeBuffer &px86, nodep::DWORD &instrCounter);
	void AssembleTrackingLeave(RelocableCodeBuffer &px86, nodep::DWORD &instrCounter);

	//bool GenerateTransitions(const RiverInstruction &ri, nodep::BYTE *&px86, nodep::DWORD &pFlags, nodep::BYTE &repReg, nodep::DWORD &instrCounter);
	bool TranslateNative(const RiverInstruction &ri, RelocableCodeBuffer &px86, nodep::DWORD &pFlags, nodep::BYTE &currentFamily, nodep::BYTE &repReg, nodep::DWORD &instrCounter, nodep::BYTE outputType);
	bool TranslateTracking(const RiverInstruction &ri, RelocableCodeBuffer &px86, nodep::DWORD &pFlags, nodep::BYTE &currentFamily, nodep::BYTE &repReg, nodep::DWORD &instrCounter, nodep::BYTE outputType);
public :
	virtual bool Init(RiverRuntime *rt, nodep::DWORD dwTranslationFlags);

	virtual bool Assemble(RiverInstruction *pRiver, nodep::DWORD dwInstrCount, RelocableCodeBuffer &px86, nodep::DWORD flg, nodep::DWORD &instrCounter, nodep::DWORD &byteCounter, nodep::BYTE outputType);
	//bool AssembleTracking(RiverInstruction *pRiver, nodep::DWORD dwInstrCount, RelocableCodeBuffer &px86, nodep::DWORD flg, nodep::DWORD &instrCounter, nodep::DWORD &byteCounter);
};



#endif