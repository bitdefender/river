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

		rev_memcpy(rTmp, &rIn, sizeof(*rTmp));
		for (BYTE i = 0; i < 4; ++i) {
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

void X86Assembler::SwitchToRiver(BYTE *&px86, DWORD &instrCounter) {
	static const unsigned char code[] = { 0x87, 0x25, 0x00, 0x00, 0x00, 0x00 };			// 0x00 - xchg esp, large ds:<dwVirtualStack>}

	rev_memcpy(px86, code, sizeof(code));
	*(unsigned int *)(&(px86[0x02])) = (unsigned int)&runtime->execBuff;

	px86 += sizeof(code);
	instrCounter++;
}

void X86Assembler::SwitchToRiverEsp(BYTE *&px86, DWORD &instrCounter, BYTE repReg) {
	static const unsigned char code[] = { 0x87, 0x05, 0x00, 0x00, 0x00, 0x00 };			// 0x00 - xchg eax, large ds:<dwVirtualStack>}

	rev_memcpy(px86, code, sizeof(code));
	px86[0x01] |= (repReg & 0x03); // choose the acording replacement register
	*(unsigned int *)(&(px86[0x02])) = (unsigned int)&runtime->execBuff;

	px86 += sizeof(code);
	instrCounter++;
}

void X86Assembler::EndRiverConversion(RelocableCodeBuffer &px86, DWORD &pFlags, BYTE &currentFamily, BYTE &repReg, DWORD &instrCounter) {
	if (RIVER_FAMILY_NATIVE != currentFamily) {
		DWORD currentStack;

		switch (currentFamily)	{
			case RIVER_FAMILY_NATIVE:
				break;
			case RIVER_FAMILY_RIVER:
				currentStack = (DWORD)&runtime->execBuff;
				break;
			case RIVER_FAMILY_PRETRACK:
				currentStack = (DWORD)&runtime->trackBuff;
				break;
			default:
				__asm int 3;
				break;
		}

		SwitchToNative(px86, currentFamily, repReg, instrCounter, currentStack);
	}
}

bool X86Assembler::TranslateNative(const RiverInstruction &ri, RelocableCodeBuffer &px86, DWORD &pFlags, BYTE &currentFamily, BYTE &repReg, DWORD &instrCounter, BYTE outputType) {
	// skip ignored instructions
	if (ri.family & RIVER_FAMILY_FLAG_IGNORE) {
		return true;
	}

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

	DWORD dwTable = 0;

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
		__asm int 3;
	}

	if (casm == &nAsm) {
		MarkOriginalInstruction(px86.GetOffset());
	}

	casm->Translate(*rOut, px86, pFlags, currentFamily, repReg, instrCounter, outputType);


	return true;
}

bool X86Assembler::TranslateTracking(const RiverInstruction &ri, RelocableCodeBuffer &px86, DWORD &pFlags, BYTE &currentFamily, BYTE &repReg, DWORD &instrCounter, BYTE outputType) {
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
		__asm int 3;
	}
	
	casm->Translate(ri, px86, pFlags, currentFamily, repReg, instrCounter, outputType);

	return true;
}

void X86Assembler::AssembleTrackingEnter(RelocableCodeBuffer &px86, DWORD &instrCounter) {
	static const BYTE trackingEnter[] = {
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

void X86Assembler::AssembleTrackingLeave(RelocableCodeBuffer &px86, DWORD &instrCounter) {
	static const BYTE trackingLeave[] = {
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

extern "C" {
	void __stdcall ExceptionHandler(struct ExecutionEnvironment *pEnv, ADDR_TYPE a);
};

DWORD dwExceptionHandler = (DWORD) ::ExceptionHandler;

void X86Assembler::AssembleHook(RelocableCodeBuffer &px86, DWORD &instrCounter, DWORD address) {
	static const BYTE hookCode[] = {
		0x87, 0x25, 0x00, 0x00, 0x00, 0x00,			// 0x00 - xchg esp, large ds:<dwVirtualStack>
		0x9C, 										// 0x06 - pushf
		0x60,										// 0x07 - pusha
		0x68, 0x46, 0x02, 0x00, 0x00,				// 0x08 - push 0x00000246 - NEW FLAGS
		0x9D,										// 0x0D - popf
		0x68, 0x00, 0x00, 0x00, 0x00,				// 0x0E - push <EIP>
		0x68, 0x00, 0x00, 0x00, 0x00,				// 0x13 - push <execution_environment>
		0xFF, 0x15, 0x00, 0x00, 0x00, 0x00,			// 0x18 - call <dwExceptionHandler>
		0x61,										// 0x1E - popa
		0x9D,										// 0x1F - popf
		0x87, 0x25, 0x00, 0x00, 0x00, 0x00			// 0x20 - xchg esp, large ds:<dwVirtualStack>
	};

	rev_memcpy(px86.cursor, hookCode, sizeof(hookCode));
	*(unsigned int *)(&(px86.cursor[0x02])) = (unsigned int)&runtime->virtualStack;
	*(unsigned int *)(&(px86.cursor[0x0F])) = (unsigned int)address;
	*(unsigned int *)(&(px86.cursor[0x14])) = (unsigned int)runtime;
	*(unsigned int *)(&(px86.cursor[0x1A])) = (unsigned int)&dwExceptionHandler;
	*(unsigned int *)(&(px86.cursor[0x22])) = (unsigned int)&runtime->virtualStack;
	px86.cursor += sizeof(hookCode);

	instrCounter += 11;
}

bool X86Assembler::Assemble(RiverInstruction *pRiver, DWORD dwInstrCount, RelocableCodeBuffer &px86, DWORD flg, DWORD &instrCounter, DWORD &byteCounter, BYTE outputType) {
	BYTE *pTmp = px86.cursor, *pAux = px86.cursor;
	DWORD pFlags = flg;
	BYTE currentFamily = (outputType & ASSEMBLER_CODE_TRACKING) ? RIVER_FAMILY_TRACK : RIVER_FAMILY_NATIVE;
	BYTE repReg = 0;

	static const char headers[][35] = {
		" river to x86 (native forward) ===",
		" river to x86 (native backward) ==",
		" river to x86 (tracking forward) =",
		" river to x86 (tracking backward) "
	};

	static const DWORD masks[] = {
		PRINT_NATIVE | PRINT_FORWARD,
		PRINT_NATIVE | PRINT_BACKWARD,
		PRINT_TRACKING | PRINT_FORWARD,
		PRINT_TRACKING | PRINT_BACKWARD
	};

	DWORD printMask = PRINT_INFO | PRINT_ASSEMBLY | masks[outputType & 3];

	TRANSLATE_PRINT(printMask, "=%s============================================\n", headers[outputType & 3]);

	if (ASSEMBLER_CODE_TRACKING & outputType) {
		AssembleTrackingEnter(px86, instrCounter);
		for (; pTmp < px86.cursor; ++pTmp) {
			TRANSLATE_PRINT(printMask, "%02x ", *pTmp);
		}
		TRANSLATE_PRINT(printMask, "\n");
	}

	if (ASSEMBLER_CODE_HOOK & outputType) {
		unsigned int firstNative = 0;
		while ((firstNative < dwInstrCount) && (RIVER_FAMILY(pRiver[firstNative].family) != RIVER_FAMILY_NATIVE)) {
			firstNative++;
		}

		AssembleHook(px86, instrCounter, pRiver[firstNative].instructionAddress);
		for (; pTmp < px86.cursor; ++pTmp) {
			TRANSLATE_PRINT(printMask, "%02x ", *pTmp);
		}
		TRANSLATE_PRINT(printMask, "\n");
	}

	for (DWORD i = 0; i < dwInstrCount; ++i) {
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
			SwitchToStack(px86, instrCounter, (DWORD)&runtime->trackStack);
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

void X86Assembler::SwitchEspWithReg(RelocableCodeBuffer &px86, DWORD &instrCounter, BYTE repReg, DWORD dwStack) {
	static const BYTE code[] = { 0x87, 0x05, 0x00, 0x00, 0x00, 0x00 };			// 0x00 - xchg eax, large ds:<dwVirtualStack>}

	rev_memcpy(px86.cursor, code, sizeof(code));
	px86.cursor[0x01] |= (repReg & 0x03); // choose the acording replacement register
	*(DWORD *)(&px86.cursor[0x02]) = dwStack;

	px86.cursor += sizeof(code);
	instrCounter++;
}

void X86Assembler::SwitchToStack(RelocableCodeBuffer &px86, DWORD &instrCounter, DWORD dwStack) {
	static const unsigned char code[] = { 0x87, 0x25, 0x00, 0x00, 0x00, 0x00 };			// 0x00 - xchg esp, large ds:<dwVirtualStack>}

	rev_memcpy(px86.cursor, code, sizeof(code));
	*(DWORD *)(&px86.cursor[0x02]) = dwStack;

	px86.cursor += sizeof(code);
	instrCounter++;
}

bool X86Assembler::SwitchToNative(RelocableCodeBuffer &px86, BYTE &currentFamily, BYTE repReg, DWORD &instrCounter, DWORD dwStack) {
	if (currentFamily & RIVER_FAMILY_FLAG_ORIG_xSP) {
		SwitchEspWithReg(px86, instrCounter, repReg, dwStack);
		currentFamily &= ~RIVER_FAMILY_FLAG_ORIG_xSP;
	}

	switch (RIVER_FAMILY(currentFamily)) {
		case RIVER_FAMILY_NATIVE : 
			break;
		case RIVER_FAMILY_RIVER :
			SwitchToStack(px86, instrCounter, (DWORD)&runtime->execBuff);
			break;
		case RIVER_FAMILY_PRETRACK :
			SwitchToStack(px86, instrCounter, (DWORD)&runtime->trackBuff);
			break;
		default:
			__asm int 3;
			break;
	}

	currentFamily = RIVER_FAMILY_NATIVE;
	return true;
}

bool X86Assembler::GenerateTransitionsNative(const RiverInstruction &ri, RelocableCodeBuffer &px86, DWORD &pFlags, BYTE &currentFamily, BYTE &repReg, DWORD &instrCounter) {
	BYTE cf = RIVER_FAMILY(currentFamily);
	BYTE tf = RIVER_FAMILY(ri.family);

	DWORD currentStack = 0;

	switch (cf)	{
		case RIVER_FAMILY_NATIVE:
			break;
		case RIVER_FAMILY_RIVER:
			currentStack = (DWORD)&runtime->execBuff;
			break;
		case RIVER_FAMILY_PRETRACK:
			currentStack = (DWORD)&runtime->trackBuff;
			break;
		default:
			__asm int 3;
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
				SwitchToStack(px86, instrCounter, (DWORD)&runtime->execBuff);
				currentFamily = RIVER_FAMILY_RIVER;
				currentStack = (DWORD)&runtime->execBuff;
				break;
			case RIVER_FAMILY_PRETRACK :
				SwitchToStack(px86, instrCounter, (DWORD)&runtime->trackBuff);
				currentFamily = RIVER_FAMILY_PRETRACK;
				currentStack = (DWORD)&runtime->trackBuff;
				break;
			case RIVER_FAMILY_RIVER_TRACK :
				SwitchToStack(px86, instrCounter, (DWORD)&runtime->trackStack);
				currentFamily = RIVER_FAMILY_RIVER_TRACK;
				currentStack = (DWORD)&runtime->trackStack;
			default:
				__asm int 3;
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

bool X86Assembler::GenerateTransitionsTracking(const RiverInstruction &ri, RelocableCodeBuffer &px86, DWORD &pFlags, BYTE &currentFamily, BYTE &repReg, DWORD &instrCounter) {
	BYTE cf = RIVER_FAMILY(currentFamily);
	BYTE tf = RIVER_FAMILY(ri.family);

	if ((tf != RIVER_FAMILY_RIVER_TRACK) && (tf != RIVER_FAMILY_TRACK)) {
		__asm int 3;
	}

	if (cf != tf) {
		SwitchToStack(px86, instrCounter, (DWORD)&runtime->trackStack);
		currentFamily = ri.family;
	}

	return true;
}

bool X86Assembler::Init(RiverRuntime *runtime, DWORD dwTranslationFlags) {
	this->runtime = runtime;

	nAsm.Init(runtime);
	rAsm.Init(runtime);
	ptAsm.Init(runtime);
	tAsm.Init(runtime);
	tAsm.SetTranslationFlags(dwTranslationFlags);
	rtAsm.Init(runtime);

	return true;
}

void X86Assembler::SetOriginalInstructions(RiverInstruction *ptr) {
	original = ptr;
	originalIdx = 0;
}

void X86Assembler::MarkOriginalInstruction(unsigned int offset) {
	if (nullptr != original) {
		original[originalIdx].assembledOffset = offset;
		originalIdx++;
	}
}