#include "RiverX86Assembler.h"

#include "mm.h"

#define X86_LOCK_PREFIX				0xF0
#define X86_REPNZ_PREFIX			0xF2
#define X86_REPZ_PREFIX				0xF3
#define X86_REP_PREFIX				0xF3

#define FLAG_SKIP_METAOP			0x01
#define FLAG_GENERATE_RIVER			0x02
#define FLAG_GENERATE_RIVER_xSP		0x04

extern DWORD dwSysHandler; // = 0; // &SysHandler
extern DWORD dwSysEndHandler; // = 0; // &SysEndHandler
extern DWORD dwBranchHandler; // = 0; // &BranchHandler

void RiverX86Assembler::SwitchToRiver(BYTE *&px86, DWORD &instrCounter) {
	static const unsigned char code[] = { 0x87, 0x25, 0x00, 0x00, 0x00, 0x00 };			// 0x00 - xchg esp, large ds:<dwVirtualStack>}

	memcpy(px86, code, sizeof(code));
	*(unsigned int *)(&(px86[0x02])) = (unsigned int)&runtime->execBuff;

	px86 += sizeof(code);
	instrCounter++;
}

void RiverX86Assembler::SwitchToRiverEsp(BYTE *&px86, DWORD &instrCounter) {
	static const unsigned char code[] = { 0x87, 0x05, 0x00, 0x00, 0x00, 0x00 };			// 0x00 - xchg eax, large ds:<dwVirtualStack>}

	memcpy(px86, code, sizeof(code));
	*(unsigned int *)(&(px86[0x02])) = (unsigned int)&runtime->execBuff;

	px86 += sizeof(code);
	instrCounter++;
}

void RiverX86Assembler::EndRiverConversion(BYTE *&px86, DWORD &pFlags, DWORD &instrCounter) {
	if (pFlags & FLAG_GENERATE_RIVER) {
		if (pFlags & FLAG_GENERATE_RIVER_xSP) {
			SwitchToRiverEsp(px86, instrCounter);
			pFlags &= ~FLAG_GENERATE_RIVER_xSP;
		}

		SwitchToRiver(px86, instrCounter);
		pFlags &= ~FLAG_GENERATE_RIVER;
	}
}

bool RiverX86Assembler::Init(RiverRuntime *rt) {
	runtime = rt;
	return true;
}

static void FixRiverEspOp(BYTE opType, RiverOperand *op) {
	switch (opType & 0xFC) {
	case RIVER_OPTYPE_IMM:
	case RIVER_OPTYPE_NONE:
		break;
	case RIVER_OPTYPE_REG:
		if (op->asRegister.name == RIVER_REG_xSP) {
			op->asRegister.versioned = RIVER_REG_xAX;
		}
		break;
	case RIVER_OPTYPE_MEM:
		break;
	default:
		__asm int 3;
	}
}

const RiverInstruction *FixRiverEspInstruction(const RiverInstruction &rIn, RiverInstruction *rTmp, RiverAddress *aTmp) {
	if (rIn.modifiers & RIVER_MODIFIER_ORIG_xSP) {
		memcpy(rTmp, &rIn, sizeof(*rTmp));
		if (rIn.opTypes[0] != RIVER_OPTYPE_NONE) {
			if (rIn.opTypes[0] & RIVER_OPTYPE_MEM) {
				rTmp->operands[0].asAddress = aTmp;
				memcpy(aTmp, rIn.operands[0].asAddress, sizeof(*aTmp));
			}
			FixRiverEspOp(rTmp->opTypes[0], &rTmp->operands[0]);
		}

		if (rIn.opTypes[1] != RIVER_OPTYPE_NONE) {
			if (rIn.opTypes[1] & RIVER_OPTYPE_MEM) {
				rTmp->operands[1].asAddress = aTmp;
				memcpy(aTmp, rIn.operands[1].asAddress, sizeof(*aTmp));
			}
			FixRiverEspOp(rTmp->opTypes[1], &rTmp->operands[1]);
		}
		return rTmp;
	} else {
		return &rIn;
	}
}

bool RiverX86Assembler::Translate(const RiverInstruction &ri, BYTE *&px86, DWORD &pFlags, DWORD &instrCounter) {
	// skip ignored instructions
	if (ri.modifiers & RIVER_MODIFIER_IGNORE) {
		return true;
	}

	//FIXME skip all symbop instructions
	if (ri.modifiers & RIVER_MODIFIER_SYMBOP) {
		return true;
	}

	// when generating fwcode skip meta instructions
	if (ri.modifiers & RIVER_MODIFIER_METAOP) {
		if (pFlags & FLAG_SKIP_METAOP) {
			return true;
		}
	}

	// ensure state transitions between river and x86
	if (ri.modifiers & RIVER_MODIFIER_RIVEROP) {
		if (0 == (pFlags & FLAG_GENERATE_RIVER)) {
			SwitchToRiver(px86, instrCounter);
			pFlags |= FLAG_GENERATE_RIVER;
		}

		// ensure state transitions between riveresp and river
		if (ri.modifiers & RIVER_MODIFIER_ORIG_xSP) {
			if (0 == (pFlags & FLAG_GENERATE_RIVER_xSP)) {
				SwitchToRiverEsp(px86, instrCounter);
				pFlags |= FLAG_GENERATE_RIVER_xSP;
			}
		}
		else {
			if (pFlags & FLAG_GENERATE_RIVER_xSP) {
				SwitchToRiverEsp(px86, instrCounter);
				pFlags &= ~FLAG_GENERATE_RIVER_xSP;
			}
		}
	}
	else {
		if (pFlags & FLAG_GENERATE_RIVER) {
			if (pFlags & FLAG_GENERATE_RIVER_xSP) {
				SwitchToRiverEsp(px86, instrCounter);
				pFlags &= ~FLAG_GENERATE_RIVER_xSP;
			}

			SwitchToRiver(px86, instrCounter);
			pFlags &= ~FLAG_GENERATE_RIVER;
		}
	}

	if (ri.modifiers & RIVER_MODIFIER_LOCK) {
		*px86 = X86_LOCK_PREFIX;
		px86++;
	}

	if (ri.modifiers & RIVER_MODIFIER_REP) {
		*px86 = X86_REP_PREFIX;
		px86++;
	}

	if (ri.modifiers & RIVER_MODIFIER_REPZ) {
		*px86 = X86_REPZ_PREFIX;
		px86++;
	}

	if (ri.modifiers & RIVER_MODIFIER_REPNZ) {
		*px86 = X86_REPNZ_PREFIX;
		px86++;
	}

	RiverInstruction rInstr;
	RiverAddress32 rAddr;

	const RiverInstruction *rOut = FixRiverEspInstruction(ri, &rInstr, &rAddr);

	DWORD dwTable = 0;

	if (rOut->modifiers & RIVER_MODIFIER_EXT) {
		*px86 = 0x0F;
		px86++;
		dwTable = 1;
	}

	(this->*assembleOpcodes[dwTable][rOut->opCode])(*rOut, px86, pFlags, instrCounter);
	(this->*assembleOperands[dwTable][rOut->opCode])(*rOut, px86);
	return true;
}

bool RiverX86Assembler::Assemble(RiverInstruction *pRiver, DWORD dwInstrCount, BYTE *px86, DWORD flg, DWORD &instrCounter, DWORD &byteCounter) {
	BYTE *pTmp, *pAux = px86;
	DWORD pFlags = flg;

	DbgPrint("= river to x86 ================================================================\n");

	for (DWORD i = 0; i < dwInstrCount; ++i) {
		pTmp = pAux;

		//pFlags = flg;
		if (!Translate(pRiver[i], pAux, pFlags, instrCounter)) {
			return false;
		}
		//ConvertRiverInstruction(cg, rt, &pRiver[i], &pTmp, &pFlags);

		for (; pTmp < pAux; ++pTmp) {
			DbgPrint("%02x ", *pTmp);
		}
		DbgPrint("\n");
	}

	pTmp = pAux;
	EndRiverConversion(pAux, pFlags, instrCounter);
	for (; pTmp < pAux; ++pTmp) {
		DbgPrint("%02x ", *pTmp);
	}
	DbgPrint("\n");

	DbgPrint("===============================================================================\n");
	byteCounter = pAux - px86;
	return true;
}


/* =========================================== */
/* Opcode assemblers                           */
/* =========================================== */

void RiverX86Assembler::AssembleUnkInstr(const RiverInstruction &ri, BYTE *&px86, DWORD &pFlags, DWORD &instrCounter) {
	__asm int 3;
}

void RiverX86Assembler::AssembleDefaultInstr(const RiverInstruction &ri, BYTE *&px86, DWORD &pFlags, DWORD &instrCounter) {
	*px86 = ri.opCode;
	px86++;
	instrCounter++;
}

void RiverX86Assembler::AssemblePlusRegInstr(const RiverInstruction &ri, BYTE *&px86, DWORD &pFlags, DWORD &instrCounter) {
	unsigned char regName = ri.operands[0].asRegister.name & 0x07; // verify if 8 bit operand
	*px86 = ri.opCode + regName;
	px86++;
	instrCounter++;
}

void RiverX86Assembler::AssembleRelJMPInstr(const RiverInstruction &ri, BYTE *&px86, DWORD &pFlags, DWORD &instrCounter) {
	static const BYTE pBranchJMP[] = {
		0x87, 0x25, 0x00, 0x00, 0x00, 0x00,			// 0x00 - xchg esp, large ds:<dwVirtualStack>
		0x9C, 										// 0x06 - pushf
		0x60,										// 0x07 - pusha
		0x68, 0x46, 0x02, 0x00, 0x00,				// 0x08 - push 0x00000246 - NEW FLAGS
		0x9D,										// 0x0D - popf
		0x68, 0x00, 0x00, 0x00, 0x00,				// 0x0E - push <jump_addr>
		0x68, 0x00, 0x00, 0x00, 0x00,				// 0x13 - push <execution_environment>
		0xFF, 0x15, 0x00, 0x00, 0x00, 0x00,			// 0x18 - call <dwBranchHandler>
		0x61,										// 0x1E - popa
		0x9D,										// 0x1F - popf
		0x87, 0x25, 0x00, 0x00, 0x00, 0x00,			// 0x20 - xchg esp, large ds:<dwVirtualStack>
		0xFF, 0x25, 0x00, 0x00, 0x00, 0x00			// 0x26 - jmp large dword ptr ds:<jumpbuff>	
	};

	int addrJump = (int)(ri.operands[1].asImm32);

	switch (ri.opTypes[0] & 0x03) {
	case RIVER_OPSIZE_8:
		addrJump += (char)ri.operands[0].asImm8;
		break;
	case RIVER_OPSIZE_16:
		addrJump += (short)ri.operands[0].asImm16;
		break;
	case RIVER_OPSIZE_32:
		addrJump += (int)ri.operands[0].asImm32;
		break;
	}

	memcpy(px86, pBranchJMP, sizeof(pBranchJMP));
	*(unsigned int *)(&(px86[0x02])) = (unsigned int)&runtime->virtualStack;
	*(unsigned int *)(&(px86[0x0F])) = addrJump;
	*(unsigned int *)(&(px86[0x14])) = (unsigned int)runtime;
	*(unsigned int *)(&(px86[0x1A])) = (unsigned int)&dwBranchHandler;
	*(unsigned int *)(&(px86[0x22])) = (unsigned int)&runtime->virtualStack;
	*(unsigned int *)(&(px86[0x28])) = (unsigned int)&runtime->jumpBuff;

	px86 += sizeof(pBranchJMP);
	pFlags |= RIVER_FLAG_BRANCH;
	instrCounter += 12;
}

void RiverX86Assembler::AssembleRelJmpCondInstr(const RiverInstruction &ri, BYTE *&px86, DWORD &pFlags, DWORD &instrCounter) {
	const BYTE pBranchJCC[] = {
		0x87, 0x25, 0x00, 0x00, 0x00, 0x00,			// 0x00 - xchg esp, large ds:<dwVirtualStack>
		0x9C, 										// 0x06 - pushf
		0x60,										// 0x07 - pusha
		0x68, 0x46, 0x02, 0x00, 0x00,				// 0x08 - push 0x00000246 - NEW FLAGS
		0x9D,										// 0x0D - popf
		0x68, 0x00, 0x00, 0x00, 0x00,				// 0x0E - push <original_address>
		0x68, 0x00, 0x00, 0x00, 0x00,				// 0x13 - push <execution_environment>
		0xFF, 0x15, 0x00, 0x00, 0x00, 0x00,			// 0x18 - call <branch_handler>
		0x61,										// 0x1E - popa
		0x9D,										// 0x1F - popf
		0x87, 0x25, 0x00, 0x00, 0x00, 0x00,			// 0x20 - xchg esp, large ds:<dwVirtualStack>
		0xFF, 0x25, 0x00, 0x00, 0x00, 0x00			// 0x26 - jmp large dword ptr ds:<jumpbuff>
	};

	int addrFallThrough = (int)(ri.operands[1].asImm32);
	int addrJump = addrFallThrough;

	switch (ri.opTypes[0] & 0x03) {
	case RIVER_OPSIZE_8:
		addrJump += (char)ri.operands[0].asImm8;
		break;
	case RIVER_OPSIZE_16:
		addrJump += (short)ri.operands[0].asImm16;
		break;
	case RIVER_OPSIZE_32:
		addrJump += (int)ri.operands[0].asImm32;
		break;
	}

	/* copy the Jcc instruction */
	*px86 = ri.opCode;
	px86++;
	*px86 = sizeof(pBranchJCC);
	px86++;

	memcpy(px86, pBranchJCC, sizeof(pBranchJCC));

	*(unsigned int *)(&(px86[0x02])) = (unsigned int)&runtime->virtualStack;
	*(unsigned int *)(&(px86[0x0F])) = addrFallThrough;
	*(unsigned int *)(&(px86[0x14])) = (unsigned int)runtime;
	*(unsigned int *)(&(px86[0x1A])) = (unsigned int)&dwBranchHandler;
	*(unsigned int *)(&(px86[0x22])) = (unsigned int)&runtime->virtualStack;
	*(unsigned int *)(&(px86[0x28])) = (unsigned int)&runtime->jumpBuff;
	px86 += sizeof(pBranchJCC);


	memcpy(px86, pBranchJCC, sizeof(pBranchJCC));

	*(unsigned int *)(&(px86[0x02])) = (unsigned int)&runtime->virtualStack;
	*(unsigned int *)(&(px86[0x0F])) = addrJump;
	*(unsigned int *)(&(px86[0x14])) = (unsigned int)runtime;
	*(unsigned int *)(&(px86[0x1A])) = (unsigned int)&dwBranchHandler;
	*(unsigned int *)(&(px86[0x22])) = (unsigned int)&runtime->virtualStack;
	*(unsigned int *)(&(px86[0x28])) = (unsigned int)&runtime->jumpBuff;
	px86 += sizeof(pBranchJCC);

	pFlags |= RIVER_FLAG_BRANCH;
	instrCounter += 25;
}

void RiverX86Assembler::AssembleRetnInstr(const RiverInstruction &ri, BYTE *&px86, DWORD &pFlags, DWORD &instrCounter) {
	//RetImm - copy the value
	//Retn - 0
	//RetFar - 4
	const BYTE pBranchRet[] = {
		0xA3, 0x00, 0x00, 0x00, 0x00,				// 0x00 - mov [<dwEaxSave>], eax
		0x58,										// 0x05 - pop eax
		0x87, 0x25, 0x00, 0x00, 0x00, 0x00,			// 0x06 - xchg esp, large ds:<dwVirtualStack>
		0x9C,			 							// 0x0C - pushf
		0x60,										// 0x0D - pusha
		0x68, 0x46, 0x02, 0x00, 0x00,				// 0x0E - push 0x00000246 - NEW FLAGS
		0x9D,										// 0x13 - popf
		0x50,										// 0x14 - push eax
		0x68, 0x00, 0x00, 0x00, 0x00,				// 0x15 - push <execution_environment>
		0xFF, 0x15, 0x00, 0x00, 0x00, 0x00,			// 0x1A - call <branch_handler>

		0x61,										// 0x20 - popa
		0x9D,										// 0x21 - popf
		0x87, 0x25, 0x00, 0x00, 0x00, 0x00,			// 0x22 - xchg esp, large ds:<dwVirtualStack>
		0xA1, 0x00, 0x00, 0x00, 0x00,				// 0x28 - mov eax, large ds:<dwEaxSave>
		0x8D, 0xA4, 0x24, 0x00, 0x00, 0x00, 0x00,   // 0x2D - lea esp, [esp + <pI>] // probably sub esp, xxx
		0xFF, 0x25, 0x00, 0x00, 0x00, 0x00			// 0x34 - jmp large dword ptr ds:<jumpbuff>
	};

	unsigned short stackSpace = 0;
	memcpy(px86, pBranchRet, sizeof(pBranchRet));

	*(unsigned int *)(&(px86[0x01])) = (unsigned int)&runtime->returnRegister;
	*(unsigned int *)(&(px86[0x08])) = (unsigned int)&runtime->virtualStack;
	*(unsigned int *)(&(px86[0x16])) = (unsigned int)runtime;
	*(unsigned int *)(&(px86[0x1C])) = (unsigned int)&dwBranchHandler;
	*(unsigned int *)(&(px86[0x24])) = (unsigned int)&runtime->virtualStack;
	*(unsigned int *)(&(px86[0x29])) = (unsigned int)&runtime->returnRegister;
	*(unsigned int *)(&(px86[0x30])) = stackSpace;
	*(unsigned int *)(&(px86[0x36])) = (unsigned int)&runtime->jumpBuff;

	px86 += sizeof(pBranchRet);
	pFlags |= RIVER_FLAG_BRANCH;
	instrCounter += 16;
}

void RiverX86Assembler::AssembleRiverAddSubInstr(const RiverInstruction &ri, BYTE *&px86, DWORD &pFlags, DWORD &instrCounter) {
	*px86 = 0x8d; // add and sub are converted to lea
	px86++;
	instrCounter++;
}

/* =========================================== */
/* Operand helpers                             */
/* =========================================== */

void RiverX86Assembler::AssembleModRMOp(unsigned int opIdx, const RiverInstruction &ri, BYTE *&px86, BYTE extra) {
	ri.operands[opIdx].asAddress->EncodeTox86(px86, extra, ri.modifiers);
}

void RiverX86Assembler::AssembleImmOp(unsigned int opIdx, const RiverInstruction &ri, BYTE *&px86, BYTE immSize) {
	switch (immSize) {
	case RIVER_OPSIZE_8:
		*((BYTE *)px86) = ri.operands[opIdx].asImm8;
		px86++;
		break;
	case RIVER_OPSIZE_16:
		*((WORD *)px86) = ri.operands[opIdx].asImm16;
		px86 += 2;
		break;
	case RIVER_OPSIZE_32:
		*((DWORD *)px86) = ri.operands[opIdx].asImm32;
		px86 += 4;
		break;
	}
}


/* =========================================== */
/* Operand assemblers                          */
/* =========================================== */

void RiverX86Assembler::AssembleUnknownOp(const RiverInstruction &ri, BYTE *&px86) {
	__asm int 3;
}

void RiverX86Assembler::AssembleNoOp(const RiverInstruction &ri, BYTE *&px86) {
}

void RiverX86Assembler::AssembleRegModRMOp(const RiverInstruction &ri, BYTE *&px86) {
	AssembleModRMOp(1, ri, px86, ri.operands[0].asRegister.name);
}

void RiverX86Assembler::AssembleModRMRegOp(const RiverInstruction &ri, BYTE *&px86) {
	AssembleModRMOp(0, ri, px86, ri.operands[1].asRegister.name);
}

void RiverX86Assembler::AssembleSubOpModRMImm8Op(const RiverInstruction &ri, BYTE *&px86) {
	AssembleModRMOp(0, ri, px86, ri.subOpCode);
	AssembleImmOp(1, ri, px86, RIVER_OPSIZE_8);
}

void RiverX86Assembler::AssembleRiverAddSubOp(const RiverInstruction &ri, BYTE *&px86) {
	RiverInstruction rTmp;
	RiverAddress32 addr;

	addr.type = RIVER_ADDR_DIRTY | RIVER_ADDR_BASE | RIVER_ADDR_DISP8;
	addr.base.versioned = ri.operands[0].asRegister.versioned;

	addr.disp.d8 = ri.operands[1].asImm8;

	if (ri.subOpCode == 5) {
		addr.disp.d8 = ~addr.disp.d8 + 1;
	}

	addr.modRM |= ri.operands[0].asRegister.name << 3;

	rTmp.opTypes[0] = ri.opTypes[0];
	rTmp.operands[0].asRegister.versioned = ri.operands[0].asRegister.versioned;

	rTmp.opTypes[1] = RIVER_OPTYPE_MEM; // no size specified;
	rTmp.operands[1].asAddress = &addr;


	AssembleRegModRMOp(rTmp, px86);
}

/* =========================================== */
/* Additional assembly tables                  */
/* =========================================== */

RiverX86Assembler::AssembleOpcodeFunc RiverX86Assembler::assemble0xFFInstr[8] = {
	/*0x00*/ &RiverX86Assembler::AssembleUnkInstr, &RiverX86Assembler::AssembleUnkInstr, &RiverX86Assembler::AssembleUnkInstr, &RiverX86Assembler::AssembleUnkInstr,
	/*0x04*/ &RiverX86Assembler::AssembleUnkInstr, &RiverX86Assembler::AssembleUnkInstr, &RiverX86Assembler::AssembleDefaultInstr, &RiverX86Assembler::AssembleUnkInstr
};

RiverX86Assembler::AssembleOperandsFunc RiverX86Assembler::assemble0xFFOp[8] = {
	/*0x00*/ &RiverX86Assembler::AssembleUnknownOp, &RiverX86Assembler::AssembleUnknownOp, &RiverX86Assembler::AssembleUnknownOp, &RiverX86Assembler::AssembleUnknownOp,
	/*0x04*/ &RiverX86Assembler::AssembleUnknownOp, &RiverX86Assembler::AssembleUnknownOp, &RiverX86Assembler::AssembleModRMOp<6>, &RiverX86Assembler::AssembleUnknownOp,
};

/* =========================================== */
/* Assembly tables                             */
/* =========================================== */

RiverX86Assembler::AssembleOpcodeFunc RiverX86Assembler::assembleOpcodes[2][0x100] = {
	{
		/*0x00*/ &RiverX86Assembler::AssembleUnkInstr, &RiverX86Assembler::AssembleUnkInstr, &RiverX86Assembler::AssembleUnkInstr, &RiverX86Assembler::AssembleUnkInstr,
		/*0x04*/ &RiverX86Assembler::AssembleUnkInstr, &RiverX86Assembler::AssembleUnkInstr, &RiverX86Assembler::AssembleUnkInstr, &RiverX86Assembler::AssembleUnkInstr,
		/*0x08*/ &RiverX86Assembler::AssembleUnkInstr, &RiverX86Assembler::AssembleUnkInstr, &RiverX86Assembler::AssembleUnkInstr, &RiverX86Assembler::AssembleUnkInstr,
		/*0x0C*/ &RiverX86Assembler::AssembleUnkInstr, &RiverX86Assembler::AssembleUnkInstr, &RiverX86Assembler::AssembleUnkInstr, &RiverX86Assembler::AssembleUnkInstr,

		/*0x10*/ &RiverX86Assembler::AssembleUnkInstr, &RiverX86Assembler::AssembleUnkInstr, &RiverX86Assembler::AssembleUnkInstr, &RiverX86Assembler::AssembleUnkInstr,
		/*0x14*/ &RiverX86Assembler::AssembleUnkInstr, &RiverX86Assembler::AssembleUnkInstr, &RiverX86Assembler::AssembleUnkInstr, &RiverX86Assembler::AssembleUnkInstr,
		/*0x18*/ &RiverX86Assembler::AssembleUnkInstr, &RiverX86Assembler::AssembleUnkInstr, &RiverX86Assembler::AssembleUnkInstr, &RiverX86Assembler::AssembleUnkInstr,
		/*0x1C*/ &RiverX86Assembler::AssembleUnkInstr, &RiverX86Assembler::AssembleUnkInstr, &RiverX86Assembler::AssembleUnkInstr, &RiverX86Assembler::AssembleUnkInstr,

		/*0x20*/ &RiverX86Assembler::AssembleUnkInstr, &RiverX86Assembler::AssembleUnkInstr, &RiverX86Assembler::AssembleUnkInstr, &RiverX86Assembler::AssembleUnkInstr,
		/*0x24*/ &RiverX86Assembler::AssembleUnkInstr, &RiverX86Assembler::AssembleUnkInstr, &RiverX86Assembler::AssembleUnkInstr, &RiverX86Assembler::AssembleUnkInstr,
		/*0x28*/ &RiverX86Assembler::AssembleUnkInstr, &RiverX86Assembler::AssembleUnkInstr, &RiverX86Assembler::AssembleUnkInstr, &RiverX86Assembler::AssembleUnkInstr,
		/*0x2C*/ &RiverX86Assembler::AssembleUnkInstr, &RiverX86Assembler::AssembleUnkInstr, &RiverX86Assembler::AssembleUnkInstr, &RiverX86Assembler::AssembleUnkInstr,

		/*0x30*/ &RiverX86Assembler::AssembleUnkInstr, &RiverX86Assembler::AssembleUnkInstr, &RiverX86Assembler::AssembleUnkInstr, &RiverX86Assembler::AssembleDefaultInstr,
		/*0x34*/ &RiverX86Assembler::AssembleUnkInstr, &RiverX86Assembler::AssembleUnkInstr, &RiverX86Assembler::AssembleUnkInstr, &RiverX86Assembler::AssembleUnkInstr,
		/*0x38*/ &RiverX86Assembler::AssembleUnkInstr, &RiverX86Assembler::AssembleUnkInstr, &RiverX86Assembler::AssembleUnkInstr, &RiverX86Assembler::AssembleDefaultInstr,
		/*0x3C*/ &RiverX86Assembler::AssembleUnkInstr, &RiverX86Assembler::AssembleUnkInstr, &RiverX86Assembler::AssembleUnkInstr, &RiverX86Assembler::AssembleUnkInstr,

		/*0x40*/ &RiverX86Assembler::AssemblePlusRegInstr, &RiverX86Assembler::AssemblePlusRegInstr, &RiverX86Assembler::AssemblePlusRegInstr, &RiverX86Assembler::AssemblePlusRegInstr,
		/*0x44*/ &RiverX86Assembler::AssemblePlusRegInstr, &RiverX86Assembler::AssemblePlusRegInstr, &RiverX86Assembler::AssemblePlusRegInstr, &RiverX86Assembler::AssemblePlusRegInstr,
		/*0x48*/ &RiverX86Assembler::AssemblePlusRegInstr, &RiverX86Assembler::AssemblePlusRegInstr, &RiverX86Assembler::AssemblePlusRegInstr, &RiverX86Assembler::AssemblePlusRegInstr,
		/*0x4C*/ &RiverX86Assembler::AssemblePlusRegInstr, &RiverX86Assembler::AssemblePlusRegInstr, &RiverX86Assembler::AssemblePlusRegInstr, &RiverX86Assembler::AssemblePlusRegInstr,

		/*0x50*/ &RiverX86Assembler::AssemblePlusRegInstr, &RiverX86Assembler::AssemblePlusRegInstr, &RiverX86Assembler::AssemblePlusRegInstr, &RiverX86Assembler::AssemblePlusRegInstr,
		/*0x54*/ &RiverX86Assembler::AssemblePlusRegInstr, &RiverX86Assembler::AssemblePlusRegInstr, &RiverX86Assembler::AssemblePlusRegInstr, &RiverX86Assembler::AssemblePlusRegInstr,
		/*0x58*/ &RiverX86Assembler::AssemblePlusRegInstr, &RiverX86Assembler::AssemblePlusRegInstr, &RiverX86Assembler::AssemblePlusRegInstr, &RiverX86Assembler::AssemblePlusRegInstr,
		/*0x5C*/ &RiverX86Assembler::AssemblePlusRegInstr, &RiverX86Assembler::AssemblePlusRegInstr, &RiverX86Assembler::AssemblePlusRegInstr, &RiverX86Assembler::AssemblePlusRegInstr,

		/*0x60*/ &RiverX86Assembler::AssembleUnkInstr, &RiverX86Assembler::AssembleUnkInstr, &RiverX86Assembler::AssembleUnkInstr, &RiverX86Assembler::AssembleUnkInstr,
		/*0x64*/ &RiverX86Assembler::AssembleUnkInstr, &RiverX86Assembler::AssembleUnkInstr, &RiverX86Assembler::AssembleUnkInstr, &RiverX86Assembler::AssembleUnkInstr,
		/*0x68*/ &RiverX86Assembler::AssembleUnkInstr, &RiverX86Assembler::AssembleUnkInstr, &RiverX86Assembler::AssembleUnkInstr, &RiverX86Assembler::AssembleUnkInstr,
		/*0x6C*/ &RiverX86Assembler::AssembleUnkInstr, &RiverX86Assembler::AssembleUnkInstr, &RiverX86Assembler::AssembleUnkInstr, &RiverX86Assembler::AssembleUnkInstr,

		/*0x70*/ &RiverX86Assembler::AssembleRelJmpCondInstr, &RiverX86Assembler::AssembleRelJmpCondInstr, &RiverX86Assembler::AssembleRelJmpCondInstr, &RiverX86Assembler::AssembleRelJmpCondInstr,
		/*0x74*/ &RiverX86Assembler::AssembleRelJmpCondInstr, &RiverX86Assembler::AssembleRelJmpCondInstr, &RiverX86Assembler::AssembleRelJmpCondInstr, &RiverX86Assembler::AssembleRelJmpCondInstr,
		/*0x78*/ &RiverX86Assembler::AssembleRelJmpCondInstr, &RiverX86Assembler::AssembleRelJmpCondInstr, &RiverX86Assembler::AssembleRelJmpCondInstr, &RiverX86Assembler::AssembleRelJmpCondInstr,
		/*0x7C*/ &RiverX86Assembler::AssembleRelJmpCondInstr, &RiverX86Assembler::AssembleRelJmpCondInstr, &RiverX86Assembler::AssembleRelJmpCondInstr, &RiverX86Assembler::AssembleRelJmpCondInstr,

		/*0x80*/ &RiverX86Assembler::AssembleUnkInstr, &RiverX86Assembler::AssembleUnkInstr, &RiverX86Assembler::AssembleUnkInstr, &RiverX86Assembler::AssembleRiverInstr<&RiverX86Assembler::AssembleRiverAddSubInstr, &RiverX86Assembler::AssembleDefaultInstr>,
		/*0x84*/ &RiverX86Assembler::AssembleUnkInstr, &RiverX86Assembler::AssembleUnkInstr, &RiverX86Assembler::AssembleUnkInstr, &RiverX86Assembler::AssembleUnkInstr,
		/*0x88*/ &RiverX86Assembler::AssembleUnkInstr, &RiverX86Assembler::AssembleDefaultInstr, &RiverX86Assembler::AssembleUnkInstr, &RiverX86Assembler::AssembleDefaultInstr,
		/*0x8C*/ &RiverX86Assembler::AssembleUnkInstr, &RiverX86Assembler::AssembleUnkInstr, &RiverX86Assembler::AssembleUnkInstr, &RiverX86Assembler::AssembleDefaultInstr,

		/*0x90*/ &RiverX86Assembler::AssemblePlusRegInstr, &RiverX86Assembler::AssemblePlusRegInstr, &RiverX86Assembler::AssemblePlusRegInstr, &RiverX86Assembler::AssemblePlusRegInstr,
		/*0x94*/ &RiverX86Assembler::AssemblePlusRegInstr, &RiverX86Assembler::AssemblePlusRegInstr, &RiverX86Assembler::AssemblePlusRegInstr, &RiverX86Assembler::AssemblePlusRegInstr,
		/*0x98*/ &RiverX86Assembler::AssembleUnkInstr, &RiverX86Assembler::AssembleUnkInstr, &RiverX86Assembler::AssembleUnkInstr, &RiverX86Assembler::AssembleUnkInstr,
		/*0x9C*/ &RiverX86Assembler::AssembleDefaultInstr, &RiverX86Assembler::AssembleDefaultInstr, &RiverX86Assembler::AssembleUnkInstr, &RiverX86Assembler::AssembleUnkInstr,

		/*0xA0*/ &RiverX86Assembler::AssembleUnkInstr, &RiverX86Assembler::AssembleUnkInstr, &RiverX86Assembler::AssembleUnkInstr, &RiverX86Assembler::AssembleUnkInstr,
		/*0xA4*/ &RiverX86Assembler::AssembleUnkInstr, &RiverX86Assembler::AssembleUnkInstr, &RiverX86Assembler::AssembleUnkInstr, &RiverX86Assembler::AssembleUnkInstr,
		/*0xA8*/ &RiverX86Assembler::AssembleUnkInstr, &RiverX86Assembler::AssembleUnkInstr, &RiverX86Assembler::AssembleUnkInstr, &RiverX86Assembler::AssembleUnkInstr,
		/*0xAC*/ &RiverX86Assembler::AssembleUnkInstr, &RiverX86Assembler::AssembleUnkInstr, &RiverX86Assembler::AssembleUnkInstr, &RiverX86Assembler::AssembleUnkInstr,

		/*0xB0*/ &RiverX86Assembler::AssemblePlusRegInstr, &RiverX86Assembler::AssemblePlusRegInstr, &RiverX86Assembler::AssemblePlusRegInstr, &RiverX86Assembler::AssemblePlusRegInstr,
		/*0xB4*/ &RiverX86Assembler::AssemblePlusRegInstr, &RiverX86Assembler::AssemblePlusRegInstr, &RiverX86Assembler::AssemblePlusRegInstr, &RiverX86Assembler::AssemblePlusRegInstr,
		/*0xB8*/ &RiverX86Assembler::AssemblePlusRegInstr, &RiverX86Assembler::AssemblePlusRegInstr, &RiverX86Assembler::AssemblePlusRegInstr, &RiverX86Assembler::AssemblePlusRegInstr,
		/*0xBC*/ &RiverX86Assembler::AssemblePlusRegInstr, &RiverX86Assembler::AssemblePlusRegInstr, &RiverX86Assembler::AssemblePlusRegInstr, &RiverX86Assembler::AssemblePlusRegInstr,

		/*0xC0*/ &RiverX86Assembler::AssembleUnkInstr, &RiverX86Assembler::AssembleUnkInstr, &RiverX86Assembler::AssembleUnkInstr, &RiverX86Assembler::AssembleRetnInstr,
		/*0xC4*/ &RiverX86Assembler::AssembleUnkInstr, &RiverX86Assembler::AssembleUnkInstr, &RiverX86Assembler::AssembleUnkInstr, &RiverX86Assembler::AssembleUnkInstr,
		/*0xC8*/ &RiverX86Assembler::AssembleUnkInstr, &RiverX86Assembler::AssembleUnkInstr, &RiverX86Assembler::AssembleUnkInstr, &RiverX86Assembler::AssembleUnkInstr,
		/*0xCC*/ &RiverX86Assembler::AssembleUnkInstr, &RiverX86Assembler::AssembleUnkInstr, &RiverX86Assembler::AssembleUnkInstr, &RiverX86Assembler::AssembleUnkInstr,

		/*0xD0*/ &RiverX86Assembler::AssembleUnkInstr, &RiverX86Assembler::AssembleUnkInstr, &RiverX86Assembler::AssembleUnkInstr, &RiverX86Assembler::AssembleUnkInstr,
		/*0xD4*/ &RiverX86Assembler::AssembleUnkInstr, &RiverX86Assembler::AssembleUnkInstr, &RiverX86Assembler::AssembleUnkInstr, &RiverX86Assembler::AssembleUnkInstr,
		/*0xD8*/ &RiverX86Assembler::AssembleUnkInstr, &RiverX86Assembler::AssembleUnkInstr, &RiverX86Assembler::AssembleUnkInstr, &RiverX86Assembler::AssembleUnkInstr,
		/*0xDC*/ &RiverX86Assembler::AssembleUnkInstr, &RiverX86Assembler::AssembleUnkInstr, &RiverX86Assembler::AssembleUnkInstr, &RiverX86Assembler::AssembleUnkInstr,

		/*0xE0*/ &RiverX86Assembler::AssembleUnkInstr, &RiverX86Assembler::AssembleUnkInstr, &RiverX86Assembler::AssembleUnkInstr, &RiverX86Assembler::AssembleUnkInstr,
		/*0xE4*/ &RiverX86Assembler::AssembleUnkInstr, &RiverX86Assembler::AssembleUnkInstr, &RiverX86Assembler::AssembleUnkInstr, &RiverX86Assembler::AssembleUnkInstr,
		/*0xE8*/ &RiverX86Assembler::AssembleUnkInstr, &RiverX86Assembler::AssembleRelJMPInstr, &RiverX86Assembler::AssembleUnkInstr, &RiverX86Assembler::AssembleUnkInstr,
		/*0xEC*/ &RiverX86Assembler::AssembleUnkInstr, &RiverX86Assembler::AssembleUnkInstr, &RiverX86Assembler::AssembleUnkInstr, &RiverX86Assembler::AssembleUnkInstr,

		/*0xF0*/ &RiverX86Assembler::AssembleUnkInstr, &RiverX86Assembler::AssembleUnkInstr, &RiverX86Assembler::AssembleUnkInstr, &RiverX86Assembler::AssembleUnkInstr,
		/*0xF4*/ &RiverX86Assembler::AssembleUnkInstr, &RiverX86Assembler::AssembleUnkInstr, &RiverX86Assembler::AssembleUnkInstr, &RiverX86Assembler::AssembleUnkInstr,
		/*0xF8*/ &RiverX86Assembler::AssembleUnkInstr, &RiverX86Assembler::AssembleUnkInstr, &RiverX86Assembler::AssembleUnkInstr, &RiverX86Assembler::AssembleUnkInstr,
		/*0xFC*/ &RiverX86Assembler::AssembleUnkInstr, &RiverX86Assembler::AssembleUnkInstr, &RiverX86Assembler::AssembleUnkInstr, &RiverX86Assembler::AssembleSubOpInstr<assemble0xFFInstr>
	}, {
		/*0x00*/ &RiverX86Assembler::AssembleUnkInstr, &RiverX86Assembler::AssembleUnkInstr, &RiverX86Assembler::AssembleUnkInstr, &RiverX86Assembler::AssembleUnkInstr,
		/*0x04*/ &RiverX86Assembler::AssembleUnkInstr, &RiverX86Assembler::AssembleUnkInstr, &RiverX86Assembler::AssembleUnkInstr, &RiverX86Assembler::AssembleUnkInstr,
		/*0x08*/ &RiverX86Assembler::AssembleUnkInstr, &RiverX86Assembler::AssembleUnkInstr, &RiverX86Assembler::AssembleUnkInstr, &RiverX86Assembler::AssembleUnkInstr,
		/*0x0C*/ &RiverX86Assembler::AssembleUnkInstr, &RiverX86Assembler::AssembleUnkInstr, &RiverX86Assembler::AssembleUnkInstr, &RiverX86Assembler::AssembleUnkInstr,

		/*0x10*/ &RiverX86Assembler::AssembleUnkInstr, &RiverX86Assembler::AssembleUnkInstr, &RiverX86Assembler::AssembleUnkInstr, &RiverX86Assembler::AssembleUnkInstr,
		/*0x14*/ &RiverX86Assembler::AssembleUnkInstr, &RiverX86Assembler::AssembleUnkInstr, &RiverX86Assembler::AssembleUnkInstr, &RiverX86Assembler::AssembleUnkInstr,
		/*0x18*/ &RiverX86Assembler::AssembleUnkInstr, &RiverX86Assembler::AssembleUnkInstr, &RiverX86Assembler::AssembleUnkInstr, &RiverX86Assembler::AssembleUnkInstr,
		/*0x1C*/ &RiverX86Assembler::AssembleUnkInstr, &RiverX86Assembler::AssembleUnkInstr, &RiverX86Assembler::AssembleUnkInstr, &RiverX86Assembler::AssembleUnkInstr,

		/*0x20*/ &RiverX86Assembler::AssembleUnkInstr, &RiverX86Assembler::AssembleUnkInstr, &RiverX86Assembler::AssembleUnkInstr, &RiverX86Assembler::AssembleUnkInstr,
		/*0x24*/ &RiverX86Assembler::AssembleUnkInstr, &RiverX86Assembler::AssembleUnkInstr, &RiverX86Assembler::AssembleUnkInstr, &RiverX86Assembler::AssembleUnkInstr,
		/*0x28*/ &RiverX86Assembler::AssembleUnkInstr, &RiverX86Assembler::AssembleUnkInstr, &RiverX86Assembler::AssembleUnkInstr, &RiverX86Assembler::AssembleUnkInstr,
		/*0x2C*/ &RiverX86Assembler::AssembleUnkInstr, &RiverX86Assembler::AssembleUnkInstr, &RiverX86Assembler::AssembleUnkInstr, &RiverX86Assembler::AssembleUnkInstr,

		/*0x30*/ &RiverX86Assembler::AssembleUnkInstr, &RiverX86Assembler::AssembleUnkInstr, &RiverX86Assembler::AssembleUnkInstr, &RiverX86Assembler::AssembleUnkInstr,
		/*0x34*/ &RiverX86Assembler::AssembleUnkInstr, &RiverX86Assembler::AssembleUnkInstr, &RiverX86Assembler::AssembleUnkInstr, &RiverX86Assembler::AssembleUnkInstr,
		/*0x38*/ &RiverX86Assembler::AssembleUnkInstr, &RiverX86Assembler::AssembleUnkInstr, &RiverX86Assembler::AssembleUnkInstr, &RiverX86Assembler::AssembleUnkInstr,
		/*0x3C*/ &RiverX86Assembler::AssembleUnkInstr, &RiverX86Assembler::AssembleUnkInstr, &RiverX86Assembler::AssembleUnkInstr, &RiverX86Assembler::AssembleUnkInstr,

		/*0x40*/ &RiverX86Assembler::AssembleUnkInstr, &RiverX86Assembler::AssembleUnkInstr, &RiverX86Assembler::AssembleUnkInstr, &RiverX86Assembler::AssembleUnkInstr,
		/*0x44*/ &RiverX86Assembler::AssembleUnkInstr, &RiverX86Assembler::AssembleUnkInstr, &RiverX86Assembler::AssembleUnkInstr, &RiverX86Assembler::AssembleUnkInstr,
		/*0x48*/ &RiverX86Assembler::AssembleUnkInstr, &RiverX86Assembler::AssembleUnkInstr, &RiverX86Assembler::AssembleUnkInstr, &RiverX86Assembler::AssembleUnkInstr,
		/*0x4C*/ &RiverX86Assembler::AssembleUnkInstr, &RiverX86Assembler::AssembleUnkInstr, &RiverX86Assembler::AssembleUnkInstr, &RiverX86Assembler::AssembleUnkInstr,

		/*0x50*/ &RiverX86Assembler::AssembleUnkInstr, &RiverX86Assembler::AssembleUnkInstr, &RiverX86Assembler::AssembleUnkInstr, &RiverX86Assembler::AssembleUnkInstr,
		/*0x54*/ &RiverX86Assembler::AssembleUnkInstr, &RiverX86Assembler::AssembleUnkInstr, &RiverX86Assembler::AssembleUnkInstr, &RiverX86Assembler::AssembleUnkInstr,
		/*0x58*/ &RiverX86Assembler::AssembleUnkInstr, &RiverX86Assembler::AssembleUnkInstr, &RiverX86Assembler::AssembleUnkInstr, &RiverX86Assembler::AssembleUnkInstr,
		/*0x5C*/ &RiverX86Assembler::AssembleUnkInstr, &RiverX86Assembler::AssembleUnkInstr, &RiverX86Assembler::AssembleUnkInstr, &RiverX86Assembler::AssembleUnkInstr,

		/*0x60*/ &RiverX86Assembler::AssembleUnkInstr, &RiverX86Assembler::AssembleUnkInstr, &RiverX86Assembler::AssembleUnkInstr, &RiverX86Assembler::AssembleUnkInstr,
		/*0x64*/ &RiverX86Assembler::AssembleUnkInstr, &RiverX86Assembler::AssembleUnkInstr, &RiverX86Assembler::AssembleUnkInstr, &RiverX86Assembler::AssembleUnkInstr,
		/*0x68*/ &RiverX86Assembler::AssembleUnkInstr, &RiverX86Assembler::AssembleUnkInstr, &RiverX86Assembler::AssembleUnkInstr, &RiverX86Assembler::AssembleUnkInstr,
		/*0x6C*/ &RiverX86Assembler::AssembleUnkInstr, &RiverX86Assembler::AssembleUnkInstr, &RiverX86Assembler::AssembleUnkInstr, &RiverX86Assembler::AssembleUnkInstr,

		/*0x70*/ &RiverX86Assembler::AssembleUnkInstr, &RiverX86Assembler::AssembleUnkInstr, &RiverX86Assembler::AssembleUnkInstr, &RiverX86Assembler::AssembleUnkInstr,
		/*0x74*/ &RiverX86Assembler::AssembleUnkInstr, &RiverX86Assembler::AssembleUnkInstr, &RiverX86Assembler::AssembleUnkInstr, &RiverX86Assembler::AssembleUnkInstr,
		/*0x78*/ &RiverX86Assembler::AssembleUnkInstr, &RiverX86Assembler::AssembleUnkInstr, &RiverX86Assembler::AssembleUnkInstr, &RiverX86Assembler::AssembleUnkInstr,
		/*0x7C*/ &RiverX86Assembler::AssembleUnkInstr, &RiverX86Assembler::AssembleUnkInstr, &RiverX86Assembler::AssembleUnkInstr, &RiverX86Assembler::AssembleUnkInstr,

		/*0x80*/ &RiverX86Assembler::AssembleUnkInstr, &RiverX86Assembler::AssembleUnkInstr, &RiverX86Assembler::AssembleUnkInstr, &RiverX86Assembler::AssembleUnkInstr,
		/*0x84*/ &RiverX86Assembler::AssembleUnkInstr, &RiverX86Assembler::AssembleUnkInstr, &RiverX86Assembler::AssembleUnkInstr, &RiverX86Assembler::AssembleUnkInstr,
		/*0x88*/ &RiverX86Assembler::AssembleUnkInstr, &RiverX86Assembler::AssembleUnkInstr, &RiverX86Assembler::AssembleUnkInstr, &RiverX86Assembler::AssembleUnkInstr,
		/*0x8C*/ &RiverX86Assembler::AssembleUnkInstr, &RiverX86Assembler::AssembleUnkInstr, &RiverX86Assembler::AssembleUnkInstr, &RiverX86Assembler::AssembleUnkInstr,

		/*0x90*/ &RiverX86Assembler::AssembleUnkInstr, &RiverX86Assembler::AssembleUnkInstr, &RiverX86Assembler::AssembleUnkInstr, &RiverX86Assembler::AssembleUnkInstr,
		/*0x94*/ &RiverX86Assembler::AssembleUnkInstr, &RiverX86Assembler::AssembleUnkInstr, &RiverX86Assembler::AssembleUnkInstr, &RiverX86Assembler::AssembleUnkInstr,
		/*0x98*/ &RiverX86Assembler::AssembleUnkInstr, &RiverX86Assembler::AssembleUnkInstr, &RiverX86Assembler::AssembleUnkInstr, &RiverX86Assembler::AssembleUnkInstr,
		/*0x9C*/ &RiverX86Assembler::AssembleUnkInstr, &RiverX86Assembler::AssembleUnkInstr, &RiverX86Assembler::AssembleUnkInstr, &RiverX86Assembler::AssembleUnkInstr,

		/*0xA0*/ &RiverX86Assembler::AssembleUnkInstr, &RiverX86Assembler::AssembleUnkInstr, &RiverX86Assembler::AssembleUnkInstr, &RiverX86Assembler::AssembleUnkInstr,
		/*0xA4*/ &RiverX86Assembler::AssembleUnkInstr, &RiverX86Assembler::AssembleUnkInstr, &RiverX86Assembler::AssembleUnkInstr, &RiverX86Assembler::AssembleUnkInstr,
		/*0xA8*/ &RiverX86Assembler::AssembleUnkInstr, &RiverX86Assembler::AssembleUnkInstr, &RiverX86Assembler::AssembleUnkInstr, &RiverX86Assembler::AssembleUnkInstr,
		/*0xAC*/ &RiverX86Assembler::AssembleUnkInstr, &RiverX86Assembler::AssembleUnkInstr, &RiverX86Assembler::AssembleUnkInstr, &RiverX86Assembler::AssembleUnkInstr,

		/*0xB0*/ &RiverX86Assembler::AssembleUnkInstr, &RiverX86Assembler::AssembleUnkInstr, &RiverX86Assembler::AssembleUnkInstr, &RiverX86Assembler::AssembleUnkInstr,
		/*0xB4*/ &RiverX86Assembler::AssembleUnkInstr, &RiverX86Assembler::AssembleUnkInstr, &RiverX86Assembler::AssembleUnkInstr, &RiverX86Assembler::AssembleUnkInstr,
		/*0xB8*/ &RiverX86Assembler::AssembleUnkInstr, &RiverX86Assembler::AssembleUnkInstr, &RiverX86Assembler::AssembleUnkInstr, &RiverX86Assembler::AssembleUnkInstr,
		/*0xBC*/ &RiverX86Assembler::AssembleUnkInstr, &RiverX86Assembler::AssembleUnkInstr, &RiverX86Assembler::AssembleUnkInstr, &RiverX86Assembler::AssembleUnkInstr,

		/*0xC0*/ &RiverX86Assembler::AssembleUnkInstr, &RiverX86Assembler::AssembleUnkInstr, &RiverX86Assembler::AssembleUnkInstr, &RiverX86Assembler::AssembleUnkInstr,
		/*0xC4*/ &RiverX86Assembler::AssembleUnkInstr, &RiverX86Assembler::AssembleUnkInstr, &RiverX86Assembler::AssembleUnkInstr, &RiverX86Assembler::AssembleUnkInstr,
		/*0xC8*/ &RiverX86Assembler::AssembleUnkInstr, &RiverX86Assembler::AssembleUnkInstr, &RiverX86Assembler::AssembleUnkInstr, &RiverX86Assembler::AssembleUnkInstr,
		/*0xCC*/ &RiverX86Assembler::AssembleUnkInstr, &RiverX86Assembler::AssembleUnkInstr, &RiverX86Assembler::AssembleUnkInstr, &RiverX86Assembler::AssembleUnkInstr,

		/*0xD0*/ &RiverX86Assembler::AssembleUnkInstr, &RiverX86Assembler::AssembleUnkInstr, &RiverX86Assembler::AssembleUnkInstr, &RiverX86Assembler::AssembleUnkInstr,
		/*0xD4*/ &RiverX86Assembler::AssembleUnkInstr, &RiverX86Assembler::AssembleUnkInstr, &RiverX86Assembler::AssembleUnkInstr, &RiverX86Assembler::AssembleUnkInstr,
		/*0xD8*/ &RiverX86Assembler::AssembleUnkInstr, &RiverX86Assembler::AssembleUnkInstr, &RiverX86Assembler::AssembleUnkInstr, &RiverX86Assembler::AssembleUnkInstr,
		/*0xDC*/ &RiverX86Assembler::AssembleUnkInstr, &RiverX86Assembler::AssembleUnkInstr, &RiverX86Assembler::AssembleUnkInstr, &RiverX86Assembler::AssembleUnkInstr,

		/*0xE0*/ &RiverX86Assembler::AssembleUnkInstr, &RiverX86Assembler::AssembleUnkInstr, &RiverX86Assembler::AssembleUnkInstr, &RiverX86Assembler::AssembleUnkInstr,
		/*0xE4*/ &RiverX86Assembler::AssembleUnkInstr, &RiverX86Assembler::AssembleUnkInstr, &RiverX86Assembler::AssembleUnkInstr, &RiverX86Assembler::AssembleUnkInstr,
		/*0xE8*/ &RiverX86Assembler::AssembleUnkInstr, &RiverX86Assembler::AssembleUnkInstr, &RiverX86Assembler::AssembleUnkInstr, &RiverX86Assembler::AssembleUnkInstr,
		/*0xEC*/ &RiverX86Assembler::AssembleUnkInstr, &RiverX86Assembler::AssembleUnkInstr, &RiverX86Assembler::AssembleUnkInstr, &RiverX86Assembler::AssembleUnkInstr,

		/*0xF0*/ &RiverX86Assembler::AssembleUnkInstr, &RiverX86Assembler::AssembleUnkInstr, &RiverX86Assembler::AssembleUnkInstr, &RiverX86Assembler::AssembleUnkInstr,
		/*0xF4*/ &RiverX86Assembler::AssembleUnkInstr, &RiverX86Assembler::AssembleUnkInstr, &RiverX86Assembler::AssembleUnkInstr, &RiverX86Assembler::AssembleUnkInstr,
		/*0xF8*/ &RiverX86Assembler::AssembleUnkInstr, &RiverX86Assembler::AssembleUnkInstr, &RiverX86Assembler::AssembleUnkInstr, &RiverX86Assembler::AssembleUnkInstr,
		/*0xFC*/ &RiverX86Assembler::AssembleUnkInstr, &RiverX86Assembler::AssembleUnkInstr, &RiverX86Assembler::AssembleUnkInstr, &RiverX86Assembler::AssembleUnkInstr
	}
};

RiverX86Assembler::AssembleOperandsFunc RiverX86Assembler::assembleOperands[2][0x100] = {
	{
		/*0x00*/ &RiverX86Assembler::AssembleUnknownOp, &RiverX86Assembler::AssembleUnknownOp, &RiverX86Assembler::AssembleUnknownOp, &RiverX86Assembler::AssembleUnknownOp,
		/*0x04*/ &RiverX86Assembler::AssembleUnknownOp, &RiverX86Assembler::AssembleUnknownOp, &RiverX86Assembler::AssembleUnknownOp, &RiverX86Assembler::AssembleUnknownOp,
		/*0x08*/ &RiverX86Assembler::AssembleUnknownOp, &RiverX86Assembler::AssembleUnknownOp, &RiverX86Assembler::AssembleUnknownOp, &RiverX86Assembler::AssembleUnknownOp,
		/*0x0C*/ &RiverX86Assembler::AssembleUnknownOp, &RiverX86Assembler::AssembleUnknownOp, &RiverX86Assembler::AssembleUnknownOp, &RiverX86Assembler::AssembleUnknownOp,

		/*0x10*/ &RiverX86Assembler::AssembleUnknownOp, &RiverX86Assembler::AssembleUnknownOp, &RiverX86Assembler::AssembleUnknownOp, &RiverX86Assembler::AssembleUnknownOp,
		/*0x14*/ &RiverX86Assembler::AssembleUnknownOp, &RiverX86Assembler::AssembleUnknownOp, &RiverX86Assembler::AssembleUnknownOp, &RiverX86Assembler::AssembleUnknownOp,
		/*0x18*/ &RiverX86Assembler::AssembleUnknownOp, &RiverX86Assembler::AssembleUnknownOp, &RiverX86Assembler::AssembleUnknownOp, &RiverX86Assembler::AssembleUnknownOp,
		/*0x1C*/ &RiverX86Assembler::AssembleUnknownOp, &RiverX86Assembler::AssembleUnknownOp, &RiverX86Assembler::AssembleUnknownOp, &RiverX86Assembler::AssembleUnknownOp,

		/*0x20*/ &RiverX86Assembler::AssembleUnknownOp, &RiverX86Assembler::AssembleUnknownOp, &RiverX86Assembler::AssembleUnknownOp, &RiverX86Assembler::AssembleUnknownOp,
		/*0x24*/ &RiverX86Assembler::AssembleUnknownOp, &RiverX86Assembler::AssembleUnknownOp, &RiverX86Assembler::AssembleUnknownOp, &RiverX86Assembler::AssembleUnknownOp,
		/*0x28*/ &RiverX86Assembler::AssembleUnknownOp, &RiverX86Assembler::AssembleUnknownOp, &RiverX86Assembler::AssembleUnknownOp, &RiverX86Assembler::AssembleUnknownOp,
		/*0x2C*/ &RiverX86Assembler::AssembleUnknownOp, &RiverX86Assembler::AssembleUnknownOp, &RiverX86Assembler::AssembleUnknownOp, &RiverX86Assembler::AssembleUnknownOp,

		/*0x30*/ &RiverX86Assembler::AssembleUnknownOp, &RiverX86Assembler::AssembleUnknownOp, &RiverX86Assembler::AssembleUnknownOp, &RiverX86Assembler::AssembleRegModRMOp,
		/*0x34*/ &RiverX86Assembler::AssembleUnknownOp, &RiverX86Assembler::AssembleUnknownOp, &RiverX86Assembler::AssembleUnknownOp, &RiverX86Assembler::AssembleUnknownOp,
		/*0x38*/ &RiverX86Assembler::AssembleUnknownOp, &RiverX86Assembler::AssembleUnknownOp, &RiverX86Assembler::AssembleUnknownOp, &RiverX86Assembler::AssembleRegModRMOp,
		/*0x3C*/ &RiverX86Assembler::AssembleUnknownOp, &RiverX86Assembler::AssembleUnknownOp, &RiverX86Assembler::AssembleUnknownOp, &RiverX86Assembler::AssembleUnknownOp,

		/*0x40*/ &RiverX86Assembler::AssembleNoOp, &RiverX86Assembler::AssembleNoOp, &RiverX86Assembler::AssembleNoOp, &RiverX86Assembler::AssembleNoOp,
		/*0x44*/ &RiverX86Assembler::AssembleNoOp, &RiverX86Assembler::AssembleNoOp, &RiverX86Assembler::AssembleNoOp, &RiverX86Assembler::AssembleNoOp,
		/*0x48*/ &RiverX86Assembler::AssembleNoOp, &RiverX86Assembler::AssembleNoOp, &RiverX86Assembler::AssembleNoOp, &RiverX86Assembler::AssembleNoOp,
		/*0x4C*/ &RiverX86Assembler::AssembleNoOp, &RiverX86Assembler::AssembleNoOp, &RiverX86Assembler::AssembleNoOp, &RiverX86Assembler::AssembleNoOp,

		/*0x50*/ &RiverX86Assembler::AssembleNoOp, &RiverX86Assembler::AssembleNoOp, &RiverX86Assembler::AssembleNoOp, &RiverX86Assembler::AssembleNoOp,
		/*0x54*/ &RiverX86Assembler::AssembleNoOp, &RiverX86Assembler::AssembleNoOp, &RiverX86Assembler::AssembleNoOp, &RiverX86Assembler::AssembleNoOp,
		/*0x58*/ &RiverX86Assembler::AssembleNoOp, &RiverX86Assembler::AssembleNoOp, &RiverX86Assembler::AssembleNoOp, &RiverX86Assembler::AssembleNoOp,
		/*0x5C*/ &RiverX86Assembler::AssembleNoOp, &RiverX86Assembler::AssembleNoOp, &RiverX86Assembler::AssembleNoOp, &RiverX86Assembler::AssembleNoOp,

		/*0x60*/ &RiverX86Assembler::AssembleUnknownOp, &RiverX86Assembler::AssembleUnknownOp, &RiverX86Assembler::AssembleUnknownOp, &RiverX86Assembler::AssembleUnknownOp,
		/*0x64*/ &RiverX86Assembler::AssembleUnknownOp, &RiverX86Assembler::AssembleUnknownOp, &RiverX86Assembler::AssembleUnknownOp, &RiverX86Assembler::AssembleUnknownOp,
		/*0x68*/ &RiverX86Assembler::AssembleUnknownOp, &RiverX86Assembler::AssembleUnknownOp, &RiverX86Assembler::AssembleUnknownOp, &RiverX86Assembler::AssembleUnknownOp,
		/*0x6C*/ &RiverX86Assembler::AssembleUnknownOp, &RiverX86Assembler::AssembleUnknownOp, &RiverX86Assembler::AssembleUnknownOp, &RiverX86Assembler::AssembleUnknownOp,

		/*0x70*/ &RiverX86Assembler::AssembleNoOp, &RiverX86Assembler::AssembleNoOp, &RiverX86Assembler::AssembleNoOp, &RiverX86Assembler::AssembleNoOp,
		/*0x74*/ &RiverX86Assembler::AssembleNoOp, &RiverX86Assembler::AssembleNoOp, &RiverX86Assembler::AssembleNoOp, &RiverX86Assembler::AssembleNoOp,
		/*0x78*/ &RiverX86Assembler::AssembleNoOp, &RiverX86Assembler::AssembleNoOp, &RiverX86Assembler::AssembleNoOp, &RiverX86Assembler::AssembleNoOp,
		/*0x7C*/ &RiverX86Assembler::AssembleNoOp, &RiverX86Assembler::AssembleNoOp, &RiverX86Assembler::AssembleNoOp, &RiverX86Assembler::AssembleNoOp,

		/*0x80*/ &RiverX86Assembler::AssembleUnknownOp, &RiverX86Assembler::AssembleUnknownOp, &RiverX86Assembler::AssembleUnknownOp, &RiverX86Assembler::AssembleRiverOp<&RiverX86Assembler::AssembleRiverAddSubOp, &RiverX86Assembler::AssembleSubOpModRMImm8Op>,
		/*0x84*/ &RiverX86Assembler::AssembleUnknownOp, &RiverX86Assembler::AssembleUnknownOp, &RiverX86Assembler::AssembleUnknownOp, &RiverX86Assembler::AssembleUnknownOp,
		/*0x88*/ &RiverX86Assembler::AssembleUnknownOp, &RiverX86Assembler::AssembleModRMRegOp, &RiverX86Assembler::AssembleUnknownOp, &RiverX86Assembler::AssembleRegModRMOp,
		/*0x8C*/ &RiverX86Assembler::AssembleUnknownOp, &RiverX86Assembler::AssembleUnknownOp, &RiverX86Assembler::AssembleUnknownOp, &RiverX86Assembler::AssembleModRMOp<0>,

		/*0x90*/ &RiverX86Assembler::AssembleNoOp, &RiverX86Assembler::AssembleNoOp, &RiverX86Assembler::AssembleNoOp, &RiverX86Assembler::AssembleNoOp,
		/*0x94*/ &RiverX86Assembler::AssembleNoOp, &RiverX86Assembler::AssembleNoOp, &RiverX86Assembler::AssembleNoOp, &RiverX86Assembler::AssembleNoOp,
		/*0x98*/ &RiverX86Assembler::AssembleUnknownOp, &RiverX86Assembler::AssembleUnknownOp, &RiverX86Assembler::AssembleUnknownOp, &RiverX86Assembler::AssembleUnknownOp,
		/*0x9C*/ &RiverX86Assembler::AssembleNoOp, &RiverX86Assembler::AssembleNoOp, &RiverX86Assembler::AssembleUnknownOp, &RiverX86Assembler::AssembleUnknownOp,

		/*0xA0*/ &RiverX86Assembler::AssembleUnknownOp, &RiverX86Assembler::AssembleUnknownOp, &RiverX86Assembler::AssembleUnknownOp, &RiverX86Assembler::AssembleUnknownOp,
		/*0xA4*/ &RiverX86Assembler::AssembleUnknownOp, &RiverX86Assembler::AssembleUnknownOp, &RiverX86Assembler::AssembleUnknownOp, &RiverX86Assembler::AssembleUnknownOp,
		/*0xA8*/ &RiverX86Assembler::AssembleUnknownOp, &RiverX86Assembler::AssembleUnknownOp, &RiverX86Assembler::AssembleUnknownOp, &RiverX86Assembler::AssembleUnknownOp,
		/*0xAC*/ &RiverX86Assembler::AssembleUnknownOp, &RiverX86Assembler::AssembleUnknownOp, &RiverX86Assembler::AssembleUnknownOp, &RiverX86Assembler::AssembleUnknownOp,

		/*0xB0*/ &RiverX86Assembler::AssembleNoOp, &RiverX86Assembler::AssembleNoOp, &RiverX86Assembler::AssembleNoOp, &RiverX86Assembler::AssembleNoOp,
		/*0xB4*/ &RiverX86Assembler::AssembleNoOp, &RiverX86Assembler::AssembleNoOp, &RiverX86Assembler::AssembleNoOp, &RiverX86Assembler::AssembleNoOp,
		/*0xB8*/ &RiverX86Assembler::AssembleNoOp, &RiverX86Assembler::AssembleNoOp, &RiverX86Assembler::AssembleNoOp, &RiverX86Assembler::AssembleNoOp,
		/*0xBC*/ &RiverX86Assembler::AssembleNoOp, &RiverX86Assembler::AssembleNoOp, &RiverX86Assembler::AssembleNoOp, &RiverX86Assembler::AssembleNoOp,

		/*0xC0*/ &RiverX86Assembler::AssembleUnknownOp, &RiverX86Assembler::AssembleUnknownOp, &RiverX86Assembler::AssembleUnknownOp, &RiverX86Assembler::AssembleNoOp,
		/*0xC4*/ &RiverX86Assembler::AssembleUnknownOp, &RiverX86Assembler::AssembleUnknownOp, &RiverX86Assembler::AssembleUnknownOp, &RiverX86Assembler::AssembleUnknownOp,
		/*0xC8*/ &RiverX86Assembler::AssembleUnknownOp, &RiverX86Assembler::AssembleUnknownOp, &RiverX86Assembler::AssembleUnknownOp, &RiverX86Assembler::AssembleUnknownOp,
		/*0xCC*/ &RiverX86Assembler::AssembleUnknownOp, &RiverX86Assembler::AssembleUnknownOp, &RiverX86Assembler::AssembleUnknownOp, &RiverX86Assembler::AssembleUnknownOp,

		/*0xD0*/ &RiverX86Assembler::AssembleUnknownOp, &RiverX86Assembler::AssembleUnknownOp, &RiverX86Assembler::AssembleUnknownOp, &RiverX86Assembler::AssembleUnknownOp,
		/*0xD4*/ &RiverX86Assembler::AssembleUnknownOp, &RiverX86Assembler::AssembleUnknownOp, &RiverX86Assembler::AssembleUnknownOp, &RiverX86Assembler::AssembleUnknownOp,
		/*0xD8*/ &RiverX86Assembler::AssembleUnknownOp, &RiverX86Assembler::AssembleUnknownOp, &RiverX86Assembler::AssembleUnknownOp, &RiverX86Assembler::AssembleUnknownOp,
		/*0xDC*/ &RiverX86Assembler::AssembleUnknownOp, &RiverX86Assembler::AssembleUnknownOp, &RiverX86Assembler::AssembleUnknownOp, &RiverX86Assembler::AssembleUnknownOp,

		/*0xE0*/ &RiverX86Assembler::AssembleUnknownOp, &RiverX86Assembler::AssembleUnknownOp, &RiverX86Assembler::AssembleUnknownOp, &RiverX86Assembler::AssembleUnknownOp,
		/*0xE4*/ &RiverX86Assembler::AssembleUnknownOp, &RiverX86Assembler::AssembleUnknownOp, &RiverX86Assembler::AssembleUnknownOp, &RiverX86Assembler::AssembleUnknownOp,
		/*0xE8*/ &RiverX86Assembler::AssembleUnknownOp, &RiverX86Assembler::AssembleNoOp, &RiverX86Assembler::AssembleUnknownOp, &RiverX86Assembler::AssembleUnknownOp,
		/*0xEC*/ &RiverX86Assembler::AssembleUnknownOp, &RiverX86Assembler::AssembleUnknownOp, &RiverX86Assembler::AssembleUnknownOp, &RiverX86Assembler::AssembleUnknownOp,

		/*0xF0*/ &RiverX86Assembler::AssembleUnknownOp, &RiverX86Assembler::AssembleUnknownOp, &RiverX86Assembler::AssembleUnknownOp, &RiverX86Assembler::AssembleUnknownOp,
		/*0xF4*/ &RiverX86Assembler::AssembleUnknownOp, &RiverX86Assembler::AssembleUnknownOp, &RiverX86Assembler::AssembleUnknownOp, &RiverX86Assembler::AssembleUnknownOp,
		/*0xF8*/ &RiverX86Assembler::AssembleUnknownOp, &RiverX86Assembler::AssembleUnknownOp, &RiverX86Assembler::AssembleUnknownOp, &RiverX86Assembler::AssembleUnknownOp,
		/*0xFC*/ &RiverX86Assembler::AssembleUnknownOp, &RiverX86Assembler::AssembleUnknownOp, &RiverX86Assembler::AssembleUnknownOp, &RiverX86Assembler::AssembleSubOpOp<assemble0xFFOp>
	}, {
		/*0x00*/ &RiverX86Assembler::AssembleUnknownOp, &RiverX86Assembler::AssembleUnknownOp, &RiverX86Assembler::AssembleUnknownOp, &RiverX86Assembler::AssembleUnknownOp,
		/*0x04*/ &RiverX86Assembler::AssembleUnknownOp, &RiverX86Assembler::AssembleUnknownOp, &RiverX86Assembler::AssembleUnknownOp, &RiverX86Assembler::AssembleUnknownOp,
		/*0x08*/ &RiverX86Assembler::AssembleUnknownOp, &RiverX86Assembler::AssembleUnknownOp, &RiverX86Assembler::AssembleUnknownOp, &RiverX86Assembler::AssembleUnknownOp,
		/*0x0C*/ &RiverX86Assembler::AssembleUnknownOp, &RiverX86Assembler::AssembleUnknownOp, &RiverX86Assembler::AssembleUnknownOp, &RiverX86Assembler::AssembleUnknownOp,

		/*0x10*/ &RiverX86Assembler::AssembleUnknownOp, &RiverX86Assembler::AssembleUnknownOp, &RiverX86Assembler::AssembleUnknownOp, &RiverX86Assembler::AssembleUnknownOp,
		/*0x14*/ &RiverX86Assembler::AssembleUnknownOp, &RiverX86Assembler::AssembleUnknownOp, &RiverX86Assembler::AssembleUnknownOp, &RiverX86Assembler::AssembleUnknownOp,
		/*0x18*/ &RiverX86Assembler::AssembleUnknownOp, &RiverX86Assembler::AssembleUnknownOp, &RiverX86Assembler::AssembleUnknownOp, &RiverX86Assembler::AssembleUnknownOp,
		/*0x1C*/ &RiverX86Assembler::AssembleUnknownOp, &RiverX86Assembler::AssembleUnknownOp, &RiverX86Assembler::AssembleUnknownOp, &RiverX86Assembler::AssembleUnknownOp,

		/*0x20*/ &RiverX86Assembler::AssembleUnknownOp, &RiverX86Assembler::AssembleUnknownOp, &RiverX86Assembler::AssembleUnknownOp, &RiverX86Assembler::AssembleUnknownOp,
		/*0x24*/ &RiverX86Assembler::AssembleUnknownOp, &RiverX86Assembler::AssembleUnknownOp, &RiverX86Assembler::AssembleUnknownOp, &RiverX86Assembler::AssembleUnknownOp,
		/*0x28*/ &RiverX86Assembler::AssembleUnknownOp, &RiverX86Assembler::AssembleUnknownOp, &RiverX86Assembler::AssembleUnknownOp, &RiverX86Assembler::AssembleUnknownOp,
		/*0x2C*/ &RiverX86Assembler::AssembleUnknownOp, &RiverX86Assembler::AssembleUnknownOp, &RiverX86Assembler::AssembleUnknownOp, &RiverX86Assembler::AssembleUnknownOp,

		/*0x30*/ &RiverX86Assembler::AssembleUnknownOp, &RiverX86Assembler::AssembleUnknownOp, &RiverX86Assembler::AssembleUnknownOp, &RiverX86Assembler::AssembleUnknownOp,
		/*0x34*/ &RiverX86Assembler::AssembleUnknownOp, &RiverX86Assembler::AssembleUnknownOp, &RiverX86Assembler::AssembleUnknownOp, &RiverX86Assembler::AssembleUnknownOp,
		/*0x38*/ &RiverX86Assembler::AssembleUnknownOp, &RiverX86Assembler::AssembleUnknownOp, &RiverX86Assembler::AssembleUnknownOp, &RiverX86Assembler::AssembleUnknownOp,
		/*0x3C*/ &RiverX86Assembler::AssembleUnknownOp, &RiverX86Assembler::AssembleUnknownOp, &RiverX86Assembler::AssembleUnknownOp, &RiverX86Assembler::AssembleUnknownOp,

		/*0x40*/ &RiverX86Assembler::AssembleUnknownOp, &RiverX86Assembler::AssembleUnknownOp, &RiverX86Assembler::AssembleUnknownOp, &RiverX86Assembler::AssembleUnknownOp,
		/*0x44*/ &RiverX86Assembler::AssembleUnknownOp, &RiverX86Assembler::AssembleUnknownOp, &RiverX86Assembler::AssembleUnknownOp, &RiverX86Assembler::AssembleUnknownOp,
		/*0x48*/ &RiverX86Assembler::AssembleUnknownOp, &RiverX86Assembler::AssembleUnknownOp, &RiverX86Assembler::AssembleUnknownOp, &RiverX86Assembler::AssembleUnknownOp,
		/*0x4C*/ &RiverX86Assembler::AssembleUnknownOp, &RiverX86Assembler::AssembleUnknownOp, &RiverX86Assembler::AssembleUnknownOp, &RiverX86Assembler::AssembleUnknownOp,

		/*0x50*/ &RiverX86Assembler::AssembleUnknownOp, &RiverX86Assembler::AssembleUnknownOp, &RiverX86Assembler::AssembleUnknownOp, &RiverX86Assembler::AssembleUnknownOp,
		/*0x54*/ &RiverX86Assembler::AssembleUnknownOp, &RiverX86Assembler::AssembleUnknownOp, &RiverX86Assembler::AssembleUnknownOp, &RiverX86Assembler::AssembleUnknownOp,
		/*0x58*/ &RiverX86Assembler::AssembleUnknownOp, &RiverX86Assembler::AssembleUnknownOp, &RiverX86Assembler::AssembleUnknownOp, &RiverX86Assembler::AssembleUnknownOp,
		/*0x5C*/ &RiverX86Assembler::AssembleUnknownOp, &RiverX86Assembler::AssembleUnknownOp, &RiverX86Assembler::AssembleUnknownOp, &RiverX86Assembler::AssembleUnknownOp,

		/*0x60*/ &RiverX86Assembler::AssembleUnknownOp, &RiverX86Assembler::AssembleUnknownOp, &RiverX86Assembler::AssembleUnknownOp, &RiverX86Assembler::AssembleUnknownOp,
		/*0x64*/ &RiverX86Assembler::AssembleUnknownOp, &RiverX86Assembler::AssembleUnknownOp, &RiverX86Assembler::AssembleUnknownOp, &RiverX86Assembler::AssembleUnknownOp,
		/*0x68*/ &RiverX86Assembler::AssembleUnknownOp, &RiverX86Assembler::AssembleUnknownOp, &RiverX86Assembler::AssembleUnknownOp, &RiverX86Assembler::AssembleUnknownOp,
		/*0x6C*/ &RiverX86Assembler::AssembleUnknownOp, &RiverX86Assembler::AssembleUnknownOp, &RiverX86Assembler::AssembleUnknownOp, &RiverX86Assembler::AssembleUnknownOp,

		/*0x70*/ &RiverX86Assembler::AssembleUnknownOp, &RiverX86Assembler::AssembleUnknownOp, &RiverX86Assembler::AssembleUnknownOp, &RiverX86Assembler::AssembleUnknownOp,
		/*0x74*/ &RiverX86Assembler::AssembleUnknownOp, &RiverX86Assembler::AssembleUnknownOp, &RiverX86Assembler::AssembleUnknownOp, &RiverX86Assembler::AssembleUnknownOp,
		/*0x78*/ &RiverX86Assembler::AssembleUnknownOp, &RiverX86Assembler::AssembleUnknownOp, &RiverX86Assembler::AssembleUnknownOp, &RiverX86Assembler::AssembleUnknownOp,
		/*0x7C*/ &RiverX86Assembler::AssembleUnknownOp, &RiverX86Assembler::AssembleUnknownOp, &RiverX86Assembler::AssembleUnknownOp, &RiverX86Assembler::AssembleUnknownOp,

		/*0x80*/ &RiverX86Assembler::AssembleUnknownOp, &RiverX86Assembler::AssembleUnknownOp, &RiverX86Assembler::AssembleUnknownOp, &RiverX86Assembler::AssembleUnknownOp,
		/*0x84*/ &RiverX86Assembler::AssembleUnknownOp, &RiverX86Assembler::AssembleUnknownOp, &RiverX86Assembler::AssembleUnknownOp, &RiverX86Assembler::AssembleUnknownOp,
		/*0x88*/ &RiverX86Assembler::AssembleUnknownOp, &RiverX86Assembler::AssembleUnknownOp, &RiverX86Assembler::AssembleUnknownOp, &RiverX86Assembler::AssembleUnknownOp,
		/*0x8C*/ &RiverX86Assembler::AssembleUnknownOp, &RiverX86Assembler::AssembleUnknownOp, &RiverX86Assembler::AssembleUnknownOp, &RiverX86Assembler::AssembleUnknownOp,

		/*0x90*/ &RiverX86Assembler::AssembleUnknownOp, &RiverX86Assembler::AssembleUnknownOp, &RiverX86Assembler::AssembleUnknownOp, &RiverX86Assembler::AssembleUnknownOp,
		/*0x94*/ &RiverX86Assembler::AssembleUnknownOp, &RiverX86Assembler::AssembleUnknownOp, &RiverX86Assembler::AssembleUnknownOp, &RiverX86Assembler::AssembleUnknownOp,
		/*0x98*/ &RiverX86Assembler::AssembleUnknownOp, &RiverX86Assembler::AssembleUnknownOp, &RiverX86Assembler::AssembleUnknownOp, &RiverX86Assembler::AssembleUnknownOp,
		/*0x9C*/ &RiverX86Assembler::AssembleUnknownOp, &RiverX86Assembler::AssembleUnknownOp, &RiverX86Assembler::AssembleUnknownOp, &RiverX86Assembler::AssembleUnknownOp,

		/*0xA0*/ &RiverX86Assembler::AssembleUnknownOp, &RiverX86Assembler::AssembleUnknownOp, &RiverX86Assembler::AssembleUnknownOp, &RiverX86Assembler::AssembleUnknownOp,
		/*0xA4*/ &RiverX86Assembler::AssembleUnknownOp, &RiverX86Assembler::AssembleUnknownOp, &RiverX86Assembler::AssembleUnknownOp, &RiverX86Assembler::AssembleUnknownOp,
		/*0xA8*/ &RiverX86Assembler::AssembleUnknownOp, &RiverX86Assembler::AssembleUnknownOp, &RiverX86Assembler::AssembleUnknownOp, &RiverX86Assembler::AssembleUnknownOp,
		/*0xAC*/ &RiverX86Assembler::AssembleUnknownOp, &RiverX86Assembler::AssembleUnknownOp, &RiverX86Assembler::AssembleUnknownOp, &RiverX86Assembler::AssembleUnknownOp,

		/*0xB0*/ &RiverX86Assembler::AssembleUnknownOp, &RiverX86Assembler::AssembleUnknownOp, &RiverX86Assembler::AssembleUnknownOp, &RiverX86Assembler::AssembleUnknownOp,
		/*0xB4*/ &RiverX86Assembler::AssembleUnknownOp, &RiverX86Assembler::AssembleUnknownOp, &RiverX86Assembler::AssembleUnknownOp, &RiverX86Assembler::AssembleUnknownOp,
		/*0xB8*/ &RiverX86Assembler::AssembleUnknownOp, &RiverX86Assembler::AssembleUnknownOp, &RiverX86Assembler::AssembleUnknownOp, &RiverX86Assembler::AssembleUnknownOp,
		/*0xBC*/ &RiverX86Assembler::AssembleUnknownOp, &RiverX86Assembler::AssembleUnknownOp, &RiverX86Assembler::AssembleUnknownOp, &RiverX86Assembler::AssembleUnknownOp,

		/*0xC0*/ &RiverX86Assembler::AssembleUnknownOp, &RiverX86Assembler::AssembleUnknownOp, &RiverX86Assembler::AssembleUnknownOp, &RiverX86Assembler::AssembleUnknownOp,
		/*0xC4*/ &RiverX86Assembler::AssembleUnknownOp, &RiverX86Assembler::AssembleUnknownOp, &RiverX86Assembler::AssembleUnknownOp, &RiverX86Assembler::AssembleUnknownOp,
		/*0xC8*/ &RiverX86Assembler::AssembleUnknownOp, &RiverX86Assembler::AssembleUnknownOp, &RiverX86Assembler::AssembleUnknownOp, &RiverX86Assembler::AssembleUnknownOp,
		/*0xCC*/ &RiverX86Assembler::AssembleUnknownOp, &RiverX86Assembler::AssembleUnknownOp, &RiverX86Assembler::AssembleUnknownOp, &RiverX86Assembler::AssembleUnknownOp,

		/*0xD0*/ &RiverX86Assembler::AssembleUnknownOp, &RiverX86Assembler::AssembleUnknownOp, &RiverX86Assembler::AssembleUnknownOp, &RiverX86Assembler::AssembleUnknownOp,
		/*0xD4*/ &RiverX86Assembler::AssembleUnknownOp, &RiverX86Assembler::AssembleUnknownOp, &RiverX86Assembler::AssembleUnknownOp, &RiverX86Assembler::AssembleUnknownOp,
		/*0xD8*/ &RiverX86Assembler::AssembleUnknownOp, &RiverX86Assembler::AssembleUnknownOp, &RiverX86Assembler::AssembleUnknownOp, &RiverX86Assembler::AssembleUnknownOp,
		/*0xDC*/ &RiverX86Assembler::AssembleUnknownOp, &RiverX86Assembler::AssembleUnknownOp, &RiverX86Assembler::AssembleUnknownOp, &RiverX86Assembler::AssembleUnknownOp,

		/*0xE0*/ &RiverX86Assembler::AssembleUnknownOp, &RiverX86Assembler::AssembleUnknownOp, &RiverX86Assembler::AssembleUnknownOp, &RiverX86Assembler::AssembleUnknownOp,
		/*0xE4*/ &RiverX86Assembler::AssembleUnknownOp, &RiverX86Assembler::AssembleUnknownOp, &RiverX86Assembler::AssembleUnknownOp, &RiverX86Assembler::AssembleUnknownOp,
		/*0xE8*/ &RiverX86Assembler::AssembleUnknownOp, &RiverX86Assembler::AssembleUnknownOp, &RiverX86Assembler::AssembleUnknownOp, &RiverX86Assembler::AssembleUnknownOp,
		/*0xEC*/ &RiverX86Assembler::AssembleUnknownOp, &RiverX86Assembler::AssembleUnknownOp, &RiverX86Assembler::AssembleUnknownOp, &RiverX86Assembler::AssembleUnknownOp,

		/*0xF0*/ &RiverX86Assembler::AssembleUnknownOp, &RiverX86Assembler::AssembleUnknownOp, &RiverX86Assembler::AssembleUnknownOp, &RiverX86Assembler::AssembleUnknownOp,
		/*0xF4*/ &RiverX86Assembler::AssembleUnknownOp, &RiverX86Assembler::AssembleUnknownOp, &RiverX86Assembler::AssembleUnknownOp, &RiverX86Assembler::AssembleUnknownOp,
		/*0xF8*/ &RiverX86Assembler::AssembleUnknownOp, &RiverX86Assembler::AssembleUnknownOp, &RiverX86Assembler::AssembleUnknownOp, &RiverX86Assembler::AssembleUnknownOp,
		/*0xFC*/ &RiverX86Assembler::AssembleUnknownOp, &RiverX86Assembler::AssembleUnknownOp, &RiverX86Assembler::AssembleUnknownOp, &RiverX86Assembler::AssembleUnknownOp
	} 
};