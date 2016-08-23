#ifndef _TRACKING_X86_ASSEMBLER_
#define _TRACKING_X86_ASSEMBLER_

#include "GenericX86Assembler.h"

class TrackingX86Assembler : public GenericX86Assembler {
private :
	void AssembleTrackFlag(rev::DWORD testFlags, RelocableCodeBuffer &px86, rev::DWORD &pFlags, rev::DWORD &instrCounter);
	void AssembleMarkFlag(rev::DWORD testFlags, RelocableCodeBuffer &px86, rev::DWORD &pFlags, rev::DWORD &instrCounter);

	void AssembleTrackRegister(const RiverRegister &reg, RelocableCodeBuffer &px86, rev::DWORD &pFlags, rev::DWORD &instrCounter);
	void AssembleMarkRegister(const RiverRegister &reg, RelocableCodeBuffer &px86, rev::DWORD &pFlags, rev::DWORD &instrCounter);

	void AssembleTrackMemory(const RiverAddress *addr, rev::BYTE offset, RelocableCodeBuffer &px86, rev::DWORD &pFlags, rev::DWORD &instrCounter);
	void AssembleMarkMemory(const RiverAddress *addr, rev::BYTE offset, RelocableCodeBuffer &px86, rev::DWORD &pFlags, rev::DWORD &instrCounter);

	void AssembleTrackAddress(const RiverAddress *addr, RelocableCodeBuffer &px86, rev::DWORD &pFlags, rev::DWORD &instrCounter);
	void AssembleUnmark(RelocableCodeBuffer &px86, rev::DWORD &pFlags, rev::DWORD &instrCounter);

	void AssembleAdjustESI(rev::BYTE count, RelocableCodeBuffer &px86, rev::DWORD &pFlags, rev::DWORD &instrCounter);
	void AssembleSetZero(rev::BYTE reg, RelocableCodeBuffer &px86, rev::DWORD &pFlags, rev::DWORD &instrCounter);

	void AssembleSymbolicCall(rev::DWORD address, rev::BYTE index, RelocableCodeBuffer &px86, rev::DWORD &pFlags, rev::DWORD &instrCounter);

	rev::DWORD GetOperandTrackSize(const RiverInstruction &ri, rev::BYTE idx);

	rev::DWORD dwTranslationFlags;
public :
	void SetTranslationFlags(rev::DWORD dwFlags);

	virtual bool Translate(const RiverInstruction &ri, RelocableCodeBuffer &px86, rev::DWORD &pFlags, rev::BYTE &currentFamily, rev::BYTE &repReg, rev::DWORD &instrCounter, rev::BYTE outputType);
};


#endif
