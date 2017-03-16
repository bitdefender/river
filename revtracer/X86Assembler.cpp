#include "revtracer.h"
#include "common.h"

#include "X86Assembler.h"

#include "mm.h"

using namespace rev;

#define FLAG_GENERATE_RIVER_xSP		0x0F
/*#define FLAG_GENERATE_RIVER_xSP_xAX	0x01
#define FLAG_GENERATE_RIVER_xSP_xCX	0x02
#define FLAG_GENERATE_RIVER_xSP_xDX	0x04
#define FLAG_GENERATE_RIVER_xSP_xBX	0x08*/

#define FLAG_SKIP_METAOP			0x10
//#define FLAG_GENERATE_RIVER			0x20

static void FixRiverEspOp(nodep::BYTE opType, RiverOperand *op, nodep::BYTE repReg) {
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
		DEBUG_BREAK;
	}
}

const RiverInstruction *FixRiverEspInstruction(const RiverInstruction &rIn, RiverInstruction *rTmp, RiverAddress *aTmp) {
	if (rIn.family & RIVER_FAMILY_FLAG_ORIG_xSP) {
		nodep::BYTE repReg = rIn.GetUnusedRegister();

		rev_memcpy(rTmp, &rIn, sizeof(*rTmp));
		for (nodep::BYTE i = 0; i < 4; ++i) {
			if (rIn.opTypes[i] != RIVER_OPTYPE_NONE) {
				if (RIVER_OPTYPE(rIn.opTypes[i]) == RIVER_OPTYPE_MEM) {
					rTmp->operands[i].asAddress = aTmp;
					rev_memcpy(aTmp, rIn.operands[i].asAddress, sizeof(*aTmp));
				}
				FixRiverEspOp(rTmp->opTypes[i], &rTmp->operands[i], repReg);
			}
		}

		return rTmp;
	}
	else {
		return &rIn;
	}
}

void X86Assembler::SwitchToRiver(nodep::BYTE *&px86, nodep::DWORD &instrCounter) {
	static const unsigned char code[] = { 0x87, 0x25, 0x00, 0x00, 0x00, 0x00 };			// 0x00 - xchg esp, large ds:<dwVirtualStack>}

	rev_memcpy(px86, code, sizeof(code));
	*(unsigned int *)(&(px86[0x02])) = (unsigned int)&runtime->execBuff;

	px86 += sizeof(code);
	instrCounter++;
}

void X86Assembler::SwitchToRiverEsp(nodep::BYTE *&px86, nodep::DWORD &instrCounter, nodep::BYTE repReg) {
	static const unsigned char code[] = { 0x87, 0x05, 0x00, 0x00, 0x00, 0x00 };			// 0x00 - xchg eax, large ds:<dwVirtualStack>}

	rev_memcpy(px86, code, sizeof(code));
	px86[0x01] |= (repReg & 0x03); // choose the acording replacement register
	*(unsigned int *)(&(px86[0x02])) = (unsigned int)&runtime->execBuff;

	px86 += sizeof(code);
	instrCounter++;
}

void X86Assembler::EndRiverConversion(RelocableCodeBuffer &px86, nodep::DWORD &pFlags, nodep::BYTE &currentFamily, nodep::BYTE &repReg, nodep::DWORD &instrCounter) {
	if (RIVER_FAMILY_NATIVE != currentFamily) {
		nodep::DWORD currentStack;

		switch (currentFamily)	{
			case RIVER_FAMILY_NATIVE:
				break;
			case RIVER_FAMILY_RIVER:
				currentStack = (nodep::DWORD)&runtime->execBuff;
				break;
			case RIVER_FAMILY_PRETRACK:
				currentStack = (nodep::DWORD)&runtime->trackBuff;
				break;
			default:
				DEBUG_BREAK;
				break;
		}

		SwitchToNative(px86, currentFamily, repReg, instrCounter, currentStack);
	}
}

bool X86Assembler::TranslateNative(const RiverInstruction &ri, RelocableCodeBuffer &px86, nodep::DWORD &pFlags, nodep::BYTE &currentFamily, nodep::BYTE &repReg, nodep::DWORD &instrCounter, nodep::BYTE outputType) {
	// skip ignored instructions
	if (ri.family & RIVER_FAMILY_FLAG_IGNORE) {
		return true;
	}

	//if (/*(RIVER_FAMILY(ri.family) == RIVER_FAMILY_TRACK) || */(RIVER_FAMILY(ri.family) == RIVER_FAMILY_PRETRACK)) {
	//	return true;
	//}

	/*if ((RIVER_FAMILY_RIVER == RIVER_FAMILY(ri.family)) && (0x10 & pFlags)) {
		return true;
	}*/

	// when generating fwcode skip meta instructions
	if ((RIVER_FAMILY(ri.family) == RIVER_FAMILY_PREMETA) || (RIVER_FAMILY(ri.family) == RIVER_FAMILY_POSTMETA)) {
		if (pFlags & FLAG_SKIP_METAOP) {
			return true;
		}
	}

	GenerateTransitionsNative(ri, px86, pFlags, currentFamily, repReg, instrCounter);
	GeneratePrefixes(ri, px86.cursor);

	RiverInstruction rInstr;
	RiverAddress32 rAddr;

	const RiverInstruction *rOut = FixRiverEspInstruction(ri, &rInstr, &rAddr);

	nodep::DWORD dwTable = 0;

	/*if (rOut->modifiers & RIVER_MODIFIER_EXT) {
		*px86.cursor = 0x0F;
		px86.cursor++;
		dwTable = 1;
	}*/

	//(this->*assembleOpcodes[dwTable][rOut->opCode])(*rOut, px86, pFlags, instrCounter);
	//(this->*assembleOperands[dwTable][rOut->opCode])(*rOut, px86);

	GenericX86Assembler *casm = &nAsm;

	if (RIVER_FAMILY(rOut->family) == RIVER_FAMILY_RIVER) {
		casm = &rAsm;
	} else if (RIVER_FAMILY(rOut->family) == RIVER_FAMILY_TRACK) {
		casm = &tAsm; 
	} else if (RIVER_FAMILY(rOut->family) == RIVER_FAMILY_PRETRACK) {
		casm = &ptAsm;
	} else if (RIVER_FAMILY(rOut->family) == RIVER_FAMILY_RIVER_TRACK) {
		casm = &rtAsm;
	} else if (RIVER_FAMILY(rOut->family) != RIVER_FAMILY_NATIVE) {
		DEBUG_BREAK;
	}

	casm->Translate(*rOut, px86, pFlags, currentFamily, repReg, instrCounter, outputType);


	return true;
}

bool X86Assembler::TranslateTracking(const RiverInstruction &ri, RelocableCodeBuffer &px86, nodep::DWORD &pFlags, nodep::BYTE &currentFamily, nodep::BYTE &repReg, nodep::DWORD &instrCounter, nodep::BYTE outputType) {
	if (RIVER_FAMILY_FLAG_IGNORE & ri.family) {
		return true;
	}

	GenerateTransitionsTracking(ri, px86, pFlags, currentFamily, repReg, instrCounter);
	GeneratePrefixes(ri, px86.cursor);
	
	GenericX86Assembler *casm = &tAsm;

	if (RIVER_FAMILY(ri.family) == RIVER_FAMILY_TRACK) {
		casm = &tAsm;
	} else if (RIVER_FAMILY(ri.family) == RIVER_FAMILY_RIVER_TRACK) {
		casm = &rtAsm;
	} else {
		DEBUG_BREAK;
	}
	
	casm->Translate(ri, px86, pFlags, currentFamily, repReg, instrCounter, outputType);

	return true;
}

void X86Assembler::AssembleTrackingEnter(RelocableCodeBuffer &px86, nodep::DWORD &instrCounter) {
	static const nodep::BYTE trackingEnter[] = {
		0x55,								// 0x00 - push ebp
		0x89, 0xE5,							// 0x01 - mov ebp, esp
		0x57,								// 0x03 - push edi
		0x56,								// 0x04 - push esi
		0x8B, 0x75, 0x08					// 0x05 - mov esi, [ebp + 8] ; esi <- trackbuffer
	};

	rev_memcpy(px86.cursor, trackingEnter, sizeof(trackingEnter));
	px86.cursor += sizeof(trackingEnter);

	instrCounter += 5;
}

void X86Assembler::AssembleTrackingLeave(RelocableCodeBuffer &px86, nodep::DWORD &instrCounter) {
	static const nodep::BYTE trackingLeave[] = {
		0x5E,								// 0x00 - pop esi
		0x5F,								// 0x01 - pop edi
		0x89, 0xEC,							// 0x02 - mov esp, ebp
		0x5D,								// 0x04 - pop ebp
		0xC3								// 0x05 - ret
	};

	rev_memcpy(px86.cursor, trackingLeave, sizeof(trackingLeave));
	px86.cursor += sizeof(trackingLeave);

	instrCounter += 5;
}

bool X86Assembler::Assemble(RiverInstruction *pRiver, nodep::DWORD dwInstrCount, RelocableCodeBuffer &px86, nodep::DWORD flg, nodep::DWORD &instrCounter, nodep::DWORD &byteCounter, nodep::BYTE outputType) {
	nodep::BYTE *pTmp = px86.cursor, *pAux = px86.cursor;
	nodep::DWORD pFlags = flg;
	nodep::BYTE currentFamily = (outputType & ASSEMBLER_CODE_TRACKING) ? RIVER_FAMILY_TRACK : RIVER_FAMILY_NATIVE;
	nodep::BYTE repReg = 0;

	static const char headers[][35] = {
		" river to x86 (native forward) ===",
		" river to x86 (native backward) ==",
		" river to x86 (tracking forward) =",
		" river to x86 (tracking backward) "
	};

	static const nodep::DWORD masks[] = {
		PRINT_NATIVE | PRINT_FORWARD,
		PRINT_NATIVE | PRINT_BACKWARD,
		PRINT_TRACKING | PRINT_FORWARD,
		PRINT_TRACKING | PRINT_BACKWARD
	};

	nodep::DWORD printMask = PRINT_INFO | PRINT_ASSEMBLY | masks[outputType];

	TRANSLATE_PRINT(printMask, "=%s============================================\n", headers[outputType]);

	if (ASSEMBLER_CODE_TRACKING & outputType) {
		AssembleTrackingEnter(px86, instrCounter);
		for (; pTmp < px86.cursor; ++pTmp) {
			TRANSLATE_PRINT(printMask, "%02x ", *pTmp);
		}
		TRANSLATE_PRINT(printMask, "\n");
	}

	for (nodep::DWORD i = 0; i < dwInstrCount; ++i) {
		pTmp = px86.cursor;

		if (ASSEMBLER_CODE_TRACKING & outputType) {
			if (!TranslateTracking(pRiver[i], px86, pFlags, currentFamily, repReg, instrCounter, outputType)) {
				return false;
			}
		} else {
			if (!TranslateNative(pRiver[i], px86, pFlags, currentFamily, repReg, instrCounter, outputType)) {
				return false;
			}
		}
		
		for (; pTmp < px86.cursor; ++pTmp) {
			TRANSLATE_PRINT(printMask, "%02x ", *pTmp);
		}
		TRANSLATE_PRINT(printMask, "\n");
	}

	if (ASSEMBLER_CODE_TRACKING & outputType) {
		if (RIVER_FAMILY(currentFamily) == RIVER_FAMILY_RIVER_TRACK) {
			SwitchToStack(px86, instrCounter, (nodep::DWORD)&runtime->trackStack);
			currentFamily = RIVER_FAMILY_TRACK;
		}
		AssembleTrackingLeave(px86, instrCounter);
		for (; pTmp < px86.cursor; ++pTmp) {
			TRANSLATE_PRINT(printMask, "%02x ", *pTmp);
		}
		TRANSLATE_PRINT(printMask, "\n");
	} else {
		EndRiverConversion(px86, pFlags, currentFamily, repReg, instrCounter);
		for (; pTmp < px86.cursor; ++pTmp) {
			TRANSLATE_PRINT(printMask, "%02x ", *pTmp);
		}
		TRANSLATE_PRINT(printMask, "\n");
	}

	TRANSLATE_PRINT(printMask, "===============================================================================\n");
	byteCounter = px86.cursor - pAux;
	return true;
}

void X86Assembler::SwitchEspWithReg(RelocableCodeBuffer &px86, nodep::DWORD &instrCounter, nodep::BYTE repReg, nodep::DWORD dwStack) {
	static const nodep::BYTE code[] = { 0x87, 0x05, 0x00, 0x00, 0x00, 0x00 };			// 0x00 - xchg eax, large ds:<dwVirtualStack>}

	rev_memcpy(px86.cursor, code, sizeof(code));
	px86.cursor[0x01] |= (repReg & 0x03); // choose the acording replacement register
	*(nodep::DWORD *)(&px86.cursor[0x02]) = dwStack;

	px86.cursor += sizeof(code);
	instrCounter++;
}

void X86Assembler::SwitchToStack(RelocableCodeBuffer &px86, nodep::DWORD &instrCounter, nodep::DWORD dwStack) {
	static const unsigned char code[] = { 0x87, 0x25, 0x00, 0x00, 0x00, 0x00 };			// 0x00 - xchg esp, large ds:<dwVirtualStack>}

	rev_memcpy(px86.cursor, code, sizeof(code));
	*(nodep::DWORD *)(&px86.cursor[0x02]) = dwStack;

	px86.cursor += sizeof(code);
	instrCounter++;
}

bool X86Assembler::SwitchToNative(RelocableCodeBuffer &px86, nodep::BYTE &currentFamily, nodep::BYTE repReg, nodep::DWORD &instrCounter, nodep::DWORD dwStack) {
	if (currentFamily & RIVER_FAMILY_FLAG_ORIG_xSP) {
		SwitchEspWithReg(px86, instrCounter, repReg, dwStack);
		currentFamily &= ~RIVER_FAMILY_FLAG_ORIG_xSP;
	}

	switch (RIVER_FAMILY(currentFamily)) {
		case RIVER_FAMILY_NATIVE : 
			break;
		case RIVER_FAMILY_RIVER :
			SwitchToStack(px86, instrCounter, (nodep::DWORD)&runtime->execBuff);
			break;
		case RIVER_FAMILY_PRETRACK :
			SwitchToStack(px86, instrCounter, (nodep::DWORD)&runtime->trackBuff);
			break;
		default:
			DEBUG_BREAK;
			break;
	}

	currentFamily = RIVER_FAMILY_NATIVE;
	return true;
}

bool X86Assembler::GenerateTransitionsNative(const RiverInstruction &ri, RelocableCodeBuffer &px86, nodep::DWORD &pFlags, nodep::BYTE &currentFamily, nodep::BYTE &repReg, nodep::DWORD &instrCounter) {
	nodep::BYTE cf = RIVER_FAMILY(currentFamily);
	nodep::BYTE tf = RIVER_FAMILY(ri.family);

	nodep::DWORD currentStack = 0;

	switch (cf)	{
		case RIVER_FAMILY_NATIVE:
			break;
		case RIVER_FAMILY_RIVER:
			currentStack = (nodep::DWORD)&runtime->execBuff;
			break;
		case RIVER_FAMILY_PRETRACK:
			currentStack = (nodep::DWORD)&runtime->trackBuff;
			break;
		default:
			DEBUG_BREAK;
			break;
	}

	if (cf != tf) {
		SwitchToNative(px86, currentFamily, repReg, instrCounter, currentStack);
		cf = RIVER_FAMILY(currentFamily);
	}

	if (cf != tf) {
		switch (tf) {
			case RIVER_FAMILY_NATIVE :
				break;
			case RIVER_FAMILY_RIVER :
				SwitchToStack(px86, instrCounter, (nodep::DWORD)&runtime->execBuff);
				currentFamily = RIVER_FAMILY_RIVER;
				currentStack = (nodep::DWORD)&runtime->execBuff;
				break;
			case RIVER_FAMILY_PRETRACK :
				SwitchToStack(px86, instrCounter, (nodep::DWORD)&runtime->trackBuff);
				currentFamily = RIVER_FAMILY_PRETRACK;
				currentStack = (nodep::DWORD)&runtime->trackBuff;
				break;
			case RIVER_FAMILY_RIVER_TRACK :
				SwitchToStack(px86, instrCounter, (nodep::DWORD)&runtime->trackStack);
				currentFamily = RIVER_FAMILY_RIVER_TRACK;
				currentStack = (nodep::DWORD)&runtime->trackStack;
			default:
				DEBUG_BREAK;
				break;
		}
	}

	// now both current and target families have the same value
	if (RIVER_FAMILY_FLAG_ORIG_xSP & currentFamily) { // current family preservers ESP
		if (RIVER_FAMILY_FLAG_ORIG_xSP & ri.family) { // target family preservers ESP
			if (0 == ((1 << repReg) & ri.unusedRegisters)) { // switch repReg
				SwitchEspWithReg(px86, instrCounter, repReg, currentStack);
				repReg = ri.GetUnusedRegister();
				SwitchEspWithReg(px86, instrCounter, repReg, currentStack);
			}
		} else {
			SwitchEspWithReg(px86, instrCounter, repReg, currentStack);
			currentFamily &= ~RIVER_FAMILY_FLAG_ORIG_xSP;
		}
	} else { // current family doesn't preserve ESP
		if (RIVER_FAMILY_FLAG_ORIG_xSP & ri.family) {
			SwitchEspWithReg(px86, instrCounter, repReg, currentStack);
			currentFamily |= RIVER_FAMILY_FLAG_ORIG_xSP;
		}
	}

	return true;
}

bool X86Assembler::GenerateTransitionsTracking(const RiverInstruction &ri, RelocableCodeBuffer &px86, nodep::DWORD &pFlags, nodep::BYTE &currentFamily, nodep::BYTE &repReg, nodep::DWORD &instrCounter) {
	nodep::BYTE cf = RIVER_FAMILY(currentFamily);
	nodep::BYTE tf = RIVER_FAMILY(ri.family);

	if ((tf != RIVER_FAMILY_RIVER_TRACK) && (tf != RIVER_FAMILY_TRACK)) {
		DEBUG_BREAK;
	}

	if (cf != tf) {
		SwitchToStack(px86, instrCounter, (nodep::DWORD)&runtime->trackStack);
		currentFamily = ri.family;
	}

	return true;
}

bool X86Assembler::Init(RiverRuntime *runtime, nodep::DWORD dwTranslationFlags) {
	this->runtime = runtime;

	nAsm.Init(runtime);
	rAsm.Init(runtime);
	ptAsm.Init(runtime);
	tAsm.Init(runtime);
	tAsm.SetTranslationFlags(dwTranslationFlags);
	rtAsm.Init(runtime);

	return true;
}
