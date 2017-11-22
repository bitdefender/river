#include "PreTrackingX86Assembler.h"

#include "X86AssemblerFuncs.h"

using namespace rev;

void PreTrackingAssembler::AssemblePreTrackMem(RiverAddress *addr, bool saveVal, nodep::BYTE riverFamily, RelocableCodeBuffer &px86, nodep::DWORD &instrCounter) {
	const nodep::BYTE regByte[] = { 0x05, 0x0D, 0x15, 0x1D };

	const nodep::BYTE preTrackMemPrefix[] = {
		0x89, 0x05, 0x00, 0x00, 0x00, 0x00
	};

	const nodep::BYTE preTrackMemSuffix[] = {
		0x8B, 0x05, 0x00, 0x00, 0x00, 0x00
	};

	nodep::BYTE unusedRegisters = addr->GetUnusedRegisters();
	nodep::BYTE cReg;

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
		DEBUG_BREAK;
	}

	rev_memcpy(px86.cursor, preTrackMemPrefix, sizeof(preTrackMemPrefix));
	px86.cursor[1] = regByte[cReg];
	*(nodep::DWORD *)(&px86.cursor[2]) = (nodep::DWORD)&runtime->returnRegister;
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

	if (addr->HasSegment()) {
		switch (addr->GetSegment()) {
		case RIVER_MODIFIER_ESSEG:
			px86.cursor[0] = 0x06; // push es
			px86.cursor += 1;
			break;
		case RIVER_MODIFIER_CSSEG:
			px86.cursor[0] = 0x0E; // push cs
			px86.cursor += 1;
			break;
		case RIVER_MODIFIER_SSSEG:
			px86.cursor[0] = 0x16; // push ss
			px86.cursor += 1;
			break;
		case RIVER_MODIFIER_DSSEG:
			px86.cursor[0] = 0x1E; // push ds
			px86.cursor += 1;
			break;
		case RIVER_MODIFIER_FSSEG:
			px86.cursor[0] = 0x0F; // push fs
			px86.cursor[1] = 0xA0;
			px86.cursor += 2;
			break;
		case RIVER_MODIFIER_GSSEG:
			px86.cursor[0] = 0x0F; // push gs
			px86.cursor[1] = 0xA8;
			px86.cursor += 2;
			break;
		}
		instrCounter++;
	}

	nodep::DWORD flags = 0;
	GeneratePrefixes(rLea, px86.cursor);
	AssembleDefaultInstr(rLea, px86, flags, instrCounter);
	AssembleRegModRMOp(rLea, px86);
	
	px86.cursor[0] = 0x50 + cReg; // push reg;
	px86.cursor++;
	instrCounter++;

	if (addr->type & RIVER_ADDR_BASE) {
		px86.cursor[0] = 0x50 + GetFundamentalRegister(addr->base.name); // push reg;
		px86.cursor++;
		instrCounter++;
	}

	if (addr->type & RIVER_ADDR_INDEX) {
		px86.cursor[0] = 0x50 + GetFundamentalRegister(addr->index.name); // push reg;
		px86.cursor++;
		instrCounter++;
	}

	if (saveVal) {
		const nodep::BYTE andRegVal[] = { 0x9C, 0x83, 0xE0, 0xFC, 0x9D };
		const nodep::BYTE pushEax4[] = { 0xFF, 0x70, 0x04 };
		const nodep::BYTE pushEax[] = { 0xFF, 0x30 };
		const nodep::BYTE segmentPrefix[] = { 0x00, 0x26, 0x2E, 0x36, 0x3E, 0x64, 0x65 };

		rev_memcpy(px86.cursor, andRegVal, sizeof(andRegVal));
		px86.cursor[2] += cReg;
		px86.cursor += sizeof(andRegVal);

		if (addr->HasSegment()) {
			px86.cursor[0] = segmentPrefix[addr->GetSegment()];
			px86.cursor += 1;
		}
		rev_memcpy(px86.cursor, pushEax4, sizeof(pushEax4));
		px86.cursor[1] += cReg;
		px86.cursor += sizeof(pushEax4);

		if (addr->HasSegment()) {
			px86.cursor[0] = segmentPrefix[addr->GetSegment()];
			px86.cursor += 1;
		}
		rev_memcpy(px86.cursor, pushEax, sizeof(pushEax));
		px86.cursor[1] += cReg;
		px86.cursor += sizeof(pushEax);

		instrCounter += 5;
	}

	rev_memcpy(px86.cursor, preTrackMemSuffix, sizeof(preTrackMemSuffix));
	px86.cursor[1] = regByte[cReg];
	*(nodep::DWORD *)(&px86.cursor[2]) = (nodep::DWORD)&runtime->returnRegister;
	px86.cursor += sizeof(preTrackMemSuffix);
	instrCounter++;
}

bool PreTrackingAssembler::Translate(const RiverInstruction &ri, RelocableCodeBuffer &px86, nodep::DWORD &pFlags, nodep::BYTE &currentFamily, nodep::BYTE &repReg, nodep::DWORD &instrCounter, nodep::BYTE outputType) {
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
			AssemblePreTrackMem(ri.operands[0].asAddress, ri.operands[1].asImm8 == 1, ri.family, px86, instrCounter);
			break;

		/*case 0xFF :
			if (6 == ri.subOpCode) {
				::GeneratePrefixes(ri, px86.cursor);
				::AssembleDefaultInstr(ri, px86, pFlags, instrCounter);
				::AssembleModRMOp<6>(ri, px86);
				instrCounter++;
				break;
			} else {
				DEBUG_BREAK;
			}*/

		default :
			DEBUG_BREAK;
	}

	return true;
}