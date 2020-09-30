#ifndef _TRACKING_X86_ASSEMBLER_
#define _TRACKING_X86_ASSEMBLER_

#include "GenericX86Assembler.h"

class TrackingX86Assembler : public GenericX86Assembler {
private :
	void AssembleTrackFlag(nodep::DWORD testFlags, RelocableCodeBuffer &px86, nodep::DWORD &pFlags, nodep::DWORD &instrCounter);
	void AssembleMarkFlag(nodep::DWORD testFlags, RelocableCodeBuffer &px86, nodep::DWORD &pFlags, nodep::DWORD &instrCounter);

	void AssembleTrackRegister(const RiverRegister &reg, RelocableCodeBuffer &px86, nodep::DWORD &pFlags, nodep::DWORD &instrCounter);
	void AssembleMarkRegister(const RiverRegister &reg, RelocableCodeBuffer &px86, nodep::DWORD &pFlags, nodep::DWORD &instrCounter);

	void AssembleTrackMemory(const RiverAddress *addr, nodep::BYTE offset, RelocableCodeBuffer &px86, nodep::DWORD &pFlags, nodep::DWORD &instrCounter);
	void AssembleMarkMemory(const RiverAddress *addr, nodep::BYTE offset, RelocableCodeBuffer &px86, nodep::DWORD &pFlags, nodep::DWORD &instrCounter);

	void AssembleTrackAddress(const RiverAddress *addr, RelocableCodeBuffer &px86, nodep::DWORD &pFlags, nodep::DWORD &instrCounter);
	void AssembleUnmark(RelocableCodeBuffer &px86, nodep::DWORD &pFlags, nodep::DWORD &instrCounter);

	void AssembleAdjustESI(nodep::BYTE count, RelocableCodeBuffer &px86, nodep::DWORD &pFlags, nodep::DWORD &instrCounter);
	void AssembleSetZero(nodep::BYTE reg, RelocableCodeBuffer &px86, nodep::DWORD &pFlags, nodep::DWORD &instrCounter);

	void AssembleSymbolicCall(nodep::DWORD address, nodep::BYTE index, RelocableCodeBuffer &px86, nodep::DWORD &pFlags, nodep::DWORD &instrCounter);

	nodep::DWORD GetOperandTrackSize(const RiverInstruction &ri, nodep::BYTE idx);

	nodep::DWORD dwTranslationFlags;
public :
	void SetTranslationFlags(nodep::DWORD dwFlags);

	virtual bool Translate(const RiverInstruction &ri, RelocableCodeBuffer &px86, nodep::DWORD &pFlags, nodep::BYTE &currentFamily, nodep::BYTE &repReg, nodep::DWORD &instrCounter, nodep::BYTE outputType);
};


#endif
