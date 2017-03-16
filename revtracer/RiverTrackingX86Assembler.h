#ifndef _RIVER_TRACKING_X86_ASSEMBLER_H_
#define _RIVER_TRACKING_X86_ASSEMBLER_H_

#include "GenericX86Assembler.h"

class RiverTrackingX86Assembler : public GenericX86Assembler {
private:
	void AssemblePushFlg(nodep::DWORD testFlags, RelocableCodeBuffer &px86, nodep::DWORD &pFlags, nodep::DWORD &instrCounter);
	void AssemblePushReg(const RiverRegister &reg, RelocableCodeBuffer &px86, nodep::DWORD &pFlags, nodep::DWORD &instrCounter);
	void AssemblePushMem(const RiverAddress *addr, nodep::BYTE offset, RelocableCodeBuffer &px86, nodep::DWORD &pFlags, nodep::DWORD &instrCounter);

	void AssemblePopFlg(nodep::DWORD testFlags, RelocableCodeBuffer &px86, nodep::DWORD &pFlags, nodep::DWORD &instrCounter);
	void AssemblePopReg(const RiverRegister &reg, RelocableCodeBuffer &px86, nodep::DWORD &pFlags, nodep::DWORD &instrCounter);
	void AssemblePopMem(const RiverAddress *addr, nodep::BYTE offset, RelocableCodeBuffer &px86, nodep::DWORD &pFlags, nodep::DWORD &instrCounter);

	void AssembleUnmark(RelocableCodeBuffer &px86, nodep::DWORD &pFlags, nodep::DWORD &instrCounter);
public:
	virtual bool Translate(const RiverInstruction &ri, RelocableCodeBuffer &px86, nodep::DWORD &pFlags, nodep::BYTE &currentFamily, nodep::BYTE &repReg, nodep::DWORD &instrCounter, nodep::BYTE outputType);
};

#endif