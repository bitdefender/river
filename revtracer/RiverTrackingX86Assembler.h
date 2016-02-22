#ifndef _RIVER_TRACKING_X86_ASSEMBLER_H_
#define _RIVER_TRACKING_X86_ASSEMBLER_H_

#include "GenericX86Assembler.h"

class RiverTrackingX86Assembler : public GenericX86Assembler {
private:
	void AssemblePushFlg(DWORD testFlags, RelocableCodeBuffer &px86, DWORD &pFlags, DWORD &instrCounter);
	void AssemblePushReg(const RiverRegister &reg, RelocableCodeBuffer &px86, DWORD &pFlags, DWORD &instrCounter);
	void AssemblePushMem(const RiverAddress *addr, BYTE offset, RelocableCodeBuffer &px86, DWORD &pFlags, DWORD &instrCounter);

	void AssemblePopFlg(DWORD testFlags, RelocableCodeBuffer &px86, DWORD &pFlags, DWORD &instrCounter);
	void AssemblePopReg(const RiverRegister &reg, RelocableCodeBuffer &px86, DWORD &pFlags, DWORD &instrCounter);
	void AssemblePopMem(const RiverAddress *addr, BYTE offset, RelocableCodeBuffer &px86, DWORD &pFlags, DWORD &instrCounter);

	void AssembleUnmark(RelocableCodeBuffer &px86, DWORD &pFlags, DWORD &instrCounter);
public:
	virtual bool Translate(const RiverInstruction &ri, RelocableCodeBuffer &px86, DWORD &pFlags, BYTE &currentFamily, BYTE &repReg, DWORD &instrCounter);
};

#endif