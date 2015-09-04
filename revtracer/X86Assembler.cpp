#include "X86Assembler.h"

#include "mm.h"

#define FLAG_GENERATE_RIVER_xSP		0x0F
/*#define FLAG_GENERATE_RIVER_xSP_xAX	0x01
#define FLAG_GENERATE_RIVER_xSP_xCX	0x02
#define FLAG_GENERATE_RIVER_xSP_xDX	0x04
#define FLAG_GENERATE_RIVER_xSP_xBX	0x08*/

#define FLAG_SKIP_METAOP			0x10
#define FLAG_GENERATE_RIVER			0x20

static void FixRiverEspOp(BYTE opType, RiverOperand *op, BYTE repReg) {
	switch (RIVER_OPTYPE(opType)) {
	case RIVER_OPTYPE_IMM:
	case RIVER_OPTYPE_NONE:
		break;
	case RIVER_OPTYPE_REG:
		if (op->asRegister.name == RIVER_REG_xSP) {
			op->asRegister.versioned = repReg;
		}
		break;
	case RIVER_OPTYPE_MEM:
		break;
	default:
		__asm int 3;
	}
}

const RiverInstruction *FixRiverEspInstruction(const RiverInstruction &rIn, RiverInstruction *rTmp, RiverAddress *aTmp) {
	if (rIn.family & RIVER_FAMILY_FLAG_ORIG_xSP) {
		BYTE repReg = rIn.GetUnusedRegister();

		memcpy(rTmp, &rIn, sizeof(*rTmp));
		if (rIn.opTypes[0] != RIVER_OPTYPE_NONE) {
			if (RIVER_OPTYPE(rIn.opTypes[0]) == RIVER_OPTYPE_MEM) {
				rTmp->operands[0].asAddress = aTmp;
				memcpy(aTmp, rIn.operands[0].asAddress, sizeof(*aTmp));
			}
			FixRiverEspOp(rTmp->opTypes[0], &rTmp->operands[0], repReg);
		}

		if (rIn.opTypes[1] != RIVER_OPTYPE_NONE) {
			if (RIVER_OPTYPE(rIn.opTypes[1]) == RIVER_OPTYPE_MEM) {
				rTmp->operands[1].asAddress = aTmp;
				memcpy(aTmp, rIn.operands[1].asAddress, sizeof(*aTmp));
			}
			FixRiverEspOp(rTmp->opTypes[1], &rTmp->operands[1], repReg);
		}
		return rTmp;
	}
	else {
		return &rIn;
	}
}

void X86Assembler::SwitchToRiver(BYTE *&px86, DWORD &instrCounter) {
	static const unsigned char code[] = { 0x87, 0x25, 0x00, 0x00, 0x00, 0x00 };			// 0x00 - xchg esp, large ds:<dwVirtualStack>}

	memcpy(px86, code, sizeof(code));
	*(unsigned int *)(&(px86[0x02])) = (unsigned int)&runtime->execBuff;

	px86 += sizeof(code);
	instrCounter++;
}

void X86Assembler::SwitchToRiverEsp(BYTE *&px86, DWORD &instrCounter, BYTE repReg) {
	static const unsigned char code[] = { 0x87, 0x05, 0x00, 0x00, 0x00, 0x00 };			// 0x00 - xchg eax, large ds:<dwVirtualStack>}

	memcpy(px86, code, sizeof(code));
	px86[0x01] |= (repReg & 0x03); // choose the acording replacement register
	*(unsigned int *)(&(px86[0x02])) = (unsigned int)&runtime->execBuff;

	px86 += sizeof(code);
	instrCounter++;
}

void RiverX86Assembler::EndRiverConversion(BYTE *&px86, DWORD &pFlags, BYTE &repReg, DWORD &instrCounter) {
	if (pFlags & FLAG_GENERATE_RIVER) {
		if (pFlags & FLAG_GENERATE_RIVER_xSP) {
			SwitchToRiverEsp(px86, instrCounter, repReg);
			pFlags &= ~FLAG_GENERATE_RIVER_xSP;
			repReg = 0;
		}

		SwitchToRiver(px86, instrCounter);
		pFlags &= ~FLAG_GENERATE_RIVER;
	}
}

bool X86Assembler::Translate(const RiverInstruction &ri, RelocableCodeBuffer &px86, DWORD &pFlags, BYTE &repReg, DWORD &instrCounter) {
	// skip ignored instructions
	if (ri.family & RIVER_FAMILY_FLAG_IGNORE) {
		return true;
	}

	if (/*(RIVER_FAMILY(ri.family) == RIVER_FAMILY_TRACK) || */(RIVER_FAMILY(ri.family) == RIVER_FAMILY_PRETRACK)) {
		return true;
	}

	// when generating fwcode skip meta instructions
	if ((RIVER_FAMILY(ri.family) == RIVER_FAMILY_PREMETAOP) || (RIVER_FAMILY(ri.family) == RIVER_FAMILY_POSTMETAOP)) {
		if (pFlags & FLAG_SKIP_METAOP) {
			return true;
		}
	}

	GenerateTransitions(ri, px86.cursor, pFlags, repReg, instrCounter);
	GeneratePrefixes(ri, px86.cursor);

	RiverInstruction rInstr;
	RiverAddress32 rAddr;

	const RiverInstruction *rOut = FixRiverEspInstruction(ri, &rInstr, &rAddr);

	DWORD dwTable = 0;

	/*if (rOut->modifiers & RIVER_MODIFIER_EXT) {
		*px86.cursor = 0x0F;
		px86.cursor++;
		dwTable = 1;
	}*/

	//(this->*assembleOpcodes[dwTable][rOut->opCode])(*rOut, px86, pFlags, instrCounter);
	//(this->*assembleOperands[dwTable][rOut->opCode])(*rOut, px86);

	GenericX86Assembler *casm = &nAsm;

	if (RIVER_FAMILY(ri.family) == RIVER_FAMILY_RIVER) {
		casm = &rAsm;
	} else if (RIVER_FAMILY(ri.family) == RIVER_FAMILY_TRACK) {
		casm = &tAsm; 
	} else if (RIVER_FAMILY(ri.family) == RIVER_FAMILY_PRETRACK) {
		casm = &ptAsm; 
	}

	casm->Translate(*rOut, px86, pFlags, repReg, instrCounter);


	return true;
}


bool X86Assembler::Assemble(RiverInstruction *pRiver, DWORD dwInstrCount, RelocableCodeBuffer &px86, DWORD flg, DWORD &instrCounter, DWORD &byteCounter) {
	BYTE *pTmp = px86.cursor, *pAux = px86.cursor;
	DWORD pFlags = flg;
	BYTE repReg = 0;

	DbgPrint("= river to x86 ================================================================\n");

	for (DWORD i = 0; i < dwInstrCount; ++i) {
		pTmp = px86.cursor;

		//pFlags = flg;
		if (!Translate(pRiver[i], px86, pFlags, repReg, instrCounter)) {
			return false;
		}
		//ConvertRiverInstruction(cg, rt, &pRiver[i], &pTmp, &pFlags);

		for (; pTmp < px86.cursor; ++pTmp) {
			DbgPrint("%02x ", *pTmp);
		}
		DbgPrint("\n");
	}

	EndRiverConversion(px86.cursor, pFlags, repReg, instrCounter);
	for (; pTmp < px86.cursor; ++pTmp) {
		DbgPrint("%02x ", *pTmp);
	}
	DbgPrint("\n");

	DbgPrint("===============================================================================\n");
	byteCounter = px86.cursor - pAux;
	return true;
}

void X86Assembler::EndRiverConversion(BYTE *&px86, DWORD &pFlags, BYTE &repReg, DWORD &instrCounter) {
	if (pFlags & FLAG_GENERATE_RIVER) {
		if (pFlags & FLAG_GENERATE_RIVER_xSP) {
			SwitchToRiverEsp(px86, instrCounter, repReg);
			pFlags &= ~FLAG_GENERATE_RIVER_xSP;
			repReg = 0;
		}

		SwitchToRiver(px86, instrCounter);
		pFlags &= ~FLAG_GENERATE_RIVER;
	}
}

bool X86Assembler::GenerateTransitions(const RiverInstruction &ri, BYTE *&px86, DWORD &pFlags, BYTE &repReg, DWORD &instrCounter) {
	//FIXME skip all symbop instructions
	if (RIVER_FAMILY(ri.family) == RIVER_FAMILY_PRETRACK) {
		return true;
	}

	if (RIVER_FAMILY(ri.family) == RIVER_FAMILY_TRACK) {
		return true;
	}

	// ensure state transitions between river and x86
	if (RIVER_FAMILY(ri.family) == RIVER_FAMILY_RIVER) { // current instr is river
		if (0 == (pFlags & FLAG_GENERATE_RIVER)) {
			SwitchToRiver(px86, instrCounter);
			pFlags |= FLAG_GENERATE_RIVER;
		}

		// ensure state transitions between riveresp and river
		if (ri.family & RIVER_FAMILY_FLAG_ORIG_xSP) { // ri needs riveresp
			if (0 == (pFlags & FLAG_GENERATE_RIVER_xSP)) {
				repReg = ri.GetUnusedRegister();
				SwitchToRiverEsp(px86, instrCounter, repReg);
				pFlags |= FLAG_GENERATE_RIVER_xSP;
			}
			else {
				if (0 == ((1 << repReg) & ri.unusedRegisters)) { // switch repReg
					SwitchToRiverEsp(px86, instrCounter, repReg);
					repReg = ri.GetUnusedRegister();
					SwitchToRiverEsp(px86, instrCounter, repReg);
				}
			}
		}
		else {
			if (pFlags & FLAG_GENERATE_RIVER_xSP) {
				SwitchToRiverEsp(px86, instrCounter, repReg);
				pFlags &= ~FLAG_GENERATE_RIVER_xSP;
			}
		}
	}
	else { // current instr is x86
		if (pFlags & FLAG_GENERATE_RIVER) {
			if (pFlags & FLAG_GENERATE_RIVER_xSP) {
				SwitchToRiverEsp(px86, instrCounter, repReg);
				repReg = 0;
				pFlags &= ~FLAG_GENERATE_RIVER_xSP;
			}

			SwitchToRiver(px86, instrCounter);
			pFlags &= ~FLAG_GENERATE_RIVER;
		}
	}

	return true;
}

bool X86Assembler::Init(RiverRuntime *runtime) {
	this->runtime = runtime;

	nAsm.Init(runtime);
	rAsm.Init(runtime);
	ptAsm.Init(runtime);
	tAsm.Init(runtime);

	return true;
}
