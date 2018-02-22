#include "PreTrackingX86Assembler.h"

#include "X86AssemblerFuncs.h"

using namespace rev;

nodep::BYTE LogB2(nodep::BYTE x) {
	nodep::BYTE r = 0;

	if (x & 0xF0) { r += 4; x >>= 4; }
	if (x & 0x0C) { r += 2; x >>= 2; }
	if (x & 0x02) { r += 1; x >>= 1; }

	return r;
}


nodep::BYTE SelectUnusedRegister(nodep::BYTE &uIn) {
	nodep::BYTE reg = uIn;
	reg &= -reg;
	uIn &= ~reg;

	return LogB2(reg);
}

const nodep::BYTE PreTrackingAssembler::regByte[] = { 0x05, 0x0D, 0x15, 0x1D, 0x25, 0x2D, 0x35, 0x3D };

void PreTrackingAssembler::SaveUnusedRegister(nodep::BYTE reg, RelocableCodeBuffer &px86, int idx) {
	const nodep::BYTE preTrackMemPrefix[] = {
		0x89, 0x05, 0x00, 0x00, 0x00, 0x00
	};

	nodep::DWORD dest = (nodep::DWORD)&runtime->returnRegister;
	if (idx) {
		dest = (nodep::DWORD)&runtime->secondaryRegister;
	}

	rev_memcpy(px86.cursor, preTrackMemPrefix, sizeof(preTrackMemPrefix));
	px86.cursor[1] = regByte[reg];
	*(nodep::DWORD *)(&px86.cursor[2]) = dest;
	px86.cursor += sizeof(preTrackMemPrefix);

}

void PreTrackingAssembler::RestoreUnusedRegister(nodep::BYTE reg, RelocableCodeBuffer &px86, int idx) {
	const nodep::BYTE preTrackMemSuffix[] = {
		0x8B, 0x05, 0x00, 0x00, 0x00, 0x00
	};

	nodep::DWORD dest = (nodep::DWORD)&runtime->returnRegister;
	if (idx) {
		dest = (nodep::DWORD)&runtime->secondaryRegister;
	}

	rev_memcpy(px86.cursor, preTrackMemSuffix, sizeof(preTrackMemSuffix));
	px86.cursor[1] = regByte[reg];
	*(nodep::DWORD *)(&px86.cursor[2]) = dest;
	px86.cursor += sizeof(preTrackMemSuffix);
}

void PreTrackingAssembler::AssemblePreTrackAddr(RiverAddress *addr, nodep::BYTE riverFamily, nodep::BYTE repReg, RelocableCodeBuffer &px86, nodep::DWORD &instrCounter) {

	nodep::DWORD flags = 0;
	nodep::BYTE unusedRegisters = addr->GetUnusedRegisters() & 0xCF;

	if (riverFamily & RIVER_FAMILY_FLAG_ORIG_xSP) {
		unusedRegisters &= ~(1 << repReg);
	}

	nodep::BYTE cReg = SelectUnusedRegister(unusedRegisters);

	SaveUnusedRegister(cReg, px86, 0);
	instrCounter++;

	{   // wrap up in RAII - lea <cReg>, <address>
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

		GeneratePrefixes(rLea, px86.cursor);
		AssembleDefaultInstr(rLea, px86, flags, instrCounter);
		AssembleRegModRMOp(rLea, px86);
	}

	if (addr->HasSegment()) {
		nodep::BYTE sReg = SelectUnusedRegister(unusedRegisters);

		if (riverFamily & RIVER_FAMILY_FLAG_ORIG_xSP) {
			unusedRegisters &= ~(1 << repReg);
		}

		// save reg
		SaveUnusedRegister(sReg, px86, 1);
		instrCounter++;

		{	// wrap up in RAII - mov <sReg>, segment
			RiverInstruction rOps;
			RiverAddress32 rAddr;
			rOps.opCode = 0x8C;
			rOps.modifiers = rOps.specifiers = 0;
			rOps.family = RIVER_FAMILY_PRETRACK;

			rOps.opTypes[0] = RIVER_OPTYPE_REG;
			rOps.operands[0].asRegister.versioned = addr->GetSegment() - 1; //sReg;

			rOps.opTypes[1] = RIVER_OPTYPE_MEM;
			rOps.operands[1].asAddress = &rAddr;

			rAddr.type = RIVER_ADDR_DIRTY;
			rAddr.scaleAndSegment = 0;
			rAddr.index.versioned = 0;
			rAddr.base.versioned = 0;
			rAddr.base.name = sReg; //addr->GetSegment() - 1;
			rAddr.disp.d32 = 0;

			rOps.opTypes[2] = rOps.opTypes[3] = RIVER_OPTYPE_NONE;
			rOps.PromoteModifiers();
			rOps.TrackEspAsParameter();
			rOps.TrackUnusedRegisters();

			GeneratePrefixes(rOps, px86.cursor);
			AssembleDefaultInstr(rOps, px86, flags, instrCounter);
			AssembleRegModRMOp(rOps, px86);
		}

		{	// wrap up in RAII - lea <sReg>, [sReg * 4 + segmentOffsets]
			RiverInstruction rOps;
			RiverAddress32 rAddr;
			rOps.opCode = 0x8D;
			rOps.modifiers = rOps.specifiers = 0;
			rOps.family = RIVER_FAMILY_PRETRACK;

			rOps.opTypes[0] = RIVER_OPTYPE_REG;
			rOps.operands[0].asRegister.versioned = sReg;

			rOps.opTypes[1] = RIVER_OPTYPE_MEM;
			rOps.operands[1].asAddress = &rAddr;

			rAddr.type = RIVER_ADDR_SCALE | RIVER_ADDR_INDEX | RIVER_ADDR_DISP | RIVER_ADDR_DIRTY;
			rAddr.scaleAndSegment = 0;
			rAddr.SetScale(4);
			rAddr.index.versioned = sReg;
			rAddr.base.versioned = 0;
			rAddr.disp.d32 = (nodep::DWORD)revtracerConfig.segmentOffsets;

			rOps.opTypes[2] = rOps.opTypes[3] = RIVER_OPTYPE_NONE;
			rOps.PromoteModifiers();
			rOps.TrackEspAsParameter();
			rOps.TrackUnusedRegisters();

			GeneratePrefixes(rOps, px86.cursor);
			AssembleDefaultInstr(rOps, px86, flags, instrCounter);
			AssembleRegModRMOp(rOps, px86);
		}

		{	// wrap up in RAII - lea <cReg>, [cReg + sReg]
			RiverInstruction rOps;
			RiverAddress32 rAddr;
			rOps.opCode = 0x8D;
			rOps.modifiers = rOps.specifiers = 0;
			rOps.family = RIVER_FAMILY_PRETRACK;

			rOps.opTypes[0] = RIVER_OPTYPE_REG;
			rOps.operands[0].asRegister.versioned = cReg;

			rOps.opTypes[1] = RIVER_OPTYPE_MEM;
			rOps.operands[1].asAddress = &rAddr;

			rAddr.type = RIVER_ADDR_SCALE | RIVER_ADDR_INDEX | RIVER_ADDR_BASE | RIVER_ADDR_DIRTY;
			rAddr.scaleAndSegment = 0;
			rAddr.SetScale(1);
			rAddr.index.versioned = sReg;
			rAddr.base.versioned = cReg;
			rAddr.disp.d32 = 0;

			rOps.opTypes[2] = rOps.opTypes[3] = RIVER_OPTYPE_NONE;
			rOps.PromoteModifiers();
			rOps.TrackEspAsParameter();
			rOps.TrackUnusedRegisters();

			GeneratePrefixes(rOps, px86.cursor);
			AssembleDefaultInstr(rOps, px86, flags, instrCounter);
			AssembleRegModRMOp(rOps, px86);
		}

		// restore reg
		RestoreUnusedRegister(sReg, px86, 1);
		instrCounter++;
	}

	px86.cursor[0] = 0x50 + cReg; // push reg;
	px86.cursor++;
	instrCounter++;

	if (addr->type & RIVER_ADDR_BASE) {
		nodep::BYTE reg = GetFundamentalRegister(addr->base.name);

		if ((riverFamily & RIVER_FAMILY_FLAG_ORIG_xSP) && (reg == RIVER_REG_xSP)) {
			reg = repReg;
		}

		px86.cursor[0] = 0x50 + GetFundamentalRegister(reg); // push reg;
		px86.cursor++;
		instrCounter++;
	}

	if (addr->type & RIVER_ADDR_INDEX) {
		nodep::BYTE reg = GetFundamentalRegister(addr->index.name);

		if ((riverFamily & RIVER_FAMILY_FLAG_ORIG_xSP) && (reg == RIVER_REG_xSP)) {
			reg = repReg;
		}
		
		px86.cursor[0] = 0x50 + GetFundamentalRegister(reg); // push reg;
		px86.cursor++;
		instrCounter++;
	}

	RestoreUnusedRegister(cReg, px86, 0);
	instrCounter++;
}

void PreTrackingAssembler::AssemblePreTrackMem(RiverAddress *addr, nodep::BYTE riverFamily, nodep::BYTE repReg, RelocableCodeBuffer &px86, nodep::DWORD &instrCounter) {

	const nodep::BYTE andRegVal[] = { 0x9C, 0x83, 0xE0, 0xFC, 0x9D };
	const nodep::BYTE pushEax4[] = { 0xFF, 0x70, 0x04 };
	const nodep::BYTE pushEax[] = { 0xFF, 0x30 };
	const nodep::BYTE segmentPrefix[] = { 0x00, 0x26, 0x2E, 0x36, 0x3E, 0x64, 0x65 };

	nodep::DWORD flags = 0;
	nodep::BYTE unusedRegisters = addr->GetUnusedRegisters() & 0xCF;

	if (riverFamily & RIVER_FAMILY_FLAG_ORIG_xSP) {
		unusedRegisters &= ~(1 << repReg);
	}

	nodep::BYTE cReg = SelectUnusedRegister(unusedRegisters);

	SaveUnusedRegister(cReg, px86, 0);
	instrCounter++;

	{   // wrap up in RAII - lea <cReg>, <address>
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

		GeneratePrefixes(rLea, px86.cursor);
		AssembleDefaultInstr(rLea, px86, flags, instrCounter);
		AssembleRegModRMOp(rLea, px86);
	}

	if (addr->HasSegment()) {
		nodep::BYTE sReg = SelectUnusedRegister(unusedRegisters);

		if (riverFamily & RIVER_FAMILY_FLAG_ORIG_xSP) {
			unusedRegisters &= ~(1 << repReg);
		}

		// save reg
		SaveUnusedRegister(sReg, px86, 1);
		instrCounter++;

		{	// wrap up in RAII - mov <sReg>, segment
			RiverInstruction rOps;
			RiverAddress32 rAddr;
			rOps.opCode = 0x8C;
			rOps.modifiers = rOps.specifiers = 0;
			rOps.family = RIVER_FAMILY_PRETRACK;

			rOps.opTypes[0] = RIVER_OPTYPE_REG;
			rOps.operands[0].asRegister.versioned = addr->GetSegment() - 1; //sReg;

			rOps.opTypes[1] = RIVER_OPTYPE_MEM;
			rOps.operands[1].asAddress = &rAddr;

			rAddr.type = RIVER_ADDR_DIRTY;
			rAddr.scaleAndSegment = 0;
			rAddr.index.versioned = 0;
			rAddr.base.versioned = 0;
			rAddr.base.name = sReg; //addr->GetSegment() - 1;
			rAddr.disp.d32 = 0;

			rOps.opTypes[2] = rOps.opTypes[3] = RIVER_OPTYPE_NONE;
			rOps.PromoteModifiers();
			rOps.TrackEspAsParameter();
			rOps.TrackUnusedRegisters();

			GeneratePrefixes(rOps, px86.cursor);
			AssembleDefaultInstr(rOps, px86, flags, instrCounter);
			AssembleRegModRMOp(rOps, px86);
		}

		{	// wrap up in RAII - lea <sReg>, [sReg * 4 + segmentOffsets]
			RiverInstruction rOps;
			RiverAddress32 rAddr;
			rOps.opCode = 0x8D;
			rOps.modifiers = rOps.specifiers = 0;
			rOps.family = RIVER_FAMILY_PRETRACK;

			rOps.opTypes[0] = RIVER_OPTYPE_REG;
			rOps.operands[0].asRegister.versioned = sReg;

			rOps.opTypes[1] = RIVER_OPTYPE_MEM;
			rOps.operands[1].asAddress = &rAddr;

			rAddr.type = RIVER_ADDR_SCALE | RIVER_ADDR_INDEX | RIVER_ADDR_DISP | RIVER_ADDR_DIRTY;
			rAddr.scaleAndSegment = 0;
			rAddr.SetScale(4);
			rAddr.index.versioned = sReg;
			rAddr.base.versioned = 0;
			rAddr.disp.d32 = (nodep::DWORD)revtracerConfig.segmentOffsets;

			rOps.opTypes[2] = rOps.opTypes[3] = RIVER_OPTYPE_NONE;
			rOps.PromoteModifiers();
			rOps.TrackEspAsParameter();
			rOps.TrackUnusedRegisters();

			GeneratePrefixes(rOps, px86.cursor);
			AssembleDefaultInstr(rOps, px86, flags, instrCounter);
			AssembleRegModRMOp(rOps, px86);
		}

		{	// wrap up in RAII - lea <cReg>, [cReg + sReg]
			RiverInstruction rOps;
			RiverAddress32 rAddr;
			rOps.opCode = 0x8D;
			rOps.modifiers = rOps.specifiers = 0;
			rOps.family = RIVER_FAMILY_PRETRACK;

			rOps.opTypes[0] = RIVER_OPTYPE_REG;
			rOps.operands[0].asRegister.versioned = cReg;

			rOps.opTypes[1] = RIVER_OPTYPE_MEM;
			rOps.operands[1].asAddress = &rAddr;

			rAddr.type = RIVER_ADDR_SCALE | RIVER_ADDR_INDEX | RIVER_ADDR_BASE | RIVER_ADDR_DIRTY;
			rAddr.scaleAndSegment = 0;
			rAddr.SetScale(1);
			rAddr.index.versioned = sReg;
			rAddr.base.versioned = cReg;
			rAddr.disp.d32 = 0;

			rOps.opTypes[2] = rOps.opTypes[3] = RIVER_OPTYPE_NONE;
			rOps.PromoteModifiers();
			rOps.TrackEspAsParameter();
			rOps.TrackUnusedRegisters();

			GeneratePrefixes(rOps, px86.cursor);
			AssembleDefaultInstr(rOps, px86, flags, instrCounter);
			AssembleRegModRMOp(rOps, px86);
		}

		// restore reg
		RestoreUnusedRegister(sReg, px86, 1);
		instrCounter++;
	}

	rev_memcpy(px86.cursor, andRegVal, sizeof(andRegVal));
	px86.cursor[2] += cReg;
	px86.cursor += sizeof(andRegVal);

	rev_memcpy(px86.cursor, pushEax4, sizeof(pushEax4));
	px86.cursor[1] += cReg;
	px86.cursor += sizeof(pushEax4);

	rev_memcpy(px86.cursor, pushEax, sizeof(pushEax));
	px86.cursor[1] += cReg;
	px86.cursor += sizeof(pushEax);

	instrCounter += 5;

	RestoreUnusedRegister(cReg, px86, 0);
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
			AssemblePreTrackAddr(ri.operands[0].asAddress, ri.family, repReg, px86, instrCounter);
			break;

		case 0xFF :
			if (6 == ri.subOpCode) {
				ClearPrefixes(ri, px86.cursor);
				AssemblePreTrackMem(ri.operands[0].asAddress, ri.family, repReg, px86, instrCounter);
				break;
			} else {
				DEBUG_BREAK;
			}

		default :
			DEBUG_BREAK;
	}

	return true;
}
