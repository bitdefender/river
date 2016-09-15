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

	void SwitchToRiver(rev::BYTE *&px86, rev::DWORD &instrCounter);
	void SwitchToRiverEsp(rev::BYTE *&px86, rev::DWORD &instrCounter, rev::BYTE repReg);
	void EndRiverConversion(RelocableCodeBuffer &px86, rev::DWORD &pFlags, rev::BYTE &currentFamily, rev::BYTE &repReg, rev::DWORD &instrCounter);

	void SwitchEspWithReg(RelocableCodeBuffer &px86, rev::DWORD &instrCounter, rev::BYTE repReg, rev::DWORD dwStack);
	void SwitchToStack(RelocableCodeBuffer &px86, rev::DWORD &instrCounter, rev::DWORD dwStack);
	bool SwitchToNative(RelocableCodeBuffer &px86, rev::BYTE &currentFamily, rev::BYTE repReg, rev::DWORD &instrCounter, rev::DWORD dwStack);

	bool GenerateTransitionsNative(const RiverInstruction &ri, RelocableCodeBuffer &px86, rev::DWORD &pFlags, rev::BYTE &currentFamily, rev::BYTE &repReg, rev::DWORD &instrCounter);
	bool GenerateTransitionsTracking(const RiverInstruction &ri, RelocableCodeBuffer &px86, rev::DWORD &pFlags, rev::BYTE &currentFamily, rev::BYTE &repReg, rev::DWORD &instrCounter);

	void AssembleTrackingEnter(RelocableCodeBuffer &px86, rev::DWORD &instrCounter);
	void AssembleTrackingLeave(RelocableCodeBuffer &px86, rev::DWORD &instrCounter);

	void AssembleHook(RelocableCodeBuffer & px86, rev::DWORD &instrCounter, rev::DWORD address);

	//bool GenerateTransitions(const RiverInstruction &ri, rev::BYTE *&px86, rev::DWORD &pFlags, rev::BYTE &repReg, rev::DWORD &instrCounter);
	bool TranslateNative(const RiverInstruction &ri, RelocableCodeBuffer &px86, rev::DWORD &pFlags, rev::BYTE &currentFamily, rev::BYTE &repReg, rev::DWORD &instrCounter, rev::BYTE outputType);
	bool TranslateTracking(const RiverInstruction &ri, RelocableCodeBuffer &px86, rev::DWORD &pFlags, rev::BYTE &currentFamily, rev::BYTE &repReg, rev::DWORD &instrCounter, rev::BYTE outputType);

	RiverInstruction *original;
	unsigned int originalIdx;
public :
	virtual bool Init(RiverRuntime *rt, rev::DWORD dwTranslationFlags);
	
	void SetOriginalInstructions(RiverInstruction *ptr);
	void MarkOriginalInstruction(unsigned int offset);

	virtual bool Assemble(RiverInstruction *pRiver, rev::DWORD dwInstrCount, RelocableCodeBuffer &px86, rev::DWORD flg, rev::DWORD &instrCounter, rev::DWORD &byteCounter, rev::BYTE outputType);
	//bool AssembleTracking(RiverInstruction *pRiver, rev::DWORD dwInstrCount, RelocableCodeBuffer &px86, rev::DWORD flg, rev::DWORD &instrCounter, rev::DWORD &byteCounter);
};



#endif