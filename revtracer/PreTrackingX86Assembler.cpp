#include "PreTrackingX86Assembler.h"

#include "X86AssemblerFuncs.h"

void PreTrackingAssembler::AssemblePreTrackMem(RiverAddress *addr, BYTE riverFamily, RelocableCodeBuffer &px86, DWORD &instrCounter) {
	const BYTE regByte[] = { 0x05, 0x0D, 0x15, 0x1D };

	const BYTE preTrackMemPrefix[] = {
		0x89, 0x05, 0x00, 0x00, 0x00, 0x00
	};

	const BYTE preTrackMemSuffix[] = {
		0x8B, 0x05, 0x00, 0x00, 0x00, 0x00
	};

	BYTE unusedRegisters = addr->GetUnusedRegisters();
	BYTE cReg;

	if (unusedRegisters & 0x03) {
		if (unusedRegisters & 0x01) {
			cReg = RIVER_REG_xAX;
		}
		cReg = RIVER_REG_xCX;
	}
	else if (unusedRegisters & 0x0C) {
		if (unusedRegisters & 0x04) {
			cReg = RIVER_REG_xDX;
		}
		cReg = RIVER_REG_xBX;
	}
	else {
		__asm int 3;
	}

	memcpy(px86.cursor, preTrackMemPrefix, sizeof(preTrackMemPrefix));
	px86.cursor[1] = regByte[cReg];
	*(DWORD *)(&px86.cursor[2]) = (DWORD)&runtime->returnRegister;
	px86.cursor += sizeof(preTrackMemPrefix);
	instrCounter++;

	RiverInstruction rLea;
	rLea.opCode = 0x8D;
	rLea.modifiers = rLea.specifiers = 0;
	rLea.family = RIVER_FAMILY_PRETRACK;
	rLea.opTypes[0] = RIVER_OPTYPE_REG;
	rLea.operands[0].asRegister.versioned = cReg;
	
	rLea.opTypes[1] = RIVER_OPTYPE_MEM;
	rLea.operands[1].asAddress = addr;

	rLea.opTypes[2] = rLea.opTypes[3] = RIVER_OPTYPE_NONE;
	rLea.PromoteModifiers();
	rLea.TrackEspAsParameter();
	rLea.TrackUnusedRegisters();

	DWORD flags = 0;
	GeneratePrefixes(rLea, px86.cursor);
	AssembleDefaultInstr(rLea, px86, flags, instrCounter);
	AssembleRegModRMOp(rLea, px86);
	

	/*px86.cursor[0] = 0x8D;
	px86.cursor++;
	addr->EncodeTox86(px86.cursor, cReg, riverFamily, 0);
	instrCounter++;*/

	px86.cursor[0] = 0x50 + cReg; // push reg;
	px86.cursor++;
	instrCounter++;

	memcpy(px86.cursor, preTrackMemSuffix, sizeof(preTrackMemSuffix));
	px86.cursor[1] = regByte[cReg];
	*(DWORD *)(&px86.cursor[2]) = (DWORD)&runtime->returnRegister;
	px86.cursor += sizeof(preTrackMemSuffix);
	instrCounter++;
}

bool PreTrackingAssembler::Translate(const RiverInstruction &ri, RelocableCodeBuffer &px86, DWORD &pFlags, BYTE &currentFamily, BYTE &repReg, DWORD &instrCounter) {
	switch (ri.opCode) {
		case 0x9C :
			::AssembleDefaultInstr(ri, px86, pFlags, instrCounter);
			::AssembleNoOp(ri, px86);
			instrCounter++;
			break;

		case 0x50 :
			::AssemblePlusRegInstr(ri, px86, pFlags, instrCounter);
			instrCounter++;
			break;

		case 0x8D :
			ClearPrefixes(ri, px86.cursor);
			AssemblePreTrackMem(ri.operands[0].asAddress, ri.family, px86, instrCounter);
			break;

		case 0xFF :
			if (6 == ri.subOpCode) {
				::GeneratePrefixes(ri, px86.cursor);
				::AssembleDefaultInstr(ri, px86, pFlags, instrCounter);
				::AssembleModRMOp<6>(ri, px86);
				instrCounter++;
				break;
			} else {
				__asm int 3;
			}

		default :
			__asm int 3;
	}

	return true;
}