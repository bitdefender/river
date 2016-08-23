#ifndef _RIVER_TRACKING_X86_ASSEMBLER_H_
#define _RIVER_TRACKING_X86_ASSEMBLER_H_

#include "GenericX86Assembler.h"

class RiverTrackingX86Assembler : public GenericX86Assembler {
private:
	void AssemblePushFlg(rev::DWORD testFlags, RelocableCodeBuffer &px86, rev::DWORD &pFlags, rev::DWORD &instrCounter);
	void AssemblePushReg(const RiverRegister &reg, RelocableCodeBuffer &px86, rev::DWORD &pFlags, rev::DWORD &instrCounter);
	void AssemblePushMem(const RiverAddress *addr, rev::BYTE offset, RelocableCodeBuffer &px86, rev::DWORD &pFlags, rev::DWORD &instrCounter);

	void AssemblePopFlg(rev::DWORD testFlags, RelocableCodeBuffer &px86, rev::DWORD &pFlags, rev::DWORD &instrCounter);
	void AssemblePopReg(const RiverRegister &reg, RelocableCodeBuffer &px86, rev::DWORD &pFlags, rev::DWORD &instrCounter);
	void AssemblePopMem(const RiverAddress *addr, rev::BYTE offset, RelocableCodeBuffer &px86, rev::DWORD &pFlags, rev::DWORD &instrCounter);

	void AssembleUnmark(RelocableCodeBuffer &px86, rev::DWORD &pFlags, rev::DWORD &instrCounter);
public:
	virtual bool Translate(const RiverInstruction &ri, RelocableCodeBuffer &px86, rev::DWORD &pFlags, rev::BYTE &currentFamily, rev::BYTE &repReg, rev::DWORD &instrCounter, rev::BYTE outputType);
};

#endif