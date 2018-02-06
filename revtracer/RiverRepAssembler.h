#ifndef _RIVER_REP_ASSEMBLER_H
#define _RIVER_REP_ASSEMBLER_H

#include "GenericX86Assembler.h"

class RiverRepAssembler : public GenericX86Assembler {
	public:
		virtual bool Translate(const RiverInstruction &ri, RelocableCodeBuffer &px86,  nodep::DWORD &pFlags, nodep::BYTE &currentFamily, nodep::BYTE &repReg, nodep::DWORD &instrCounter, nodep::BYTE outputType);
};

#endif
