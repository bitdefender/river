#ifndef _TRACKING_X86_ASSEMBLER_
#define _TRACKING_X86_ASSEMBLER_

#include "GenericX86Assembler.h"

class TrackingX86Assembler : public GenericX86Assembler {
private :
	void AssembleTrackFlag(DWORD testFlags, RelocableCodeBuffer &px86, DWORD &pFlags, DWORD &instrCounter);
	void AssembleMarkFlag(DWORD testFlags, RelocableCodeBuffer &px86, DWORD &pFlags, DWORD &instrCounter);

	void AssembleTrackRegister(const RiverRegister &reg, RelocableCodeBuffer &px86, DWORD &pFlags, DWORD &instrCounter);
	void AssembleMarkRegister(const RiverRegister &reg, RelocableCodeBuffer &px86, DWORD &pFlags, DWORD &instrCounter);

	void AssembleTrackMemory(const RiverAddress *addr, BYTE offset, RelocableCodeBuffer &px86, DWORD &pFlags, DWORD &instrCounter);
	void AssembleMarkMemory(const RiverAddress *addr, BYTE offset, RelocableCodeBuffer &px86, DWORD &pFlags, DWORD &instrCounter);

	void AssembleTrackAddress(const RiverAddress *addr, RelocableCodeBuffer &px86, DWORD &pFlags, DWORD &instrCounter);

	void AssembleAdjustESI(BYTE count, RelocableCodeBuffer &px86, DWORD &pFlags, DWORD &instrCounter);
	void AssembleSetZero(BYTE reg, RelocableCodeBuffer &px86, DWORD &pFlags, DWORD &instrCounter);

	DWORD GetOperandTrackSize(const RiverInstruction &ri, BYTE idx);
public :
	virtual bool Translate(const RiverInstruction &ri, RelocableCodeBuffer &px86, DWORD &pFlags, BYTE &currentFamily, BYTE &repReg, DWORD &instrCounter);
};


#endif
