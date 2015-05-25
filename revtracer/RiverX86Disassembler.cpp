#include "RiverX86Disassembler.h"
#include "CodeGen.h"

extern const BYTE specTbl[2][0x100];
extern const BYTE _specTblExt[][8];

BYTE GetSpecifiers(RiverInstruction &ri) {
	//const WORD *specTbl = specTbl00;
	DWORD dwTable = (RIVER_MODIFIER_EXT & ri.modifiers) ? 1 : 0;
	BYTE tmp = specTbl[dwTable][ri.opCode];

	if (tmp == 0xFF) {
		__asm int 3;
	}

	if (tmp & RIVER_SPEC_MODIFIES_EXT) {
		tmp = _specTblExt[tmp & 0x7F][ri.subOpCode];
	}

	if (tmp == 0xFF) {
		__asm int 3;
	}

	return tmp;
}

void RiverX86Disassembler::TrackModifiedRegisters(RiverInstruction &ri) {
	//bool modifiysp = true;
	if (RIVER_SPEC_MODIFIES_xSP & ri.specifiers) {
		codegen->NextReg(RIVER_REG_xSP);
		//modifiysp = false;
	}

	if ((RIVER_SPEC_MODIFIES_OP2 & ri.specifiers) && (RIVER_OPTYPE_REG & ri.opTypes[1])) {
		ri.operands[1].asRegister.versioned = codegen->NextReg(ri.operands[1].asRegister.name);
	}

	if ((RIVER_SPEC_MODIFIES_OP1 & ri.specifiers) && (RIVER_OPTYPE_REG & ri.opTypes[0])) {
		ri.operands[0].asRegister.versioned = codegen->NextReg(ri.operands[0].asRegister.name);
	}
}

bool RiverX86Disassembler::Init(RiverCodeGen *cg) {
	codegen = cg;
	return true;
}

bool RiverX86Disassembler::Translate(BYTE *&px86, RiverInstruction &rOut, DWORD &flags) {
	DWORD dwTable = 0;
	
	rOut.modifiers = 0;
	rOut.specifiers = 0;
	rOut.subOpCode = 0;
	rOut.opTypes[0] = rOut.opTypes[1] = rOut.opTypes[2] = rOut.opTypes[3] = RIVER_OPTYPE_NONE;

	flags = 0;
	do {
		dwTable = (rOut.modifiers & RIVER_MODIFIER_EXT) ? 1 : 0;

		flags &= ~RIVER_FLAG_PFX;
		(this->*disassembleOpcodes[dwTable][*px86])(px86, rOut, flags);
	} while (flags & RIVER_FLAG_PFX);

	dwTable = (rOut.modifiers & RIVER_MODIFIER_EXT) ? 1 : 0;
	
	(this->*disassembleOperands[dwTable][rOut.opCode])(px86, rOut);
	TrackModifiedRegisters(rOut);
	rOut.TrackEspAsParameter();
	rOut.TrackUnusedRegisters();
	return true;
}

/* =========================================== */
/* Opcode disassemblers                        */
/* =========================================== */

void RiverX86Disassembler::DisassembleUnkInstr(BYTE *&px86, RiverInstruction &ri, DWORD &flags) {
	__asm int 3;
	px86++;
}

void RiverX86Disassembler::DisassembleExtInstr(BYTE *&px86, RiverInstruction &ri, DWORD &flags) {
	ri.opCode = *px86;
	px86++;

	ri.subOpCode = (*px86 >> 3) & 0x07;
	ri.specifiers = GetSpecifiers(ri);
}

/* =========================================== */
/* Operand helpers                             */
/* =========================================== */

void RiverX86Disassembler::DisassembleImmOp(BYTE opIdx, BYTE *&px86, RiverInstruction &ri, BYTE immSize) {
	ri.opTypes[opIdx] = RIVER_OPTYPE_IMM | immSize;
	switch (immSize) {
	case RIVER_OPSIZE_8:
		ri.operands[opIdx].asImm8 = *((BYTE *)px86);
		px86++;
		break;
	case RIVER_OPSIZE_16:
		ri.operands[opIdx].asImm16 = *((WORD *)px86);
		px86 += 2;
		break;
	case RIVER_OPSIZE_32:
		if (ri.modifiers & RIVER_MODIFIER_O16) {
			ri.operands[opIdx].asImm16 = *((WORD *)px86);
			px86 += 2;
		} else {
			ri.operands[opIdx].asImm32 = *((DWORD *)px86);
			px86 += 4;
		}
		break;
	}
}

void RiverX86Disassembler::DisassembleRegOp(BYTE opIdx, RiverInstruction &ri, BYTE reg) {
	BYTE opType = RIVER_OPTYPE_REG;
	if (RIVER_MODIFIER_O8 & ri.modifiers) {
		opType |= RIVER_OPSIZE_8;
	}
	else if (RIVER_MODIFIER_O16 & ri.modifiers) {
		opType |= RIVER_OPSIZE_16;
	}

	ri.opTypes[opIdx] = opType;
	ri.operands[opIdx].asRegister.versioned = codegen->GetCurrentReg(reg);
}

void RiverX86Disassembler::DisassembleModRMOp(BYTE opIdx, BYTE *&px86, RiverInstruction &ri, BYTE &extra) {
	RiverAddress *rAddr;
	BYTE mod = *px86 >> 6;
	BYTE rm = *px86 & 0x07;
	
	rAddr = codegen->AllocAddr(ri.modifiers); //new RiverAddress;
	rAddr->DecodeFromx86(*codegen, px86, extra, ri.modifiers);

	BYTE opType = RIVER_OPTYPE_MEM;
	if (RIVER_MODIFIER_O8 & ri.modifiers) {
		opType |= RIVER_OPSIZE_8;
	}
	else if (RIVER_MODIFIER_O16 & ri.modifiers) {
		opType |= RIVER_OPSIZE_16;
	}

	ri.opTypes[opIdx] = opType;
	ri.operands[opIdx].asAddress = rAddr;
}

void RiverX86Disassembler::DisassembleSzModRMOp(BYTE opIdx, BYTE *&px86, RiverInstruction &ri, BYTE &extra, WORD sz) {
	RiverAddress *rAddr;
	BYTE mod = *px86 >> 6;
	BYTE rm = *px86 & 0x07;

	rAddr = codegen->AllocAddr(ri.modifiers); //new RiverAddress;
	rAddr->DecodeFromx86(*codegen, px86, extra, ri.modifiers);

	BYTE opType = RIVER_OPTYPE_MEM;
	if (RIVER_MODIFIER_O8 & sz) {
		opType |= RIVER_OPSIZE_8;
	}
	else if (RIVER_MODIFIER_O16 & sz) {
		opType |= RIVER_OPSIZE_16;
	}

	ri.opTypes[opIdx] = opType;
	ri.operands[opIdx].asAddress = rAddr;
}

void RiverX86Disassembler::DisassembleMoffs8(BYTE opIdx, BYTE *&px86, RiverInstruction &ri) {
	RiverAddress *rAddr;

	rAddr = codegen->AllocAddr(ri.modifiers); //new RiverAddress;
	rAddr->DecodeFlags(ri.modifiers); 
	rAddr->type = RIVER_ADDR_DISP8 | RIVER_ADDR_DIRTY;
	rAddr->disp.d8 = *px86;

	BYTE opType = RIVER_OPTYPE_MEM;
	if (RIVER_MODIFIER_O8 & ri.modifiers) {
		opType |= RIVER_OPSIZE_8;
	}
	else if (RIVER_MODIFIER_O16 & ri.modifiers) {
		opType |= RIVER_OPSIZE_16;
	}

	px86++;

	ri.opTypes[opIdx] = opType;
	ri.operands[opIdx].asAddress = rAddr;
}

void RiverX86Disassembler::DisassembleMoffs32(BYTE opIdx, BYTE *&px86, RiverInstruction &ri) {
	RiverAddress *rAddr;

	rAddr = codegen->AllocAddr(ri.modifiers); //new RiverAddress;
	rAddr->scaleAndSegment = 0;
	rAddr->DecodeFlags(ri.modifiers);
	rAddr->type = RIVER_ADDR_DISP | RIVER_ADDR_DIRTY;
	
	if (ri.modifiers & RIVER_MODIFIER_A16) {
		rAddr->disp.d32 = *(WORD *)px86;
		px86 += 2;
	} else {
		rAddr->disp.d32 = *(DWORD *)px86;
		px86 += 4;
	}
	
	BYTE opType = RIVER_OPTYPE_MEM;
	if (RIVER_MODIFIER_O8 & ri.modifiers) {
		opType |= RIVER_OPSIZE_8;
	}
	else if (RIVER_MODIFIER_O16 & ri.modifiers) {
		opType |= RIVER_OPSIZE_16;
	}

	ri.opTypes[opIdx] = opType;
	ri.operands[opIdx].asAddress = rAddr;
}

/* =========================================== */
/* Operand disassemblers                       */
/* =========================================== */

void RiverX86Disassembler::DisassembleUnkOp(BYTE *&px86, RiverInstruction &ri) {
	__asm int 3;
}

void RiverX86Disassembler::DisassembleNoOp(BYTE *&px86, RiverInstruction &ri) {
}

void RiverX86Disassembler::DisassembleRegModRM(BYTE *&px86, RiverInstruction &ri) {
	BYTE sec;
	DisassembleModRMOp(1, px86, ri, sec);
	DisassembleRegOp(0, ri, sec);
}

void RiverX86Disassembler::DisassembleModRMReg(BYTE *&px86, RiverInstruction &ri) {
	BYTE sec;
	DisassembleModRMOp(0, px86, ri, sec);
	DisassembleRegOp(1, ri, sec);
}

void RiverX86Disassembler::DisassembleModRMImm8(BYTE *&px86, RiverInstruction &ri) {
	BYTE sec;
	DisassembleModRMOp(0, px86, ri, sec);
	DisassembleImmOp(1, px86, ri, RIVER_OPSIZE_8);
}

void RiverX86Disassembler::DisassembleModRMImm32(BYTE *&px86, RiverInstruction &ri) {
	BYTE sec;
	DisassembleModRMOp(0, px86, ri, sec);
	DisassembleImmOp(1, px86, ri, RIVER_OPSIZE_32);
}

void RiverX86Disassembler::DisassembleSubOpModRM(BYTE *&px86, RiverInstruction &ri) {
	DisassembleModRMOp(0, px86, ri, ri.subOpCode);
}

void RiverX86Disassembler::DisassembleSubOpModRMImm8(BYTE *&px86, RiverInstruction &ri) {
	DisassembleModRMOp(0, px86, ri, ri.subOpCode);
	DisassembleImmOp(1, px86, ri, RIVER_OPSIZE_8);
}

void RiverX86Disassembler::DisassembleSubOpModRMImm32(BYTE *&px86, RiverInstruction &ri) {
	DisassembleModRMOp(0, px86, ri, ri.subOpCode);
	DisassembleImmOp(1, px86, ri, RIVER_OPSIZE_32);
}

void RiverX86Disassembler::DisassembleImm8(BYTE *&px86, RiverInstruction &ri) {
	DisassembleImmOp(0, px86, ri, RIVER_OPSIZE_8);
}

void RiverX86Disassembler::DisassembleImm16(BYTE *&px86, RiverInstruction &ri) {
	DisassembleImmOp(0, px86, ri, RIVER_OPSIZE_16);
}

void RiverX86Disassembler::DisassembleImm32(BYTE *&px86, RiverInstruction &ri) {
	DisassembleImmOp(0, px86, ri, RIVER_OPSIZE_32);
}

void RiverX86Disassembler::DisassembleImm32Imm16(BYTE *&px86, RiverInstruction &ri) {
	DisassembleImmOp(0, px86, ri, RIVER_OPSIZE_32);
	DisassembleImmOp(1, px86, ri, RIVER_OPSIZE_16);
}

void RiverX86Disassembler::DisassembleRegModRMImm8(BYTE *&px86, RiverInstruction &ri) {
	BYTE sec;
	DisassembleModRMOp(1, px86, ri, sec);
	DisassembleRegOp(0, ri, sec);
	DisassembleImmOp(2, px86, ri, RIVER_OPSIZE_8);
}

void RiverX86Disassembler::DisassembleRegModRMImm32(BYTE *&px86, RiverInstruction &ri) {
	BYTE sec;
	DisassembleModRMOp(1, px86, ri, sec);
	DisassembleRegOp(0, ri, sec);
	DisassembleImmOp(2, px86, ri, RIVER_OPSIZE_32);
}

/* =========================================== */
/* Disassembly tables                          */
/* =========================================== */

RiverX86Disassembler::DisassembleOpcodeFunc RiverX86Disassembler::disassembleOpcodes[2][0x100] = {
	{
		/*0x00*/ &RiverX86Disassembler::DisassembleDefaultInstr<RIVER_MODIFIER_O8>, &RiverX86Disassembler::DisassembleDefaultInstr, &RiverX86Disassembler::DisassembleDefaultInstr<RIVER_MODIFIER_O8>, &RiverX86Disassembler::DisassembleDefaultInstr,
		/*0x04*/ &RiverX86Disassembler::DisassembleDefaultRegInstr<RIVER_REG_AL, RIVER_MODIFIER_O8>, &RiverX86Disassembler::DisassembleDefaultRegInstr<RIVER_REG_xAX>, &RiverX86Disassembler::DisassembleDefaultRegInstr<RIVER_REG_ES>, &RiverX86Disassembler::DisassembleDefaultRegInstr<RIVER_REG_ES>,
		/*0x08*/ &RiverX86Disassembler::DisassembleDefaultInstr<RIVER_MODIFIER_O8>, &RiverX86Disassembler::DisassembleDefaultInstr, &RiverX86Disassembler::DisassembleDefaultInstr<RIVER_MODIFIER_O8>, &RiverX86Disassembler::DisassembleDefaultInstr,
		/*0x0C*/ &RiverX86Disassembler::DisassembleDefaultRegInstr<RIVER_REG_AL, RIVER_MODIFIER_O8>, &RiverX86Disassembler::DisassembleDefaultRegInstr<RIVER_REG_xAX>, &RiverX86Disassembler::DisassembleDefaultRegInstr<RIVER_REG_CS>, &RiverX86Disassembler::DisassembleDefaultInstr<RIVER_MODIFIER_EXT, RIVER_FLAG_PFX>,

		/*0x10*/ &RiverX86Disassembler::DisassembleDefaultInstr<RIVER_MODIFIER_O8>, &RiverX86Disassembler::DisassembleDefaultInstr, &RiverX86Disassembler::DisassembleDefaultInstr<RIVER_MODIFIER_O8>, &RiverX86Disassembler::DisassembleDefaultInstr,
		/*0x14*/ &RiverX86Disassembler::DisassembleDefaultRegInstr<RIVER_REG_AL, RIVER_MODIFIER_O8>, &RiverX86Disassembler::DisassembleDefaultRegInstr<RIVER_REG_xAX>, &RiverX86Disassembler::DisassembleDefaultRegInstr<RIVER_REG_SS>, &RiverX86Disassembler::DisassembleDefaultRegInstr<RIVER_REG_SS>,
		/*0x18*/ &RiverX86Disassembler::DisassembleDefaultInstr<RIVER_MODIFIER_O8>, &RiverX86Disassembler::DisassembleDefaultInstr, &RiverX86Disassembler::DisassembleDefaultInstr<RIVER_MODIFIER_O8>, &RiverX86Disassembler::DisassembleDefaultInstr,
		/*0x1C*/ &RiverX86Disassembler::DisassembleDefaultRegInstr<RIVER_REG_AL, RIVER_MODIFIER_O8>, &RiverX86Disassembler::DisassembleDefaultRegInstr<RIVER_REG_xAX>, &RiverX86Disassembler::DisassembleDefaultRegInstr<RIVER_REG_DS>, &RiverX86Disassembler::DisassembleDefaultRegInstr<RIVER_REG_DS>,

		/*0x20*/ &RiverX86Disassembler::DisassembleDefaultInstr<RIVER_MODIFIER_O8>, &RiverX86Disassembler::DisassembleDefaultInstr, &RiverX86Disassembler::DisassembleDefaultInstr<RIVER_MODIFIER_O8>, &RiverX86Disassembler::DisassembleDefaultInstr,
		/*0x24*/ &RiverX86Disassembler::DisassembleDefaultRegInstr<RIVER_REG_AL, RIVER_MODIFIER_O8>, &RiverX86Disassembler::DisassembleDefaultRegInstr<RIVER_REG_xAX>, &RiverX86Disassembler::DisassembleSegInstr<RIVER_MODIFIER_ESSEG>, &RiverX86Disassembler::DisassembleDefaultRegInstr<RIVER_REG_AL, RIVER_MODIFIER_O8>,
		/*0x28*/ &RiverX86Disassembler::DisassembleDefaultInstr<RIVER_MODIFIER_O8>, &RiverX86Disassembler::DisassembleDefaultInstr, &RiverX86Disassembler::DisassembleDefaultInstr<RIVER_MODIFIER_O8>, &RiverX86Disassembler::DisassembleDefaultInstr,
		/*0x2C*/ &RiverX86Disassembler::DisassembleDefaultRegInstr<RIVER_REG_AL, RIVER_MODIFIER_O8>, &RiverX86Disassembler::DisassembleDefaultRegInstr<RIVER_REG_xAX>, &RiverX86Disassembler::DisassembleSegInstr<RIVER_MODIFIER_CSSEG>, &RiverX86Disassembler::DisassembleDefaultRegInstr<RIVER_REG_AL, RIVER_MODIFIER_O8>,

		/*0x30*/ &RiverX86Disassembler::DisassembleDefaultInstr<RIVER_MODIFIER_O8>, &RiverX86Disassembler::DisassembleDefaultInstr, &RiverX86Disassembler::DisassembleDefaultInstr<RIVER_MODIFIER_O8>, &RiverX86Disassembler::DisassembleDefaultInstr,
		/*0x34*/ &RiverX86Disassembler::DisassembleDefaultRegInstr<RIVER_REG_AL, RIVER_MODIFIER_O8>, &RiverX86Disassembler::DisassembleDefaultRegInstr<RIVER_REG_xAX>, &RiverX86Disassembler::DisassembleSegInstr<RIVER_MODIFIER_SSSEG>, &RiverX86Disassembler::DisassembleDefault2RegInstr<RIVER_REG_AL, RIVER_REG_AH, RIVER_MODIFIER_O8>,
		/*0x38*/ &RiverX86Disassembler::DisassembleDefaultInstr<RIVER_MODIFIER_O8>, &RiverX86Disassembler::DisassembleDefaultInstr, &RiverX86Disassembler::DisassembleDefaultInstr<RIVER_MODIFIER_O8>, &RiverX86Disassembler::DisassembleDefaultInstr,
		/*0x3C*/ &RiverX86Disassembler::DisassembleDefaultRegInstr<RIVER_REG_AL, RIVER_MODIFIER_O8>, &RiverX86Disassembler::DisassembleDefaultRegInstr<RIVER_REG_xAX>, &RiverX86Disassembler::DisassembleSegInstr<RIVER_MODIFIER_DSSEG>, &RiverX86Disassembler::DisassembleDefault2RegInstr<RIVER_REG_AL, RIVER_REG_AH, RIVER_MODIFIER_O8>,

		/*0x40*/ &RiverX86Disassembler::DisassemblePlusRegInstr<0x40>, &RiverX86Disassembler::DisassemblePlusRegInstr<0x40>, &RiverX86Disassembler::DisassemblePlusRegInstr<0x40>, &RiverX86Disassembler::DisassemblePlusRegInstr<0x40>,
		/*0x44*/ &RiverX86Disassembler::DisassemblePlusRegInstr<0x40>, &RiverX86Disassembler::DisassemblePlusRegInstr<0x40>, &RiverX86Disassembler::DisassemblePlusRegInstr<0x40>, &RiverX86Disassembler::DisassemblePlusRegInstr<0x40>,
		/*0x48*/ &RiverX86Disassembler::DisassemblePlusRegInstr<0x48>, &RiverX86Disassembler::DisassemblePlusRegInstr<0x48>, &RiverX86Disassembler::DisassemblePlusRegInstr<0x48>, &RiverX86Disassembler::DisassemblePlusRegInstr<0x48>,
		/*0x4C*/ &RiverX86Disassembler::DisassemblePlusRegInstr<0x48>, &RiverX86Disassembler::DisassemblePlusRegInstr<0x48>, &RiverX86Disassembler::DisassemblePlusRegInstr<0x48>, &RiverX86Disassembler::DisassemblePlusRegInstr<0x48>,

		/*0x50*/ &RiverX86Disassembler::DisassemblePlusRegInstr<0x50>, &RiverX86Disassembler::DisassemblePlusRegInstr<0x50>, &RiverX86Disassembler::DisassemblePlusRegInstr<0x50>, &RiverX86Disassembler::DisassemblePlusRegInstr<0x50>,
		/*0x54*/ &RiverX86Disassembler::DisassemblePlusRegInstr<0x50>, &RiverX86Disassembler::DisassemblePlusRegInstr<0x50>, &RiverX86Disassembler::DisassemblePlusRegInstr<0x50>, &RiverX86Disassembler::DisassemblePlusRegInstr<0x50>,
		/*0x58*/ &RiverX86Disassembler::DisassemblePlusRegInstr<0x58>, &RiverX86Disassembler::DisassemblePlusRegInstr<0x58>, &RiverX86Disassembler::DisassemblePlusRegInstr<0x58>, &RiverX86Disassembler::DisassemblePlusRegInstr<0x58>,
		/*0x5C*/ &RiverX86Disassembler::DisassemblePlusRegInstr<0x58>, &RiverX86Disassembler::DisassemblePlusRegInstr<0x58>, &RiverX86Disassembler::DisassemblePlusRegInstr<0x58>, &RiverX86Disassembler::DisassemblePlusRegInstr<0x58>,

		/*0x60*/ &RiverX86Disassembler::DisassembleUnkInstr, &RiverX86Disassembler::DisassembleUnkInstr, &RiverX86Disassembler::DisassembleUnkInstr, &RiverX86Disassembler::DisassembleUnkInstr,
		/*0x64*/ &RiverX86Disassembler::DisassembleSegInstr<RIVER_MODIFIER_FSSEG>, &RiverX86Disassembler::DisassembleSegInstr<RIVER_MODIFIER_GSSEG>, &RiverX86Disassembler::DisassembleDefaultInstr<RIVER_MODIFIER_O16, RIVER_FLAG_PFX>, &RiverX86Disassembler::DisassembleDefaultInstr<RIVER_MODIFIER_A16, RIVER_FLAG_PFX>,
		/*0x68*/ &RiverX86Disassembler::DisassembleDefaultInstr, &RiverX86Disassembler::DisassembleDefaultInstr, &RiverX86Disassembler::DisassembleDefaultInstr, &RiverX86Disassembler::DisassembleDefaultInstr,
		/*0x6C*/ &RiverX86Disassembler::DisassembleUnkInstr, &RiverX86Disassembler::DisassembleUnkInstr, &RiverX86Disassembler::DisassembleUnkInstr, &RiverX86Disassembler::DisassembleUnkInstr,

		/*0x70*/ &RiverX86Disassembler::DisassembleRelJmpInstr<2>, &RiverX86Disassembler::DisassembleRelJmpInstr<2>, &RiverX86Disassembler::DisassembleRelJmpInstr<2>, &RiverX86Disassembler::DisassembleRelJmpInstr<2>,
		/*0x74*/ &RiverX86Disassembler::DisassembleRelJmpInstr<2>, &RiverX86Disassembler::DisassembleRelJmpInstr<2>, &RiverX86Disassembler::DisassembleRelJmpInstr<2>, &RiverX86Disassembler::DisassembleRelJmpInstr<2>,
		/*0x78*/ &RiverX86Disassembler::DisassembleRelJmpInstr<2>, &RiverX86Disassembler::DisassembleRelJmpInstr<2>, &RiverX86Disassembler::DisassembleRelJmpInstr<2>, &RiverX86Disassembler::DisassembleRelJmpInstr<2>,
		/*0x7C*/ &RiverX86Disassembler::DisassembleRelJmpInstr<2>, &RiverX86Disassembler::DisassembleRelJmpInstr<2>, &RiverX86Disassembler::DisassembleRelJmpInstr<2>, &RiverX86Disassembler::DisassembleRelJmpInstr<2>,

		/*0x80*/ &RiverX86Disassembler::DisassembleDefaultInstr<RIVER_MODIFIER_O8>, &RiverX86Disassembler::DisassembleDefaultInstr, &RiverX86Disassembler::DisassembleDefaultInstr<RIVER_MODIFIER_O8>, &RiverX86Disassembler::DisassembleExtInstr,
		/*0x84*/ &RiverX86Disassembler::DisassembleDefaultInstr<RIVER_MODIFIER_O8>, &RiverX86Disassembler::DisassembleDefaultInstr, &RiverX86Disassembler::DisassembleDefaultInstr<RIVER_MODIFIER_O8>, &RiverX86Disassembler::DisassembleDefaultInstr,
		/*0x88*/ &RiverX86Disassembler::DisassembleDefaultInstr<RIVER_MODIFIER_O8>, &RiverX86Disassembler::DisassembleDefaultInstr, &RiverX86Disassembler::DisassembleDefaultInstr<RIVER_MODIFIER_O8>, &RiverX86Disassembler::DisassembleDefaultInstr,
		/*0x8C*/ &RiverX86Disassembler::DisassembleUnkInstr, &RiverX86Disassembler::DisassembleDefaultInstr, &RiverX86Disassembler::DisassembleUnkInstr, &RiverX86Disassembler::DisassembleUnkInstr,

		/*0x90*/ &RiverX86Disassembler::DisassemblePlusRegInstr<0x90>, &RiverX86Disassembler::DisassemblePlusRegInstr<0x90>, &RiverX86Disassembler::DisassemblePlusRegInstr<0x90>, &RiverX86Disassembler::DisassemblePlusRegInstr<0x90>,
		/*0x94*/ &RiverX86Disassembler::DisassemblePlusRegInstr<0x90>, &RiverX86Disassembler::DisassemblePlusRegInstr<0x90>, &RiverX86Disassembler::DisassemblePlusRegInstr<0x90>, &RiverX86Disassembler::DisassemblePlusRegInstr<0x90>,
		/*0x98*/ &RiverX86Disassembler::DisassembleUnkInstr, &RiverX86Disassembler::DisassembleDefaultInstr, &RiverX86Disassembler::DisassembleUnkInstr, &RiverX86Disassembler::DisassembleUnkInstr,
		/*0x9C*/ &RiverX86Disassembler::DisassembleUnkInstr, &RiverX86Disassembler::DisassembleUnkInstr, &RiverX86Disassembler::DisassembleUnkInstr, &RiverX86Disassembler::DisassembleUnkInstr,

		/*0xA0*/ &RiverX86Disassembler::DisassembleDefaultRegInstr<RIVER_REG_AL, RIVER_MODIFIER_O8>, &RiverX86Disassembler::DisassembleDefaultRegInstr<RIVER_REG_xAX>, &RiverX86Disassembler::DisassembleDefaultSecondRegInstr<RIVER_REG_AL, RIVER_MODIFIER_O8>, &RiverX86Disassembler::DisassembleDefaultSecondRegInstr<RIVER_REG_xAX>,
		/*0xA4*/ &RiverX86Disassembler::DisassembleDefaultRegInstr<RIVER_MODIFIER_O8>, &RiverX86Disassembler::DisassembleDefaultInstr, &RiverX86Disassembler::DisassembleDefaultInstr<RIVER_MODIFIER_O8>, &RiverX86Disassembler::DisassembleDefaultInstr,
		/*0xA8*/ &RiverX86Disassembler::DisassembleDefaultRegInstr<RIVER_REG_AL, RIVER_MODIFIER_O8>, &RiverX86Disassembler::DisassembleDefaultRegInstr<RIVER_REG_xAX>, &RiverX86Disassembler::DisassembleDefaultInstr<RIVER_OPSIZE_8>, &RiverX86Disassembler::DisassembleDefaultInstr,
		/*0xAC*/ &RiverX86Disassembler::DisassembleUnkInstr, &RiverX86Disassembler::DisassembleUnkInstr, &RiverX86Disassembler::DisassembleDefaultInstr<RIVER_MODIFIER_O8>, &RiverX86Disassembler::DisassembleDefaultInstr,

		/*0xB0*/ &RiverX86Disassembler::DisassemblePlusRegInstr<0xB0, RIVER_MODIFIER_O8>, &RiverX86Disassembler::DisassemblePlusRegInstr<0xB0, RIVER_MODIFIER_O8>, &RiverX86Disassembler::DisassemblePlusRegInstr<0xB0, RIVER_MODIFIER_O8>, &RiverX86Disassembler::DisassemblePlusRegInstr<0xB0, RIVER_MODIFIER_O8>,
		/*0xB4*/ &RiverX86Disassembler::DisassemblePlusRegInstr<0xB0, RIVER_MODIFIER_O8>, &RiverX86Disassembler::DisassemblePlusRegInstr<0xB0, RIVER_MODIFIER_O8>, &RiverX86Disassembler::DisassemblePlusRegInstr<0xB0, RIVER_MODIFIER_O8>, &RiverX86Disassembler::DisassemblePlusRegInstr<0xB0, RIVER_MODIFIER_O8>,
		/*0xB8*/ &RiverX86Disassembler::DisassemblePlusRegInstr<0xB8>, &RiverX86Disassembler::DisassemblePlusRegInstr<0xB8>, &RiverX86Disassembler::DisassemblePlusRegInstr<0xB8>, &RiverX86Disassembler::DisassemblePlusRegInstr<0xB8>,
		/*0xBC*/ &RiverX86Disassembler::DisassemblePlusRegInstr<0xB8>, &RiverX86Disassembler::DisassemblePlusRegInstr<0xB8>, &RiverX86Disassembler::DisassemblePlusRegInstr<0xB8>, &RiverX86Disassembler::DisassemblePlusRegInstr<0xB8>,

		/*0xC0*/ &RiverX86Disassembler::DisassembleDefaultInstr<RIVER_MODIFIER_O8>, &RiverX86Disassembler::DisassembleDefaultInstr, &RiverX86Disassembler::DisassembleDefaultInstr<0, RIVER_FLAG_BRANCH | RIVER_MODIFIER_O16>, &RiverX86Disassembler::DisassembleDefaultInstr<0, RIVER_FLAG_BRANCH>,
		/*0xC4*/ &RiverX86Disassembler::DisassembleUnkInstr, &RiverX86Disassembler::DisassembleUnkInstr, &RiverX86Disassembler::DisassembleDefaultInstr<RIVER_MODIFIER_O8>, &RiverX86Disassembler::DisassembleDefaultInstr,
		/*0xC8*/ &RiverX86Disassembler::DisassembleUnkInstr, &RiverX86Disassembler::DisassembleDefaultInstr, &RiverX86Disassembler::DisassembleUnkInstr, &RiverX86Disassembler::DisassembleUnkInstr,
		/*0xCC*/ &RiverX86Disassembler::DisassembleUnkInstr, &RiverX86Disassembler::DisassembleDefaultInstr, &RiverX86Disassembler::DisassembleUnkInstr, &RiverX86Disassembler::DisassembleUnkInstr,

		/*0xD0*/ &RiverX86Disassembler::DisassembleDefaultInstr<RIVER_MODIFIER_O8>, &RiverX86Disassembler::DisassembleDefaultInstr, &RiverX86Disassembler::DisassembleDefaultSecondRegInstr<RIVER_REG_CL, RIVER_MODIFIER_O8>, &RiverX86Disassembler::DisassembleDefaultSecondRegInstr<RIVER_REG_CL>,
		/*0xD4*/ &RiverX86Disassembler::DisassembleUnkInstr, &RiverX86Disassembler::DisassembleUnkInstr, &RiverX86Disassembler::DisassembleUnkInstr, &RiverX86Disassembler::DisassembleUnkInstr,
		/*0xD8*/ &RiverX86Disassembler::DisassembleUnkInstr, &RiverX86Disassembler::DisassembleUnkInstr, &RiverX86Disassembler::DisassembleUnkInstr, &RiverX86Disassembler::DisassembleUnkInstr,
		/*0xDC*/ &RiverX86Disassembler::DisassembleUnkInstr, &RiverX86Disassembler::DisassembleUnkInstr, &RiverX86Disassembler::DisassembleUnkInstr, &RiverX86Disassembler::DisassembleUnkInstr,

		/*0xE0*/ &RiverX86Disassembler::DisassembleUnkInstr, &RiverX86Disassembler::DisassembleUnkInstr, &RiverX86Disassembler::DisassembleUnkInstr, &RiverX86Disassembler::DisassembleUnkInstr,
		/*0xE4*/ &RiverX86Disassembler::DisassembleUnkInstr, &RiverX86Disassembler::DisassembleUnkInstr, &RiverX86Disassembler::DisassembleUnkInstr, &RiverX86Disassembler::DisassembleUnkInstr,
		/*0xE8*/ &RiverX86Disassembler::DisassembleRelJmpInstr<5>, &RiverX86Disassembler::DisassembleRelJmpInstr<5>, &RiverX86Disassembler::DisassembleDefaultInstr<0, RIVER_FLAG_BRANCH>, &RiverX86Disassembler::DisassembleRelJmpInstr<2>,
		/*0xEC*/ &RiverX86Disassembler::DisassembleUnkInstr, &RiverX86Disassembler::DisassembleUnkInstr, &RiverX86Disassembler::DisassembleUnkInstr, &RiverX86Disassembler::DisassembleUnkInstr,

		/*0xF0*/ &RiverX86Disassembler::DisassembleDefaultInstr<RIVER_MODIFIER_LOCK, RIVER_FLAG_PFX>, &RiverX86Disassembler::DisassembleUnkInstr, &RiverX86Disassembler::DisassembleDefaultInstr<RIVER_MODIFIER_REPNZ, RIVER_FLAG_PFX>, &RiverX86Disassembler::DisassembleDefaultInstr<RIVER_MODIFIER_REPZ, RIVER_FLAG_PFX>,
		/*0xF4*/ &RiverX86Disassembler::DisassembleUnkInstr, &RiverX86Disassembler::DisassembleUnkInstr, &RiverX86Disassembler::DisassembleSubOpInstr<disassemble0xF6Instr>, &RiverX86Disassembler::DisassembleSubOpInstr<disassemble0xF7Instr>,
		/*0xF8*/ &RiverX86Disassembler::DisassembleUnkInstr, &RiverX86Disassembler::DisassembleUnkInstr, &RiverX86Disassembler::DisassembleUnkInstr, &RiverX86Disassembler::DisassembleUnkInstr,
		/*0xFC*/ &RiverX86Disassembler::DisassembleDefaultInstr, &RiverX86Disassembler::DisassembleDefaultInstr, &RiverX86Disassembler::DisassembleDefaultInstr<RIVER_MODIFIER_O8>, &RiverX86Disassembler::DisassembleSubOpInstr<disassemble0xFFInstr>
	}, {
		/*0x00*/ &RiverX86Disassembler::DisassembleUnkInstr, &RiverX86Disassembler::DisassembleUnkInstr, &RiverX86Disassembler::DisassembleDefaultInstr, &RiverX86Disassembler::DisassembleDefaultInstr,
		/*0x04*/ &RiverX86Disassembler::DisassembleUnkInstr, &RiverX86Disassembler::DisassembleUnkInstr, &RiverX86Disassembler::DisassembleUnkInstr, &RiverX86Disassembler::DisassembleUnkInstr,
		/*0x08*/ &RiverX86Disassembler::DisassembleUnkInstr, &RiverX86Disassembler::DisassembleUnkInstr, &RiverX86Disassembler::DisassembleUnkInstr, &RiverX86Disassembler::DisassembleUnkInstr,
		/*0x0C*/ &RiverX86Disassembler::DisassembleUnkInstr, &RiverX86Disassembler::DisassembleUnkInstr, &RiverX86Disassembler::DisassembleUnkInstr, &RiverX86Disassembler::DisassembleUnkInstr,

		/*0x10*/ &RiverX86Disassembler::DisassembleUnkInstr, &RiverX86Disassembler::DisassembleUnkInstr, &RiverX86Disassembler::DisassembleUnkInstr, &RiverX86Disassembler::DisassembleUnkInstr,
		/*0x14*/ &RiverX86Disassembler::DisassembleUnkInstr, &RiverX86Disassembler::DisassembleUnkInstr, &RiverX86Disassembler::DisassembleUnkInstr, &RiverX86Disassembler::DisassembleUnkInstr,
		/*0x18*/ &RiverX86Disassembler::DisassembleUnkInstr, &RiverX86Disassembler::DisassembleUnkInstr, &RiverX86Disassembler::DisassembleUnkInstr, &RiverX86Disassembler::DisassembleUnkInstr,
		/*0x1C*/ &RiverX86Disassembler::DisassembleUnkInstr, &RiverX86Disassembler::DisassembleUnkInstr, &RiverX86Disassembler::DisassembleUnkInstr, &RiverX86Disassembler::DisassembleUnkInstr,

		/*0x20*/ &RiverX86Disassembler::DisassembleUnkInstr, &RiverX86Disassembler::DisassembleUnkInstr, &RiverX86Disassembler::DisassembleUnkInstr, &RiverX86Disassembler::DisassembleUnkInstr,
		/*0x24*/ &RiverX86Disassembler::DisassembleUnkInstr, &RiverX86Disassembler::DisassembleUnkInstr, &RiverX86Disassembler::DisassembleUnkInstr, &RiverX86Disassembler::DisassembleUnkInstr,
		/*0x28*/ &RiverX86Disassembler::DisassembleUnkInstr, &RiverX86Disassembler::DisassembleUnkInstr, &RiverX86Disassembler::DisassembleUnkInstr, &RiverX86Disassembler::DisassembleUnkInstr,
		/*0x2C*/ &RiverX86Disassembler::DisassembleUnkInstr, &RiverX86Disassembler::DisassembleUnkInstr, &RiverX86Disassembler::DisassembleUnkInstr, &RiverX86Disassembler::DisassembleUnkInstr,

		/*0x30*/ &RiverX86Disassembler::DisassembleUnkInstr, &RiverX86Disassembler::DisassembleDefaultInstr, &RiverX86Disassembler::DisassembleUnkInstr, &RiverX86Disassembler::DisassembleUnkInstr,
		/*0x34*/ &RiverX86Disassembler::DisassembleUnkInstr, &RiverX86Disassembler::DisassembleUnkInstr, &RiverX86Disassembler::DisassembleUnkInstr, &RiverX86Disassembler::DisassembleUnkInstr,
		/*0x38*/ &RiverX86Disassembler::DisassembleUnkInstr, &RiverX86Disassembler::DisassembleUnkInstr, &RiverX86Disassembler::DisassembleUnkInstr, &RiverX86Disassembler::DisassembleUnkInstr,
		/*0x3C*/ &RiverX86Disassembler::DisassembleUnkInstr, &RiverX86Disassembler::DisassembleUnkInstr, &RiverX86Disassembler::DisassembleUnkInstr, &RiverX86Disassembler::DisassembleUnkInstr,

		/*0x40*/ &RiverX86Disassembler::DisassembleDefaultInstr, &RiverX86Disassembler::DisassembleDefaultInstr, &RiverX86Disassembler::DisassembleDefaultInstr, &RiverX86Disassembler::DisassembleDefaultInstr,
		/*0x44*/ &RiverX86Disassembler::DisassembleDefaultInstr, &RiverX86Disassembler::DisassembleDefaultInstr, &RiverX86Disassembler::DisassembleDefaultInstr, &RiverX86Disassembler::DisassembleDefaultInstr,
		/*0x48*/ &RiverX86Disassembler::DisassembleDefaultInstr, &RiverX86Disassembler::DisassembleDefaultInstr, &RiverX86Disassembler::DisassembleDefaultInstr, &RiverX86Disassembler::DisassembleDefaultInstr,
		/*0x4C*/ &RiverX86Disassembler::DisassembleDefaultInstr, &RiverX86Disassembler::DisassembleDefaultInstr, &RiverX86Disassembler::DisassembleDefaultInstr, &RiverX86Disassembler::DisassembleDefaultInstr,

		/*0x50*/ &RiverX86Disassembler::DisassembleUnkInstr, &RiverX86Disassembler::DisassembleUnkInstr, &RiverX86Disassembler::DisassembleUnkInstr, &RiverX86Disassembler::DisassembleUnkInstr,
		/*0x54*/ &RiverX86Disassembler::DisassembleUnkInstr, &RiverX86Disassembler::DisassembleUnkInstr, &RiverX86Disassembler::DisassembleUnkInstr, &RiverX86Disassembler::DisassembleUnkInstr,
		/*0x58*/ &RiverX86Disassembler::DisassembleUnkInstr, &RiverX86Disassembler::DisassembleUnkInstr, &RiverX86Disassembler::DisassembleUnkInstr, &RiverX86Disassembler::DisassembleUnkInstr,
		/*0x5C*/ &RiverX86Disassembler::DisassembleUnkInstr, &RiverX86Disassembler::DisassembleUnkInstr, &RiverX86Disassembler::DisassembleUnkInstr, &RiverX86Disassembler::DisassembleUnkInstr,

		/*0x60*/ &RiverX86Disassembler::DisassembleUnkInstr, &RiverX86Disassembler::DisassembleUnkInstr, &RiverX86Disassembler::DisassembleUnkInstr, &RiverX86Disassembler::DisassembleUnkInstr,
		/*0x64*/ &RiverX86Disassembler::DisassembleUnkInstr, &RiverX86Disassembler::DisassembleUnkInstr, &RiverX86Disassembler::DisassembleUnkInstr, &RiverX86Disassembler::DisassembleUnkInstr,
		/*0x68*/ &RiverX86Disassembler::DisassembleUnkInstr, &RiverX86Disassembler::DisassembleUnkInstr, &RiverX86Disassembler::DisassembleUnkInstr, &RiverX86Disassembler::DisassembleUnkInstr,
		/*0x6C*/ &RiverX86Disassembler::DisassembleUnkInstr, &RiverX86Disassembler::DisassembleUnkInstr, &RiverX86Disassembler::DisassembleUnkInstr, &RiverX86Disassembler::DisassembleUnkInstr,

		/*0x70*/ &RiverX86Disassembler::DisassembleUnkInstr, &RiverX86Disassembler::DisassembleUnkInstr, &RiverX86Disassembler::DisassembleUnkInstr, &RiverX86Disassembler::DisassembleUnkInstr,
		/*0x74*/ &RiverX86Disassembler::DisassembleUnkInstr, &RiverX86Disassembler::DisassembleUnkInstr, &RiverX86Disassembler::DisassembleUnkInstr, &RiverX86Disassembler::DisassembleUnkInstr,
		/*0x78*/ &RiverX86Disassembler::DisassembleUnkInstr, &RiverX86Disassembler::DisassembleUnkInstr, &RiverX86Disassembler::DisassembleUnkInstr, &RiverX86Disassembler::DisassembleUnkInstr,
		/*0x7C*/ &RiverX86Disassembler::DisassembleUnkInstr, &RiverX86Disassembler::DisassembleUnkInstr, &RiverX86Disassembler::DisassembleUnkInstr, &RiverX86Disassembler::DisassembleUnkInstr,

		/*0x80*/ &RiverX86Disassembler::DisassembleRelJmpInstr<5>, &RiverX86Disassembler::DisassembleRelJmpInstr<5>, &RiverX86Disassembler::DisassembleRelJmpInstr<5>, &RiverX86Disassembler::DisassembleRelJmpInstr<5>,
		/*0x84*/ &RiverX86Disassembler::DisassembleRelJmpInstr<5>, &RiverX86Disassembler::DisassembleRelJmpInstr<5>, &RiverX86Disassembler::DisassembleRelJmpInstr<5>, &RiverX86Disassembler::DisassembleRelJmpInstr<5>,
		/*0x88*/ &RiverX86Disassembler::DisassembleRelJmpInstr<5>, &RiverX86Disassembler::DisassembleRelJmpInstr<5>, &RiverX86Disassembler::DisassembleRelJmpInstr<5>, &RiverX86Disassembler::DisassembleRelJmpInstr<5>,
		/*0x8C*/ &RiverX86Disassembler::DisassembleRelJmpInstr<5>, &RiverX86Disassembler::DisassembleRelJmpInstr<5>, &RiverX86Disassembler::DisassembleRelJmpInstr<5>, &RiverX86Disassembler::DisassembleRelJmpInstr<5>,

		/*0x90*/ &RiverX86Disassembler::DisassembleDefaultInstr<RIVER_MODIFIER_O8>, &RiverX86Disassembler::DisassembleDefaultInstr<RIVER_MODIFIER_O8>, &RiverX86Disassembler::DisassembleDefaultInstr<RIVER_MODIFIER_O8>, &RiverX86Disassembler::DisassembleDefaultInstr<RIVER_MODIFIER_O8>,
		/*0x94*/ &RiverX86Disassembler::DisassembleDefaultInstr<RIVER_MODIFIER_O8>, &RiverX86Disassembler::DisassembleDefaultInstr<RIVER_MODIFIER_O8>, &RiverX86Disassembler::DisassembleDefaultInstr<RIVER_MODIFIER_O8>, &RiverX86Disassembler::DisassembleDefaultInstr<RIVER_MODIFIER_O8>,
		/*0x98*/ &RiverX86Disassembler::DisassembleDefaultInstr<RIVER_MODIFIER_O8>, &RiverX86Disassembler::DisassembleDefaultInstr<RIVER_MODIFIER_O8>, &RiverX86Disassembler::DisassembleDefaultInstr<RIVER_MODIFIER_O8>, &RiverX86Disassembler::DisassembleDefaultInstr<RIVER_MODIFIER_O8>,
		/*0x9C*/ &RiverX86Disassembler::DisassembleDefaultInstr<RIVER_MODIFIER_O8>, &RiverX86Disassembler::DisassembleDefaultInstr<RIVER_MODIFIER_O8>, &RiverX86Disassembler::DisassembleDefaultInstr<RIVER_MODIFIER_O8>, &RiverX86Disassembler::DisassembleDefaultInstr<RIVER_MODIFIER_O8>,

		/*0xA0*/ &RiverX86Disassembler::DisassembleUnkInstr, &RiverX86Disassembler::DisassembleUnkInstr, &RiverX86Disassembler::DisassembleDefaultInstr, &RiverX86Disassembler::DisassembleUnkInstr,
		/*0xA4*/ &RiverX86Disassembler::DisassembleUnkInstr, &RiverX86Disassembler::DisassembleDefaultInstr, &RiverX86Disassembler::DisassembleUnkInstr, &RiverX86Disassembler::DisassembleUnkInstr,
		/*0xA8*/ &RiverX86Disassembler::DisassembleUnkInstr, &RiverX86Disassembler::DisassembleUnkInstr, &RiverX86Disassembler::DisassembleUnkInstr, &RiverX86Disassembler::DisassembleUnkInstr,
		/*0xAC*/ &RiverX86Disassembler::DisassembleUnkInstr, &RiverX86Disassembler::DisassembleDefaultInstr, &RiverX86Disassembler::DisassembleUnkInstr, &RiverX86Disassembler::DisassembleDefaultInstr,

		/*0xB0*/ &RiverX86Disassembler::DisassembleDefaultInstr<RIVER_MODIFIER_O8>, &RiverX86Disassembler::DisassembleDefaultInstr, &RiverX86Disassembler::DisassembleUnkInstr, &RiverX86Disassembler::DisassembleUnkInstr,
		/*0xB4*/ &RiverX86Disassembler::DisassembleUnkInstr, &RiverX86Disassembler::DisassembleUnkInstr, &RiverX86Disassembler::DisassembleDefaultInstr, &RiverX86Disassembler::DisassembleDefaultInstr,
		/*0xB8*/ &RiverX86Disassembler::DisassembleUnkInstr, &RiverX86Disassembler::DisassembleUnkInstr, &RiverX86Disassembler::DisassembleExtInstr, &RiverX86Disassembler::DisassembleUnkInstr,
		/*0xBC*/ &RiverX86Disassembler::DisassembleDefaultInstr, &RiverX86Disassembler::DisassembleDefaultInstr, &RiverX86Disassembler::DisassembleDefaultInstr<RIVER_MODIFIER_O8>, &RiverX86Disassembler::DisassembleDefaultInstr,

		/*0xC0*/ &RiverX86Disassembler::DisassembleDefaultInstr<RIVER_MODIFIER_O8>, &RiverX86Disassembler::DisassembleDefaultInstr, &RiverX86Disassembler::DisassembleUnkInstr, &RiverX86Disassembler::DisassembleUnkInstr,
		/*0xC4*/ &RiverX86Disassembler::DisassembleUnkInstr, &RiverX86Disassembler::DisassembleUnkInstr, &RiverX86Disassembler::DisassembleUnkInstr, &RiverX86Disassembler::DisassembleUnkInstr,
		/*0xC8*/ &RiverX86Disassembler::DisassembleUnkInstr, &RiverX86Disassembler::DisassembleUnkInstr, &RiverX86Disassembler::DisassembleUnkInstr, &RiverX86Disassembler::DisassembleUnkInstr,
		/*0xCC*/ &RiverX86Disassembler::DisassembleUnkInstr, &RiverX86Disassembler::DisassembleUnkInstr, &RiverX86Disassembler::DisassembleUnkInstr, &RiverX86Disassembler::DisassembleUnkInstr,

		/*0xD0*/ &RiverX86Disassembler::DisassembleUnkInstr, &RiverX86Disassembler::DisassembleUnkInstr, &RiverX86Disassembler::DisassembleUnkInstr, &RiverX86Disassembler::DisassembleUnkInstr,
		/*0xD4*/ &RiverX86Disassembler::DisassembleUnkInstr, &RiverX86Disassembler::DisassembleUnkInstr, &RiverX86Disassembler::DisassembleUnkInstr, &RiverX86Disassembler::DisassembleUnkInstr,
		/*0xD8*/ &RiverX86Disassembler::DisassembleUnkInstr, &RiverX86Disassembler::DisassembleUnkInstr, &RiverX86Disassembler::DisassembleUnkInstr, &RiverX86Disassembler::DisassembleUnkInstr,
		/*0xDC*/ &RiverX86Disassembler::DisassembleUnkInstr, &RiverX86Disassembler::DisassembleUnkInstr, &RiverX86Disassembler::DisassembleUnkInstr, &RiverX86Disassembler::DisassembleUnkInstr,

		/*0xE0*/ &RiverX86Disassembler::DisassembleUnkInstr, &RiverX86Disassembler::DisassembleUnkInstr, &RiverX86Disassembler::DisassembleUnkInstr, &RiverX86Disassembler::DisassembleUnkInstr,
		/*0xE4*/ &RiverX86Disassembler::DisassembleUnkInstr, &RiverX86Disassembler::DisassembleUnkInstr, &RiverX86Disassembler::DisassembleUnkInstr, &RiverX86Disassembler::DisassembleUnkInstr,
		/*0xE8*/ &RiverX86Disassembler::DisassembleUnkInstr, &RiverX86Disassembler::DisassembleUnkInstr, &RiverX86Disassembler::DisassembleUnkInstr, &RiverX86Disassembler::DisassembleUnkInstr,
		/*0xEC*/ &RiverX86Disassembler::DisassembleUnkInstr, &RiverX86Disassembler::DisassembleUnkInstr, &RiverX86Disassembler::DisassembleUnkInstr, &RiverX86Disassembler::DisassembleUnkInstr,

		/*0xF0*/ &RiverX86Disassembler::DisassembleUnkInstr, &RiverX86Disassembler::DisassembleUnkInstr, &RiverX86Disassembler::DisassembleUnkInstr, &RiverX86Disassembler::DisassembleUnkInstr,
		/*0xF4*/ &RiverX86Disassembler::DisassembleUnkInstr, &RiverX86Disassembler::DisassembleUnkInstr, &RiverX86Disassembler::DisassembleUnkInstr, &RiverX86Disassembler::DisassembleUnkInstr,
		/*0xF8*/ &RiverX86Disassembler::DisassembleUnkInstr, &RiverX86Disassembler::DisassembleUnkInstr, &RiverX86Disassembler::DisassembleUnkInstr, &RiverX86Disassembler::DisassembleUnkInstr,
		/*0xFC*/ &RiverX86Disassembler::DisassembleUnkInstr, &RiverX86Disassembler::DisassembleUnkInstr, &RiverX86Disassembler::DisassembleUnkInstr, &RiverX86Disassembler::DisassembleUnkInstr
	}
};

RiverX86Disassembler::DisassembleOperandsFunc RiverX86Disassembler::disassembleOperands[2][0x100] = {
	{
		/*0x00*/ &RiverX86Disassembler::DisassembleModRMReg, &RiverX86Disassembler::DisassembleModRMReg, &RiverX86Disassembler::DisassembleRegModRM, &RiverX86Disassembler::DisassembleRegModRM,
		/*0x04*/ &RiverX86Disassembler::DisassembleImmOp<1, RIVER_OPSIZE_8>, &RiverX86Disassembler::DisassembleImmOp<1, RIVER_OPSIZE_32>, &RiverX86Disassembler::DisassembleUnkOp, &RiverX86Disassembler::DisassembleUnkOp,
		/*0x08*/ &RiverX86Disassembler::DisassembleModRMReg, &RiverX86Disassembler::DisassembleModRMReg, &RiverX86Disassembler::DisassembleRegModRM, &RiverX86Disassembler::DisassembleRegModRM,
		/*0x0C*/ &RiverX86Disassembler::DisassembleImmOp<1, RIVER_OPSIZE_8>, &RiverX86Disassembler::DisassembleImmOp<1, RIVER_OPSIZE_32>, &RiverX86Disassembler::DisassembleUnkOp, &RiverX86Disassembler::DisassembleUnkOp,

		/*0x10*/ &RiverX86Disassembler::DisassembleModRMReg, &RiverX86Disassembler::DisassembleModRMReg, &RiverX86Disassembler::DisassembleRegModRM, &RiverX86Disassembler::DisassembleRegModRM,
		/*0x14*/ &RiverX86Disassembler::DisassembleImmOp<1, RIVER_OPSIZE_8>, &RiverX86Disassembler::DisassembleImmOp<1, RIVER_OPSIZE_32>, &RiverX86Disassembler::DisassembleNoOp, &RiverX86Disassembler::DisassembleNoOp,
		/*0x18*/ &RiverX86Disassembler::DisassembleModRMReg, &RiverX86Disassembler::DisassembleModRMReg, &RiverX86Disassembler::DisassembleRegModRM, &RiverX86Disassembler::DisassembleRegModRM,
		/*0x1C*/ &RiverX86Disassembler::DisassembleImmOp<1, RIVER_OPSIZE_8>, &RiverX86Disassembler::DisassembleImmOp<1, RIVER_OPSIZE_32>, &RiverX86Disassembler::DisassembleUnkOp, &RiverX86Disassembler::DisassembleUnkOp,

		/*0x20*/ &RiverX86Disassembler::DisassembleModRMReg, &RiverX86Disassembler::DisassembleModRMReg, &RiverX86Disassembler::DisassembleRegModRM, &RiverX86Disassembler::DisassembleRegModRM,
		/*0x24*/ &RiverX86Disassembler::DisassembleImmOp<1, RIVER_OPSIZE_8>, &RiverX86Disassembler::DisassembleImmOp<1, RIVER_OPSIZE_32>, &RiverX86Disassembler::DisassembleUnkOp, &RiverX86Disassembler::DisassembleUnkOp,
		/*0x28*/ &RiverX86Disassembler::DisassembleModRMReg, &RiverX86Disassembler::DisassembleModRMReg, &RiverX86Disassembler::DisassembleRegModRM, &RiverX86Disassembler::DisassembleRegModRM,
		/*0x2C*/ &RiverX86Disassembler::DisassembleImmOp<1, RIVER_OPSIZE_8>, &RiverX86Disassembler::DisassembleImmOp<1, RIVER_OPSIZE_32>, &RiverX86Disassembler::DisassembleUnkOp, &RiverX86Disassembler::DisassembleUnkOp,

		/*0x30*/ &RiverX86Disassembler::DisassembleModRMReg, &RiverX86Disassembler::DisassembleModRMReg, &RiverX86Disassembler::DisassembleRegModRM, &RiverX86Disassembler::DisassembleRegModRM,
		/*0x34*/ &RiverX86Disassembler::DisassembleImmOp<1, RIVER_OPSIZE_8>, &RiverX86Disassembler::DisassembleImmOp<1, RIVER_OPSIZE_32>, &RiverX86Disassembler::DisassembleUnkOp, &RiverX86Disassembler::DisassembleUnkOp,
		/*0x38*/ &RiverX86Disassembler::DisassembleModRMReg, &RiverX86Disassembler::DisassembleModRMReg, &RiverX86Disassembler::DisassembleRegModRM, &RiverX86Disassembler::DisassembleRegModRM,
		/*0x3C*/ &RiverX86Disassembler::DisassembleImmOp<1, RIVER_OPSIZE_8>, &RiverX86Disassembler::DisassembleImmOp<1, RIVER_OPSIZE_32>, &RiverX86Disassembler::DisassembleUnkOp, &RiverX86Disassembler::DisassembleUnkOp,

		/*0x40*/ &RiverX86Disassembler::DisassembleNoOp, &RiverX86Disassembler::DisassembleNoOp, &RiverX86Disassembler::DisassembleNoOp, &RiverX86Disassembler::DisassembleNoOp,
		/*0x44*/ &RiverX86Disassembler::DisassembleNoOp, &RiverX86Disassembler::DisassembleNoOp, &RiverX86Disassembler::DisassembleNoOp, &RiverX86Disassembler::DisassembleNoOp,
		/*0x48*/ &RiverX86Disassembler::DisassembleNoOp, &RiverX86Disassembler::DisassembleNoOp, &RiverX86Disassembler::DisassembleNoOp, &RiverX86Disassembler::DisassembleNoOp,
		/*0x4C*/ &RiverX86Disassembler::DisassembleNoOp, &RiverX86Disassembler::DisassembleNoOp, &RiverX86Disassembler::DisassembleNoOp, &RiverX86Disassembler::DisassembleNoOp,

		/*0x50*/ &RiverX86Disassembler::DisassembleNoOp, &RiverX86Disassembler::DisassembleNoOp, &RiverX86Disassembler::DisassembleNoOp, &RiverX86Disassembler::DisassembleNoOp,
		/*0x54*/ &RiverX86Disassembler::DisassembleNoOp, &RiverX86Disassembler::DisassembleNoOp, &RiverX86Disassembler::DisassembleNoOp, &RiverX86Disassembler::DisassembleNoOp,
		/*0x58*/ &RiverX86Disassembler::DisassembleNoOp, &RiverX86Disassembler::DisassembleNoOp, &RiverX86Disassembler::DisassembleNoOp, &RiverX86Disassembler::DisassembleNoOp,
		/*0x5C*/ &RiverX86Disassembler::DisassembleNoOp, &RiverX86Disassembler::DisassembleNoOp, &RiverX86Disassembler::DisassembleNoOp, &RiverX86Disassembler::DisassembleNoOp,

		/*0x60*/ &RiverX86Disassembler::DisassembleUnkOp, &RiverX86Disassembler::DisassembleUnkOp, &RiverX86Disassembler::DisassembleUnkOp, &RiverX86Disassembler::DisassembleUnkOp,
		/*0x64*/ &RiverX86Disassembler::DisassembleUnkOp, &RiverX86Disassembler::DisassembleUnkOp, &RiverX86Disassembler::DisassembleUnkOp, &RiverX86Disassembler::DisassembleUnkOp,
		/*0x68*/ &RiverX86Disassembler::DisassembleImm32, &RiverX86Disassembler::DisassembleRegModRMImm32, &RiverX86Disassembler::DisassembleImm8, &RiverX86Disassembler::DisassembleRegModRMImm8,
		/*0x6C*/ &RiverX86Disassembler::DisassembleUnkOp, &RiverX86Disassembler::DisassembleUnkOp, &RiverX86Disassembler::DisassembleUnkOp, &RiverX86Disassembler::DisassembleUnkOp,

		/*0x70*/ &RiverX86Disassembler::DisassembleImm8, &RiverX86Disassembler::DisassembleImm8, &RiverX86Disassembler::DisassembleImm8, &RiverX86Disassembler::DisassembleImm8,
		/*0x74*/ &RiverX86Disassembler::DisassembleImm8, &RiverX86Disassembler::DisassembleImm8, &RiverX86Disassembler::DisassembleImm8, &RiverX86Disassembler::DisassembleImm8,
		/*0x78*/ &RiverX86Disassembler::DisassembleImm8, &RiverX86Disassembler::DisassembleImm8, &RiverX86Disassembler::DisassembleImm8, &RiverX86Disassembler::DisassembleImm8,
		/*0x7C*/ &RiverX86Disassembler::DisassembleImm8, &RiverX86Disassembler::DisassembleImm8, &RiverX86Disassembler::DisassembleImm8, &RiverX86Disassembler::DisassembleImm8,

		/*0x80*/ &RiverX86Disassembler::DisassembleSubOpModRMImm8, &RiverX86Disassembler::DisassembleSubOpModRMImm32, &RiverX86Disassembler::DisassembleSubOpModRMImm8, &RiverX86Disassembler::DisassembleSubOpModRMImm8,
		/*0x84*/ &RiverX86Disassembler::DisassembleRegModRM, &RiverX86Disassembler::DisassembleRegModRM, &RiverX86Disassembler::DisassembleRegModRM, &RiverX86Disassembler::DisassembleRegModRM,
		/*0x88*/ &RiverX86Disassembler::DisassembleModRMReg, &RiverX86Disassembler::DisassembleModRMReg, &RiverX86Disassembler::DisassembleRegModRM, &RiverX86Disassembler::DisassembleRegModRM,
		/*0x8C*/ &RiverX86Disassembler::DisassembleUnkOp, &RiverX86Disassembler::DisassembleRegModRM, &RiverX86Disassembler::DisassembleUnkOp, &RiverX86Disassembler::DisassembleUnkOp,

		/*0x90*/ &RiverX86Disassembler::DisassembleDefaultSecondRegOp<RIVER_REG_xAX>, &RiverX86Disassembler::DisassembleDefaultSecondRegOp<RIVER_REG_xAX>, &RiverX86Disassembler::DisassembleDefaultSecondRegOp<RIVER_REG_xAX>, &RiverX86Disassembler::DisassembleDefaultSecondRegOp<RIVER_REG_xAX>,
		/*0x94*/ &RiverX86Disassembler::DisassembleDefaultSecondRegOp<RIVER_REG_xAX>, &RiverX86Disassembler::DisassembleDefaultSecondRegOp<RIVER_REG_xAX>, &RiverX86Disassembler::DisassembleDefaultSecondRegOp<RIVER_REG_xAX>, &RiverX86Disassembler::DisassembleDefaultSecondRegOp<RIVER_REG_xAX>,
		/*0x98*/ &RiverX86Disassembler::DisassembleUnkOp, &RiverX86Disassembler::DisassembleNoOp, &RiverX86Disassembler::DisassembleUnkOp, &RiverX86Disassembler::DisassembleUnkOp,
		/*0x9C*/ &RiverX86Disassembler::DisassembleUnkOp, &RiverX86Disassembler::DisassembleUnkOp, &RiverX86Disassembler::DisassembleUnkOp, &RiverX86Disassembler::DisassembleUnkOp,

		/*0xA0*/ &RiverX86Disassembler::DisassembleMoffs8<1>, &RiverX86Disassembler::DisassembleMoffs32<1>, &RiverX86Disassembler::DisassembleMoffs8<0>, &RiverX86Disassembler::DisassembleMoffs32<0>,
		/*0xA4*/ &RiverX86Disassembler::DisassembleNoOp, &RiverX86Disassembler::DisassembleNoOp, &RiverX86Disassembler::DisassembleNoOp, &RiverX86Disassembler::DisassembleNoOp,
		/*0xA8*/ &RiverX86Disassembler::DisassembleImmOp<1, RIVER_OPSIZE_8>, &RiverX86Disassembler::DisassembleImmOp<1, RIVER_OPSIZE_32>, &RiverX86Disassembler::DisassembleNoOp, &RiverX86Disassembler::DisassembleNoOp,
		/*0xAC*/ &RiverX86Disassembler::DisassembleUnkOp, &RiverX86Disassembler::DisassembleUnkOp, &RiverX86Disassembler::DisassembleNoOp, &RiverX86Disassembler::DisassembleNoOp,

		/*0xB0*/ &RiverX86Disassembler::DisassembleImmOp<1, RIVER_OPSIZE_8>, &RiverX86Disassembler::DisassembleImmOp<1, RIVER_OPSIZE_8>, &RiverX86Disassembler::DisassembleImmOp<1, RIVER_OPSIZE_8>, &RiverX86Disassembler::DisassembleImmOp<1, RIVER_OPSIZE_8>,
		/*0xB4*/ &RiverX86Disassembler::DisassembleImmOp<1, RIVER_OPSIZE_8>, &RiverX86Disassembler::DisassembleImmOp<1, RIVER_OPSIZE_8>, &RiverX86Disassembler::DisassembleImmOp<1, RIVER_OPSIZE_8>, &RiverX86Disassembler::DisassembleImmOp<1, RIVER_OPSIZE_8>,
		/*0xB8*/ &RiverX86Disassembler::DisassembleImmOp<1, RIVER_OPSIZE_32>, &RiverX86Disassembler::DisassembleImmOp<1, RIVER_OPSIZE_32>, &RiverX86Disassembler::DisassembleImmOp<1, RIVER_OPSIZE_32>, &RiverX86Disassembler::DisassembleImmOp<1, RIVER_OPSIZE_32>,
		/*0xBC*/ &RiverX86Disassembler::DisassembleImmOp<1, RIVER_OPSIZE_32>, &RiverX86Disassembler::DisassembleImmOp<1, RIVER_OPSIZE_32>, &RiverX86Disassembler::DisassembleImmOp<1, RIVER_OPSIZE_32>, &RiverX86Disassembler::DisassembleImmOp<1, RIVER_OPSIZE_32>,

		/*0xC0*/ &RiverX86Disassembler::DisassembleSubOpModRMImm8, &RiverX86Disassembler::DisassembleSubOpModRMImm8, &RiverX86Disassembler::DisassembleImm16, &RiverX86Disassembler::DisassembleNoOp,
		/*0xC4*/ &RiverX86Disassembler::DisassembleUnkOp, &RiverX86Disassembler::DisassembleUnkOp, &RiverX86Disassembler::DisassembleModRMImm8, &RiverX86Disassembler::DisassembleModRMImm32,
		/*0xC8*/ &RiverX86Disassembler::DisassembleUnkOp, &RiverX86Disassembler::DisassembleNoOp, &RiverX86Disassembler::DisassembleUnkOp, &RiverX86Disassembler::DisassembleUnkOp,
		/*0xCC*/ &RiverX86Disassembler::DisassembleUnkOp, &RiverX86Disassembler::DisassembleUnkOp, &RiverX86Disassembler::DisassembleUnkOp, &RiverX86Disassembler::DisassembleUnkOp,

		/*0xD0*/ &RiverX86Disassembler::DisassembleSubOpModRM, &RiverX86Disassembler::DisassembleSubOpModRM, &RiverX86Disassembler::DisassembleSubOpModRM, &RiverX86Disassembler::DisassembleSubOpModRM,
		/*0xD4*/ &RiverX86Disassembler::DisassembleUnkOp, &RiverX86Disassembler::DisassembleUnkOp, &RiverX86Disassembler::DisassembleUnkOp, &RiverX86Disassembler::DisassembleUnkOp,
		/*0xD8*/ &RiverX86Disassembler::DisassembleUnkOp, &RiverX86Disassembler::DisassembleUnkOp, &RiverX86Disassembler::DisassembleUnkOp, &RiverX86Disassembler::DisassembleUnkOp,
		/*0xDC*/ &RiverX86Disassembler::DisassembleUnkOp, &RiverX86Disassembler::DisassembleUnkOp, &RiverX86Disassembler::DisassembleUnkOp, &RiverX86Disassembler::DisassembleUnkOp,

		/*0xE0*/ &RiverX86Disassembler::DisassembleUnkOp, &RiverX86Disassembler::DisassembleUnkOp, &RiverX86Disassembler::DisassembleUnkOp, &RiverX86Disassembler::DisassembleUnkOp,
		/*0xE4*/ &RiverX86Disassembler::DisassembleUnkOp, &RiverX86Disassembler::DisassembleUnkOp, &RiverX86Disassembler::DisassembleUnkOp, &RiverX86Disassembler::DisassembleUnkOp,
		/*0xE8*/ &RiverX86Disassembler::DisassembleImm32, &RiverX86Disassembler::DisassembleImm32, &RiverX86Disassembler::DisassembleImm32Imm16, &RiverX86Disassembler::DisassembleImm8,
		/*0xEC*/ &RiverX86Disassembler::DisassembleUnkOp, &RiverX86Disassembler::DisassembleUnkOp, &RiverX86Disassembler::DisassembleUnkOp, &RiverX86Disassembler::DisassembleUnkOp,

		/*0xF0*/ &RiverX86Disassembler::DisassembleUnkOp, &RiverX86Disassembler::DisassembleUnkOp, &RiverX86Disassembler::DisassembleUnkOp, &RiverX86Disassembler::DisassembleUnkOp,
		/*0xF4*/ &RiverX86Disassembler::DisassembleUnkOp, &RiverX86Disassembler::DisassembleUnkOp, &RiverX86Disassembler::DisassembleSubOpOp<RiverX86Disassembler::disassemble0xF6Op>, &RiverX86Disassembler::DisassembleSubOpOp<RiverX86Disassembler::disassemble0xF7Op>,
		/*0xF8*/ &RiverX86Disassembler::DisassembleUnkOp, &RiverX86Disassembler::DisassembleUnkOp, &RiverX86Disassembler::DisassembleUnkOp, &RiverX86Disassembler::DisassembleUnkOp,
		/*0xFC*/ &RiverX86Disassembler::DisassembleNoOp, &RiverX86Disassembler::DisassembleNoOp, &RiverX86Disassembler::DisassembleSubOpModRM, &RiverX86Disassembler::DisassembleSubOpOp<RiverX86Disassembler::disassemble0xFFOp>
	}, {
		/*0x00*/ &RiverX86Disassembler::DisassembleUnkOp, &RiverX86Disassembler::DisassembleUnkOp, &RiverX86Disassembler::DisassembleRegModRM, &RiverX86Disassembler::DisassembleRegModRM,
		/*0x04*/ &RiverX86Disassembler::DisassembleUnkOp, &RiverX86Disassembler::DisassembleUnkOp, &RiverX86Disassembler::DisassembleUnkOp, &RiverX86Disassembler::DisassembleUnkOp,
		/*0x08*/ &RiverX86Disassembler::DisassembleUnkOp, &RiverX86Disassembler::DisassembleUnkOp, &RiverX86Disassembler::DisassembleUnkOp, &RiverX86Disassembler::DisassembleUnkOp,
		/*0x0C*/ &RiverX86Disassembler::DisassembleUnkOp, &RiverX86Disassembler::DisassembleUnkOp, &RiverX86Disassembler::DisassembleUnkOp, &RiverX86Disassembler::DisassembleUnkOp,

		/*0x10*/ &RiverX86Disassembler::DisassembleUnkOp, &RiverX86Disassembler::DisassembleUnkOp, &RiverX86Disassembler::DisassembleUnkOp, &RiverX86Disassembler::DisassembleUnkOp,
		/*0x14*/ &RiverX86Disassembler::DisassembleUnkOp, &RiverX86Disassembler::DisassembleUnkOp, &RiverX86Disassembler::DisassembleUnkOp, &RiverX86Disassembler::DisassembleUnkOp,
		/*0x18*/ &RiverX86Disassembler::DisassembleUnkOp, &RiverX86Disassembler::DisassembleUnkOp, &RiverX86Disassembler::DisassembleUnkOp, &RiverX86Disassembler::DisassembleUnkOp,
		/*0x1C*/ &RiverX86Disassembler::DisassembleUnkOp, &RiverX86Disassembler::DisassembleUnkOp, &RiverX86Disassembler::DisassembleUnkOp, &RiverX86Disassembler::DisassembleUnkOp,

		/*0x20*/ &RiverX86Disassembler::DisassembleUnkOp, &RiverX86Disassembler::DisassembleUnkOp, &RiverX86Disassembler::DisassembleUnkOp, &RiverX86Disassembler::DisassembleUnkOp,
		/*0x24*/ &RiverX86Disassembler::DisassembleUnkOp, &RiverX86Disassembler::DisassembleUnkOp, &RiverX86Disassembler::DisassembleUnkOp, &RiverX86Disassembler::DisassembleUnkOp,
		/*0x28*/ &RiverX86Disassembler::DisassembleUnkOp, &RiverX86Disassembler::DisassembleUnkOp, &RiverX86Disassembler::DisassembleUnkOp, &RiverX86Disassembler::DisassembleUnkOp,
		/*0x2C*/ &RiverX86Disassembler::DisassembleUnkOp, &RiverX86Disassembler::DisassembleUnkOp, &RiverX86Disassembler::DisassembleUnkOp, &RiverX86Disassembler::DisassembleUnkOp,

		/*0x30*/ &RiverX86Disassembler::DisassembleUnkOp, &RiverX86Disassembler::DisassembleNoOp, &RiverX86Disassembler::DisassembleUnkOp, &RiverX86Disassembler::DisassembleUnkOp,
		/*0x34*/ &RiverX86Disassembler::DisassembleUnkOp, &RiverX86Disassembler::DisassembleUnkOp, &RiverX86Disassembler::DisassembleUnkOp, &RiverX86Disassembler::DisassembleUnkOp,
		/*0x38*/ &RiverX86Disassembler::DisassembleUnkOp, &RiverX86Disassembler::DisassembleUnkOp, &RiverX86Disassembler::DisassembleUnkOp, &RiverX86Disassembler::DisassembleUnkOp,
		/*0x3C*/ &RiverX86Disassembler::DisassembleUnkOp, &RiverX86Disassembler::DisassembleUnkOp, &RiverX86Disassembler::DisassembleUnkOp, &RiverX86Disassembler::DisassembleUnkOp,

		/*0x40*/ &RiverX86Disassembler::DisassembleRegModRM, &RiverX86Disassembler::DisassembleRegModRM, &RiverX86Disassembler::DisassembleRegModRM, &RiverX86Disassembler::DisassembleRegModRM,
		/*0x44*/ &RiverX86Disassembler::DisassembleRegModRM, &RiverX86Disassembler::DisassembleRegModRM, &RiverX86Disassembler::DisassembleRegModRM, &RiverX86Disassembler::DisassembleRegModRM,
		/*0x48*/ &RiverX86Disassembler::DisassembleRegModRM, &RiverX86Disassembler::DisassembleRegModRM, &RiverX86Disassembler::DisassembleRegModRM, &RiverX86Disassembler::DisassembleRegModRM,
		/*0x4C*/ &RiverX86Disassembler::DisassembleRegModRM, &RiverX86Disassembler::DisassembleRegModRM, &RiverX86Disassembler::DisassembleRegModRM, &RiverX86Disassembler::DisassembleRegModRM,

		/*0x50*/ &RiverX86Disassembler::DisassembleUnkOp, &RiverX86Disassembler::DisassembleUnkOp, &RiverX86Disassembler::DisassembleUnkOp, &RiverX86Disassembler::DisassembleUnkOp,
		/*0x54*/ &RiverX86Disassembler::DisassembleUnkOp, &RiverX86Disassembler::DisassembleUnkOp, &RiverX86Disassembler::DisassembleUnkOp, &RiverX86Disassembler::DisassembleUnkOp,
		/*0x58*/ &RiverX86Disassembler::DisassembleUnkOp, &RiverX86Disassembler::DisassembleUnkOp, &RiverX86Disassembler::DisassembleUnkOp, &RiverX86Disassembler::DisassembleUnkOp,
		/*0x5C*/ &RiverX86Disassembler::DisassembleUnkOp, &RiverX86Disassembler::DisassembleUnkOp, &RiverX86Disassembler::DisassembleUnkOp, &RiverX86Disassembler::DisassembleUnkOp,

		/*0x60*/ &RiverX86Disassembler::DisassembleUnkOp, &RiverX86Disassembler::DisassembleUnkOp, &RiverX86Disassembler::DisassembleUnkOp, &RiverX86Disassembler::DisassembleUnkOp,
		/*0x64*/ &RiverX86Disassembler::DisassembleUnkOp, &RiverX86Disassembler::DisassembleUnkOp, &RiverX86Disassembler::DisassembleUnkOp, &RiverX86Disassembler::DisassembleUnkOp,
		/*0x68*/ &RiverX86Disassembler::DisassembleUnkOp, &RiverX86Disassembler::DisassembleUnkOp, &RiverX86Disassembler::DisassembleUnkOp, &RiverX86Disassembler::DisassembleUnkOp,
		/*0x6C*/ &RiverX86Disassembler::DisassembleUnkOp, &RiverX86Disassembler::DisassembleUnkOp, &RiverX86Disassembler::DisassembleUnkOp, &RiverX86Disassembler::DisassembleUnkOp,

		/*0x70*/ &RiverX86Disassembler::DisassembleUnkOp, &RiverX86Disassembler::DisassembleUnkOp, &RiverX86Disassembler::DisassembleUnkOp, &RiverX86Disassembler::DisassembleUnkOp,
		/*0x74*/ &RiverX86Disassembler::DisassembleUnkOp, &RiverX86Disassembler::DisassembleUnkOp, &RiverX86Disassembler::DisassembleUnkOp, &RiverX86Disassembler::DisassembleUnkOp,
		/*0x78*/ &RiverX86Disassembler::DisassembleUnkOp, &RiverX86Disassembler::DisassembleUnkOp, &RiverX86Disassembler::DisassembleUnkOp, &RiverX86Disassembler::DisassembleUnkOp,
		/*0x7C*/ &RiverX86Disassembler::DisassembleUnkOp, &RiverX86Disassembler::DisassembleUnkOp, &RiverX86Disassembler::DisassembleUnkOp, &RiverX86Disassembler::DisassembleUnkOp,

		/*0x80*/ &RiverX86Disassembler::DisassembleImm32, &RiverX86Disassembler::DisassembleImm32, &RiverX86Disassembler::DisassembleImm32, &RiverX86Disassembler::DisassembleImm32,
		/*0x84*/ &RiverX86Disassembler::DisassembleImm32, &RiverX86Disassembler::DisassembleImm32, &RiverX86Disassembler::DisassembleImm32, &RiverX86Disassembler::DisassembleImm32,
		/*0x88*/ &RiverX86Disassembler::DisassembleImm32, &RiverX86Disassembler::DisassembleImm32, &RiverX86Disassembler::DisassembleImm32, &RiverX86Disassembler::DisassembleImm32,
		/*0x8C*/ &RiverX86Disassembler::DisassembleImm32, &RiverX86Disassembler::DisassembleImm32, &RiverX86Disassembler::DisassembleImm32, &RiverX86Disassembler::DisassembleImm32,

		/*0x90*/ &RiverX86Disassembler::DisassembleModRM<0>, &RiverX86Disassembler::DisassembleModRM<0>, &RiverX86Disassembler::DisassembleModRM<0>, &RiverX86Disassembler::DisassembleModRM<0>,
		/*0x94*/ &RiverX86Disassembler::DisassembleModRM<0>, &RiverX86Disassembler::DisassembleModRM<0>, &RiverX86Disassembler::DisassembleModRM<0>, &RiverX86Disassembler::DisassembleModRM<0>,
		/*0x98*/ &RiverX86Disassembler::DisassembleModRM<0>, &RiverX86Disassembler::DisassembleModRM<0>, &RiverX86Disassembler::DisassembleModRM<0>, &RiverX86Disassembler::DisassembleModRM<0>,
		/*0x9C*/ &RiverX86Disassembler::DisassembleModRM<0>, &RiverX86Disassembler::DisassembleModRM<0>, &RiverX86Disassembler::DisassembleModRM<0>, &RiverX86Disassembler::DisassembleModRM<0>,

		/*0xA0*/ &RiverX86Disassembler::DisassembleUnkOp, &RiverX86Disassembler::DisassembleUnkOp, &RiverX86Disassembler::DisassembleNoOp, &RiverX86Disassembler::DisassembleUnkOp,
		/*0xA4*/ &RiverX86Disassembler::DisassembleUnkOp, &RiverX86Disassembler::DisassembleModRMRegThirdFixed<RIVER_REG_CL>, &RiverX86Disassembler::DisassembleUnkOp, &RiverX86Disassembler::DisassembleUnkOp,
		/*0xA8*/ &RiverX86Disassembler::DisassembleUnkOp, &RiverX86Disassembler::DisassembleUnkOp, &RiverX86Disassembler::DisassembleUnkOp, &RiverX86Disassembler::DisassembleUnkOp,
		/*0xAC*/ &RiverX86Disassembler::DisassembleUnkOp, &RiverX86Disassembler::DisassembleModRMReg, &RiverX86Disassembler::DisassembleUnkOp, &RiverX86Disassembler::DisassembleRegModRM,

		/*0xB0*/ &RiverX86Disassembler::DisassembleModRMReg, &RiverX86Disassembler::DisassembleModRMReg, &RiverX86Disassembler::DisassembleUnkOp, &RiverX86Disassembler::DisassembleUnkOp,
		/*0xB4*/ &RiverX86Disassembler::DisassembleUnkOp, &RiverX86Disassembler::DisassembleUnkOp, &RiverX86Disassembler::DisassembleRegSzModRM<RIVER_OPSIZE_8>, &RiverX86Disassembler::DisassembleRegSzModRM<RIVER_OPSIZE_16>,
		/*0xB8*/ &RiverX86Disassembler::DisassembleUnkOp, &RiverX86Disassembler::DisassembleUnkOp, &RiverX86Disassembler::DisassembleSubOpModRMImm8, &RiverX86Disassembler::DisassembleUnkOp,
		/*0xBC*/ &RiverX86Disassembler::DisassembleRegModRM, &RiverX86Disassembler::DisassembleRegModRM, &RiverX86Disassembler::DisassembleRegModRM, &RiverX86Disassembler::DisassembleRegModRM,

		/*0xC0*/ &RiverX86Disassembler::DisassembleModRMReg, &RiverX86Disassembler::DisassembleModRMReg, &RiverX86Disassembler::DisassembleUnkOp, &RiverX86Disassembler::DisassembleUnkOp,
		/*0xC4*/ &RiverX86Disassembler::DisassembleUnkOp, &RiverX86Disassembler::DisassembleUnkOp, &RiverX86Disassembler::DisassembleUnkOp, &RiverX86Disassembler::DisassembleUnkOp,
		/*0xC8*/ &RiverX86Disassembler::DisassembleUnkOp, &RiverX86Disassembler::DisassembleUnkOp, &RiverX86Disassembler::DisassembleUnkOp, &RiverX86Disassembler::DisassembleUnkOp,
		/*0xCC*/ &RiverX86Disassembler::DisassembleUnkOp, &RiverX86Disassembler::DisassembleUnkOp, &RiverX86Disassembler::DisassembleUnkOp, &RiverX86Disassembler::DisassembleUnkOp,

		/*0xD0*/ &RiverX86Disassembler::DisassembleUnkOp, &RiverX86Disassembler::DisassembleUnkOp, &RiverX86Disassembler::DisassembleUnkOp, &RiverX86Disassembler::DisassembleUnkOp,
		/*0xD4*/ &RiverX86Disassembler::DisassembleUnkOp, &RiverX86Disassembler::DisassembleUnkOp, &RiverX86Disassembler::DisassembleUnkOp, &RiverX86Disassembler::DisassembleUnkOp,
		/*0xD8*/ &RiverX86Disassembler::DisassembleUnkOp, &RiverX86Disassembler::DisassembleUnkOp, &RiverX86Disassembler::DisassembleUnkOp, &RiverX86Disassembler::DisassembleUnkOp,
		/*0xDC*/ &RiverX86Disassembler::DisassembleUnkOp, &RiverX86Disassembler::DisassembleUnkOp, &RiverX86Disassembler::DisassembleUnkOp, &RiverX86Disassembler::DisassembleUnkOp,

		/*0xE0*/ &RiverX86Disassembler::DisassembleUnkOp, &RiverX86Disassembler::DisassembleUnkOp, &RiverX86Disassembler::DisassembleUnkOp, &RiverX86Disassembler::DisassembleUnkOp,
		/*0xE4*/ &RiverX86Disassembler::DisassembleUnkOp, &RiverX86Disassembler::DisassembleUnkOp, &RiverX86Disassembler::DisassembleUnkOp, &RiverX86Disassembler::DisassembleUnkOp,
		/*0xE8*/ &RiverX86Disassembler::DisassembleUnkOp, &RiverX86Disassembler::DisassembleUnkOp, &RiverX86Disassembler::DisassembleUnkOp, &RiverX86Disassembler::DisassembleUnkOp,
		/*0xEC*/ &RiverX86Disassembler::DisassembleUnkOp, &RiverX86Disassembler::DisassembleUnkOp, &RiverX86Disassembler::DisassembleUnkOp, &RiverX86Disassembler::DisassembleUnkOp,

		/*0xF0*/ &RiverX86Disassembler::DisassembleUnkOp, &RiverX86Disassembler::DisassembleUnkOp, &RiverX86Disassembler::DisassembleUnkOp, &RiverX86Disassembler::DisassembleUnkOp,
		/*0xF4*/ &RiverX86Disassembler::DisassembleUnkOp, &RiverX86Disassembler::DisassembleUnkOp, &RiverX86Disassembler::DisassembleUnkOp, &RiverX86Disassembler::DisassembleUnkOp,
		/*0xF8*/ &RiverX86Disassembler::DisassembleUnkOp, &RiverX86Disassembler::DisassembleUnkOp, &RiverX86Disassembler::DisassembleUnkOp, &RiverX86Disassembler::DisassembleUnkOp,
		/*0xFC*/ &RiverX86Disassembler::DisassembleUnkOp, &RiverX86Disassembler::DisassembleUnkOp, &RiverX86Disassembler::DisassembleUnkOp, &RiverX86Disassembler::DisassembleUnkOp
	}
};

RiverX86Disassembler::DisassembleOpcodeFunc RiverX86Disassembler::disassemble0xF6Instr[8] = {
	&RiverX86Disassembler::DisassembleDefaultInstr<RIVER_OPSIZE_8>, &RiverX86Disassembler::DisassembleDefaultInstr<RIVER_OPSIZE_8>, &RiverX86Disassembler::DisassembleDefaultInstr<RIVER_OPSIZE_8>, &RiverX86Disassembler::DisassembleDefaultInstr<RIVER_OPSIZE_8>,
	&RiverX86Disassembler::DisassembleUnkInstr, &RiverX86Disassembler::DisassembleUnkInstr, &RiverX86Disassembler::DisassembleUnkInstr, &RiverX86Disassembler::DisassembleUnkInstr
};

RiverX86Disassembler::DisassembleOperandsFunc RiverX86Disassembler::disassemble0xF6Op[8] = {
	&RiverX86Disassembler::DisassembleModRMImm8, &RiverX86Disassembler::DisassembleModRMImm8, &RiverX86Disassembler::DisassembleSubOpModRM, &RiverX86Disassembler::DisassembleSubOpModRM,
	&RiverX86Disassembler::DisassembleUnkOp, &RiverX86Disassembler::DisassembleUnkOp, &RiverX86Disassembler::DisassembleUnkOp, &RiverX86Disassembler::DisassembleUnkOp
};


RiverX86Disassembler::DisassembleOpcodeFunc RiverX86Disassembler::disassemble0xF7Instr[8] = {
	&RiverX86Disassembler::DisassembleDefaultInstr, &RiverX86Disassembler::DisassembleDefaultInstr, &RiverX86Disassembler::DisassembleDefaultInstr, &RiverX86Disassembler::DisassembleDefaultInstr,
	&RiverX86Disassembler::DisassembleDefaultInstr, &RiverX86Disassembler::DisassembleDefaultInstr, &RiverX86Disassembler::DisassembleDefaultInstr, &RiverX86Disassembler::DisassembleDefaultInstr
};

RiverX86Disassembler::DisassembleOperandsFunc RiverX86Disassembler::disassemble0xF7Op[8] = {
	&RiverX86Disassembler::DisassembleModRMImm32, &RiverX86Disassembler::DisassembleModRMImm32, &RiverX86Disassembler::DisassembleSubOpModRM, &RiverX86Disassembler::DisassembleSubOpModRM,
	&RiverX86Disassembler::DisassembleModRM<0>, &RiverX86Disassembler::DisassembleModRM<0>, &RiverX86Disassembler::DisassembleModRM<0>, &RiverX86Disassembler::DisassembleModRM<0>
};


RiverX86Disassembler::DisassembleOpcodeFunc RiverX86Disassembler::disassemble0xFFInstr[8] = {
	&RiverX86Disassembler::DisassembleDefaultInstr, &RiverX86Disassembler::DisassembleDefaultInstr, &RiverX86Disassembler::DisassembleRelJmpModRMInstr, &RiverX86Disassembler::DisassembleUnkInstr,
	&RiverX86Disassembler::DisassembleRelJmpModRMInstr, &RiverX86Disassembler::DisassembleUnkInstr, &RiverX86Disassembler::DisassembleDefaultInstr, &RiverX86Disassembler::DisassembleUnkInstr
};

RiverX86Disassembler::DisassembleOperandsFunc RiverX86Disassembler::disassemble0xFFOp[8] = {
	&RiverX86Disassembler::DisassembleSubOpModRM, &RiverX86Disassembler::DisassembleSubOpModRM, &RiverX86Disassembler::DisassembleNoOp, &RiverX86Disassembler::DisassembleUnkOp,
	&RiverX86Disassembler::DisassembleNoOp, &RiverX86Disassembler::DisassembleUnkOp, &RiverX86Disassembler::DisassembleSubOpModRM, &RiverX86Disassembler::DisassembleUnkOp
};

/* =========================================== */
/* Specifier tables                            */
/* =========================================== */

const BYTE specTbl[2][0x100] = {
	{
		/*0x00*/ RIVER_SPEC_MODIFIES_OP1 | RIVER_SPEC_MODIFIES_FLG, RIVER_SPEC_MODIFIES_OP1 | RIVER_SPEC_MODIFIES_FLG, RIVER_SPEC_MODIFIES_OP1 | RIVER_SPEC_MODIFIES_FLG, RIVER_SPEC_MODIFIES_OP1 | RIVER_SPEC_MODIFIES_FLG,
		/*0x04*/ RIVER_SPEC_MODIFIES_OP1 | RIVER_SPEC_MODIFIES_FLG, RIVER_SPEC_MODIFIES_OP1 | RIVER_SPEC_MODIFIES_FLG, RIVER_SPEC_MODIFIES_xSP, 0xFF,
		/*0x08*/ RIVER_SPEC_MODIFIES_OP1 | RIVER_SPEC_MODIFIES_FLG, RIVER_SPEC_MODIFIES_OP1 | RIVER_SPEC_MODIFIES_FLG, RIVER_SPEC_MODIFIES_OP1 | RIVER_SPEC_MODIFIES_FLG, RIVER_SPEC_MODIFIES_OP1 | RIVER_SPEC_MODIFIES_FLG,
		/*0x0C*/ RIVER_SPEC_MODIFIES_OP1 | RIVER_SPEC_MODIFIES_FLG, RIVER_SPEC_MODIFIES_OP1 | RIVER_SPEC_MODIFIES_FLG, 0xFF, 0,

		/*0x10*/ RIVER_SPEC_MODIFIES_OP1 | RIVER_SPEC_MODIFIES_FLG, RIVER_SPEC_MODIFIES_OP1 | RIVER_SPEC_MODIFIES_FLG, RIVER_SPEC_MODIFIES_OP1 | RIVER_SPEC_MODIFIES_FLG, RIVER_SPEC_MODIFIES_OP1 | RIVER_SPEC_MODIFIES_FLG,
		/*0x14*/ RIVER_SPEC_MODIFIES_OP1 | RIVER_SPEC_MODIFIES_FLG, RIVER_SPEC_MODIFIES_OP1 | RIVER_SPEC_MODIFIES_FLG, RIVER_SPEC_MODIFIES_xSP, 0xFF,
		/*0x18*/ RIVER_SPEC_MODIFIES_OP1 | RIVER_SPEC_MODIFIES_FLG, RIVER_SPEC_MODIFIES_OP1 | RIVER_SPEC_MODIFIES_FLG, RIVER_SPEC_MODIFIES_OP1 | RIVER_SPEC_MODIFIES_FLG, RIVER_SPEC_MODIFIES_OP1 | RIVER_SPEC_MODIFIES_FLG,
		/*0x1C*/ RIVER_SPEC_MODIFIES_OP1 | RIVER_SPEC_MODIFIES_FLG, RIVER_SPEC_MODIFIES_OP1 | RIVER_SPEC_MODIFIES_FLG, 0xFF, 0xFF,

		/*0x20*/ RIVER_SPEC_MODIFIES_OP1 | RIVER_SPEC_MODIFIES_FLG, RIVER_SPEC_MODIFIES_OP1 | RIVER_SPEC_MODIFIES_FLG, RIVER_SPEC_MODIFIES_OP1 | RIVER_SPEC_MODIFIES_FLG, RIVER_SPEC_MODIFIES_OP1 | RIVER_SPEC_MODIFIES_FLG,
		/*0x24*/ RIVER_SPEC_MODIFIES_OP1 | RIVER_SPEC_MODIFIES_FLG, RIVER_SPEC_MODIFIES_OP1 | RIVER_SPEC_MODIFIES_FLG, 0xFF, 0xFF,
		/*0x28*/ RIVER_SPEC_MODIFIES_OP1 | RIVER_SPEC_MODIFIES_FLG, RIVER_SPEC_MODIFIES_OP1 | RIVER_SPEC_MODIFIES_FLG, RIVER_SPEC_MODIFIES_OP1 | RIVER_SPEC_MODIFIES_FLG, RIVER_SPEC_MODIFIES_OP1 | RIVER_SPEC_MODIFIES_FLG,
		/*0x2C*/ RIVER_SPEC_MODIFIES_OP1 | RIVER_SPEC_MODIFIES_FLG, RIVER_SPEC_MODIFIES_OP1 | RIVER_SPEC_MODIFIES_FLG, 0xFF, 0xFF,

		/*0x30*/ RIVER_SPEC_MODIFIES_OP1 | RIVER_SPEC_MODIFIES_FLG, RIVER_SPEC_MODIFIES_OP1 | RIVER_SPEC_MODIFIES_FLG, RIVER_SPEC_MODIFIES_OP1 | RIVER_SPEC_MODIFIES_FLG, RIVER_SPEC_MODIFIES_OP1 | RIVER_SPEC_MODIFIES_FLG,
		/*0x34*/ RIVER_SPEC_MODIFIES_OP1 | RIVER_SPEC_MODIFIES_FLG, RIVER_SPEC_MODIFIES_OP1 | RIVER_SPEC_MODIFIES_FLG, 0xFF, 0xFF,
		/*0x38*/ RIVER_SPEC_MODIFIES_FLG, RIVER_SPEC_MODIFIES_FLG, RIVER_SPEC_MODIFIES_FLG, RIVER_SPEC_MODIFIES_FLG,
		/*0x3C*/ RIVER_SPEC_MODIFIES_FLG, RIVER_SPEC_MODIFIES_FLG, 0xFF, 0xFF,

		/*0x40*/ RIVER_SPEC_MODIFIES_OP1 | RIVER_SPEC_MODIFIES_FLG, RIVER_SPEC_MODIFIES_OP1 | RIVER_SPEC_MODIFIES_FLG, RIVER_SPEC_MODIFIES_OP1 | RIVER_SPEC_MODIFIES_FLG, RIVER_SPEC_MODIFIES_OP1 | RIVER_SPEC_MODIFIES_FLG,
		/*0x44*/ RIVER_SPEC_MODIFIES_OP1 | RIVER_SPEC_MODIFIES_FLG, RIVER_SPEC_MODIFIES_OP1 | RIVER_SPEC_MODIFIES_FLG, RIVER_SPEC_MODIFIES_OP1 | RIVER_SPEC_MODIFIES_FLG, RIVER_SPEC_MODIFIES_OP1 | RIVER_SPEC_MODIFIES_FLG,
		/*0x48*/ RIVER_SPEC_MODIFIES_OP1 | RIVER_SPEC_MODIFIES_FLG, RIVER_SPEC_MODIFIES_OP1 | RIVER_SPEC_MODIFIES_FLG, RIVER_SPEC_MODIFIES_OP1 | RIVER_SPEC_MODIFIES_FLG, RIVER_SPEC_MODIFIES_OP1 | RIVER_SPEC_MODIFIES_FLG,
		/*0x4C*/ RIVER_SPEC_MODIFIES_OP1 | RIVER_SPEC_MODIFIES_FLG, RIVER_SPEC_MODIFIES_OP1 | RIVER_SPEC_MODIFIES_FLG, RIVER_SPEC_MODIFIES_OP1 | RIVER_SPEC_MODIFIES_FLG, RIVER_SPEC_MODIFIES_OP1 | RIVER_SPEC_MODIFIES_FLG,

		/*0x50*/ RIVER_SPEC_MODIFIES_xSP, RIVER_SPEC_MODIFIES_xSP, RIVER_SPEC_MODIFIES_xSP, RIVER_SPEC_MODIFIES_xSP,
		/*0x54*/ RIVER_SPEC_MODIFIES_xSP, RIVER_SPEC_MODIFIES_xSP, RIVER_SPEC_MODIFIES_xSP, RIVER_SPEC_MODIFIES_xSP,
		/*0x58*/ RIVER_SPEC_MODIFIES_OP1 | RIVER_SPEC_MODIFIES_xSP, RIVER_SPEC_MODIFIES_OP1 | RIVER_SPEC_MODIFIES_xSP, RIVER_SPEC_MODIFIES_OP1 | RIVER_SPEC_MODIFIES_xSP, RIVER_SPEC_MODIFIES_OP1 | RIVER_SPEC_MODIFIES_xSP,
		/*0x5C*/ RIVER_SPEC_MODIFIES_OP1 | RIVER_SPEC_MODIFIES_xSP, RIVER_SPEC_MODIFIES_OP1 | RIVER_SPEC_MODIFIES_xSP, RIVER_SPEC_MODIFIES_OP1 | RIVER_SPEC_MODIFIES_xSP, RIVER_SPEC_MODIFIES_OP1 | RIVER_SPEC_MODIFIES_xSP,

		/*0x60*/ 0xFF, 0xFF, 0xFF, 0xFF,
		/*0x64*/ 0, 0, 0, 0,
		/*0x68*/ RIVER_SPEC_MODIFIES_xSP, RIVER_SPEC_MODIFIES_OP1 | RIVER_SPEC_MODIFIES_FLG, RIVER_SPEC_MODIFIES_xSP, RIVER_SPEC_MODIFIES_OP1 | RIVER_SPEC_MODIFIES_FLG,
		/*0x6C*/ 0xFF, 0xFF, 0xFF, 0xFF,

		/*0x70*/ 0, 0, 0, 0,
		/*0x74*/ 0, 0, 0, 0,
		/*0x78*/ 0, 0, 0, 0,
		/*0x7C*/ 0, 0, 0, 0,

		/*0x80*/ RIVER_SPEC_MODIFIES_EXT, RIVER_SPEC_MODIFIES_EXT, RIVER_SPEC_MODIFIES_EXT, RIVER_SPEC_MODIFIES_EXT,
		/*0x84*/ RIVER_SPEC_MODIFIES_FLG, RIVER_SPEC_MODIFIES_FLG, RIVER_SPEC_MODIFIES_OP1 | RIVER_SPEC_MODIFIES_OP2, RIVER_SPEC_MODIFIES_OP1 | RIVER_SPEC_MODIFIES_OP2,
		/*0x88*/ RIVER_SPEC_MODIFIES_OP1, RIVER_SPEC_MODIFIES_OP1, RIVER_SPEC_MODIFIES_OP1, RIVER_SPEC_MODIFIES_OP1,
		/*0x8C*/ 0xFF, RIVER_SPEC_MODIFIES_OP1, 0xFF, 0xFF,

		/*0x90*/ RIVER_SPEC_MODIFIES_OP1 | RIVER_SPEC_MODIFIES_OP2, RIVER_SPEC_MODIFIES_OP1 | RIVER_SPEC_MODIFIES_OP2, RIVER_SPEC_MODIFIES_OP1 | RIVER_SPEC_MODIFIES_OP2, RIVER_SPEC_MODIFIES_OP1 | RIVER_SPEC_MODIFIES_OP2,
		/*0x94*/ RIVER_SPEC_MODIFIES_OP1 | RIVER_SPEC_MODIFIES_OP2, RIVER_SPEC_MODIFIES_OP1 | RIVER_SPEC_MODIFIES_OP2, RIVER_SPEC_MODIFIES_OP1 | RIVER_SPEC_MODIFIES_OP2, RIVER_SPEC_MODIFIES_OP1 | RIVER_SPEC_MODIFIES_OP2,
		/*0x98*/ 0xFF, RIVER_SPEC_MODIFIES_CUSTOM, 0xFF, 0xFF,
		/*0x9C*/ 0xFF, 0xFF, 0xFF, 0xFF,

		/*0xA0*/ RIVER_SPEC_MODIFIES_OP1, RIVER_SPEC_MODIFIES_OP1, RIVER_SPEC_MODIFIES_OP1, RIVER_SPEC_MODIFIES_OP1,
		/*0xA4*/ RIVER_SPEC_MODIFIES_CUSTOM | RIVER_SPEC_MODIFIES_FLG, RIVER_SPEC_MODIFIES_CUSTOM | RIVER_SPEC_MODIFIES_FLG, RIVER_SPEC_MODIFIES_CUSTOM | RIVER_SPEC_MODIFIES_FLG, RIVER_SPEC_MODIFIES_CUSTOM | RIVER_SPEC_MODIFIES_FLG,
		/*0xA8*/ RIVER_SPEC_MODIFIES_FLG, RIVER_SPEC_MODIFIES_FLG, RIVER_SPEC_MODIFIES_CUSTOM, RIVER_SPEC_MODIFIES_CUSTOM,
		/*0xAC*/ 0xFF, 0xFF, RIVER_SPEC_MODIFIES_CUSTOM | RIVER_SPEC_MODIFIES_FLG, RIVER_SPEC_MODIFIES_CUSTOM | RIVER_SPEC_MODIFIES_FLG,

		/*0xB0*/ RIVER_SPEC_MODIFIES_OP1, RIVER_SPEC_MODIFIES_OP1, RIVER_SPEC_MODIFIES_OP1, RIVER_SPEC_MODIFIES_OP1,
		/*0xB4*/ RIVER_SPEC_MODIFIES_OP1, RIVER_SPEC_MODIFIES_OP1, RIVER_SPEC_MODIFIES_OP1, RIVER_SPEC_MODIFIES_OP1,
		/*0xB8*/ RIVER_SPEC_MODIFIES_OP1, RIVER_SPEC_MODIFIES_OP1, RIVER_SPEC_MODIFIES_OP1, RIVER_SPEC_MODIFIES_OP1,
		/*0xBC*/ RIVER_SPEC_MODIFIES_OP1, RIVER_SPEC_MODIFIES_OP1, RIVER_SPEC_MODIFIES_OP1, RIVER_SPEC_MODIFIES_OP1,

		/*0xC0*/ RIVER_SPEC_MODIFIES_OP1 | RIVER_SPEC_MODIFIES_FLG, RIVER_SPEC_MODIFIES_OP1 | RIVER_SPEC_MODIFIES_FLG, RIVER_SPEC_MODIFIES_xSP, RIVER_SPEC_MODIFIES_xSP,
		/*0xC4*/ 0xFF, 0xFF, RIVER_SPEC_MODIFIES_OP1, RIVER_SPEC_MODIFIES_OP1,
		/*0xC8*/ 0xFF, RIVER_SPEC_MODIFIES_CUSTOM, 0xFF, 0xFF,
		/*0xCC*/ 0xFF, 0xFF, 0xFF, 0xFF,

		/*0xD0*/ RIVER_SPEC_MODIFIES_OP1 | RIVER_SPEC_MODIFIES_FLG, RIVER_SPEC_MODIFIES_OP1 | RIVER_SPEC_MODIFIES_FLG, RIVER_SPEC_MODIFIES_OP1 | RIVER_SPEC_MODIFIES_FLG, RIVER_SPEC_MODIFIES_OP1 | RIVER_SPEC_MODIFIES_FLG,
		/*0xD4*/ 0xFF, 0xFF, 0xFF, 0xFF,
		/*0xD8*/ 0xFF, 0xFF, 0xFF, 0xFF,
		/*0xDC*/ 0xFF, 0xFF, 0xFF, 0xFF,

		/*0xE0*/ 0xFF, 0xFF, 0xFF, 0xFF,
		/*0xE4*/ 0xFF, 0xFF, 0xFF, 0xFF,
		/*0xE8*/ 0, 0, 0, 0,
		/*0xEC*/ 0xFF, 0xFF, 0xFF, 0xFF,

		/*0xF0*/ 0, 0xFF, 0, 0,
		/*0xF4*/ 0xFF, 0xFF, RIVER_SPEC_MODIFIES_EXT | 2, RIVER_SPEC_MODIFIES_EXT | 2,
		/*0xF8*/ 0xFF, 0xFF, 0xFF, 0xFF,
		/*0xFC*/ RIVER_SPEC_MODIFIES_FLG, RIVER_SPEC_MODIFIES_FLG, RIVER_SPEC_MODIFIES_OP1 | RIVER_SPEC_MODIFIES_FLG, RIVER_SPEC_MODIFIES_EXT | 1
	}, {
		/*0x00*/ 0xFF, 0xFF, RIVER_SPEC_MODIFIES_OP1 | RIVER_SPEC_MODIFIES_FLG, RIVER_SPEC_MODIFIES_OP1 | RIVER_SPEC_MODIFIES_FLG,
		/*0x04*/ 0xFF, 0xFF, 0xFF, 0xFF,
		/*0x08*/ 0xFF, 0xFF, 0xFF, 0xFF,
		/*0x0C*/ 0xFF, 0xFF, 0xFF, 0,

		/*0x10*/ 0xFF, 0xFF, 0xFF, 0xFF,
		/*0x14*/ 0xFF, 0xFF, 0xFF, 0xFF,
		/*0x18*/ 0xFF, 0xFF, 0xFF, 0xFF,
		/*0x1C*/ 0xFF, 0xFF, 0xFF, 0xFF,

		/*0x20*/ 0xFF, 0xFF, 0xFF, 0xFF,
		/*0x24*/ 0xFF, 0xFF, 0xFF, 0xFF,
		/*0x28*/ 0xFF, 0xFF, 0xFF, 0xFF,
		/*0x2C*/ 0xFF, 0xFF, 0xFF, 0xFF,

		/*0x30*/ 0xFF, RIVER_SPEC_MODIFIES_CUSTOM, 0xFF, 0xFF,
		/*0x34*/ 0xFF, 0xFF, 0xFF, 0xFF,
		/*0x38*/ 0xFF, 0xFF, 0xFF, 0xFF,
		/*0x3C*/ 0xFF, 0xFF, 0xFF, 0xFF,

		/*0x40*/ RIVER_SPEC_MODIFIES_OP1, RIVER_SPEC_MODIFIES_OP1, RIVER_SPEC_MODIFIES_OP1, RIVER_SPEC_MODIFIES_OP1,
		/*0x44*/ RIVER_SPEC_MODIFIES_OP1, RIVER_SPEC_MODIFIES_OP1, RIVER_SPEC_MODIFIES_OP1, RIVER_SPEC_MODIFIES_OP1,
		/*0x48*/ RIVER_SPEC_MODIFIES_OP1, RIVER_SPEC_MODIFIES_OP1, RIVER_SPEC_MODIFIES_OP1, RIVER_SPEC_MODIFIES_OP1,
		/*0x4C*/ RIVER_SPEC_MODIFIES_OP1, RIVER_SPEC_MODIFIES_OP1, RIVER_SPEC_MODIFIES_OP1, RIVER_SPEC_MODIFIES_OP1,

		/*0x50*/ 0xFF, 0xFF, 0xFF, 0xFF,
		/*0x54*/ 0xFF, 0xFF, 0xFF, 0xFF,
		/*0x58*/ 0xFF, 0xFF, 0xFF, 0xFF,
		/*0x5C*/ 0xFF, 0xFF, 0xFF, 0xFF,

		/*0x60*/ 0xFF, 0xFF, 0xFF, 0xFF,
		/*0x64*/ 0xFF, 0xFF, 0xFF, 0xFF,
		/*0x68*/ 0xFF, 0xFF, 0xFF, 0xFF,
		/*0x6C*/ 0xFF, 0xFF, 0xFF, 0xFF,

		/*0x70*/ 0xFF, 0xFF, 0xFF, 0xFF,
		/*0x74*/ 0xFF, 0xFF, 0xFF, 0xFF,
		/*0x78*/ 0xFF, 0xFF, 0xFF, 0xFF,
		/*0x7C*/ 0xFF, 0xFF, 0xFF, 0xFF,

		/*0x80*/ 0, 0, 0, 0,
		/*0x84*/ 0, 0, 0, 0,
		/*0x88*/ 0, 0, 0, 0,
		/*0x8C*/ 0, 0, 0, 0,

		/*0x90*/ RIVER_SPEC_MODIFIES_OP1, RIVER_SPEC_MODIFIES_OP1, RIVER_SPEC_MODIFIES_OP1, RIVER_SPEC_MODIFIES_OP1,
		/*0x94*/ RIVER_SPEC_MODIFIES_OP1, RIVER_SPEC_MODIFIES_OP1, RIVER_SPEC_MODIFIES_OP1, RIVER_SPEC_MODIFIES_OP1,
		/*0x98*/ RIVER_SPEC_MODIFIES_OP1, RIVER_SPEC_MODIFIES_OP1, RIVER_SPEC_MODIFIES_OP1, RIVER_SPEC_MODIFIES_OP1,
		/*0x9C*/ RIVER_SPEC_MODIFIES_OP1, RIVER_SPEC_MODIFIES_OP1, RIVER_SPEC_MODIFIES_OP1, RIVER_SPEC_MODIFIES_OP1,

		/*0xA0*/ 0xFF, 0xFF, RIVER_SPEC_MODIFIES_CUSTOM, 0xFF,
		/*0xA4*/ 0xFF, RIVER_SPEC_MODIFIES_OP1 | RIVER_SPEC_MODIFIES_FLG, 0xFF, 0xFF,
		/*0xA8*/ 0xFF, 0xFF, 0xFF, 0xFF,
		/*0xAC*/ 0xFF, RIVER_SPEC_MODIFIES_OP1 | RIVER_SPEC_MODIFIES_FLG, 0xFF, RIVER_SPEC_MODIFIES_OP1 | RIVER_SPEC_MODIFIES_FLG,

		/*0xB0*/ RIVER_SPEC_MODIFIES_CUSTOM | RIVER_SPEC_MODIFIES_OP1 | RIVER_SPEC_MODIFIES_FLG, RIVER_SPEC_MODIFIES_CUSTOM | RIVER_SPEC_MODIFIES_OP1 | RIVER_SPEC_MODIFIES_FLG, 0xFF, 0xFF,
		/*0xB4*/ 0xFF, 0xFF, RIVER_SPEC_MODIFIES_OP1, RIVER_SPEC_MODIFIES_OP1,
		/*0xB8*/ 0xFF, 0xFF, RIVER_SPEC_MODIFIES_EXT | 3, 0xFF,
		/*0xBC*/ RIVER_SPEC_MODIFIES_OP1 | RIVER_SPEC_MODIFIES_FLG, RIVER_SPEC_MODIFIES_OP1 | RIVER_SPEC_MODIFIES_FLG, RIVER_SPEC_MODIFIES_OP1, RIVER_SPEC_MODIFIES_OP1,

		/*0xC0*/ RIVER_SPEC_MODIFIES_OP2 | RIVER_SPEC_MODIFIES_OP1 | RIVER_SPEC_MODIFIES_FLG, RIVER_SPEC_MODIFIES_OP2 | RIVER_SPEC_MODIFIES_OP1 | RIVER_SPEC_MODIFIES_FLG, 0xFF, 0xFF,
		/*0xC4*/ 0xFF, 0xFF, 0xFF, 0xFF,
		/*0xC8*/ 0xFF, RIVER_SPEC_MODIFIES_CUSTOM, 0xFF, 0xFF,
		/*0xCC*/ 0xFF, 0xFF, 0xFF, 0xFF,

		/*0xD0*/ 0xFF, 0xFF, 0xFF, 0xFF,
		/*0xD4*/ 0xFF, 0xFF, 0xFF, 0xFF,
		/*0xD8*/ 0xFF, 0xFF, 0xFF, 0xFF,
		/*0xDC*/ 0xFF, 0xFF, 0xFF, 0xFF,

		/*0xE0*/ 0xFF, 0xFF, 0xFF, 0xFF,
		/*0xE4*/ 0xFF, 0xFF, 0xFF, 0xFF,
		/*0xE8*/ 0xFF, 0xFF, 0xFF, 0xFF,
		/*0xEC*/ 0xFF, 0xFF, 0xFF, 0xFF,

		/*0xF0*/ 0xFF, 0xFF, 0xFF, 0xFF,
		/*0xF4*/ 0xFF, 0xFF, 0xFF, 0xFF,
		/*0xF8*/ 0xFF, 0xFF, 0xFF, 0xFF,
		/*0xFC*/ 0xFF, 0xFF, 0xFF, 0xFF
	}
};

const BYTE _specTblExt[][8] = {
	{ RIVER_SPEC_MODIFIES_OP1, RIVER_SPEC_MODIFIES_OP1, RIVER_SPEC_MODIFIES_OP1, RIVER_SPEC_MODIFIES_OP1, RIVER_SPEC_MODIFIES_OP1, RIVER_SPEC_MODIFIES_OP1, RIVER_SPEC_MODIFIES_OP1, 0 },
	/* for 0xFF    */{ RIVER_SPEC_MODIFIES_OP1, RIVER_SPEC_MODIFIES_OP1, RIVER_SPEC_MODIFIES_xSP, RIVER_SPEC_MODIFIES_xSP, 0, 0, 0, 0},
	/* for 0xF6/F7 */{ RIVER_SPEC_MODIFIES_FLG, RIVER_SPEC_MODIFIES_FLG, RIVER_SPEC_MODIFIES_OP1, RIVER_SPEC_MODIFIES_OP1, RIVER_SPEC_MODIFIES_CUSTOM | RIVER_SPEC_MODIFIES_FLG, RIVER_SPEC_MODIFIES_CUSTOM | RIVER_SPEC_MODIFIES_FLG, RIVER_SPEC_MODIFIES_CUSTOM | RIVER_SPEC_MODIFIES_FLG, RIVER_SPEC_MODIFIES_CUSTOM | RIVER_SPEC_MODIFIES_FLG },
	/* for 0x0FBA  */{ 0xFF, 0xFF, 0xFF, 0xFF, RIVER_SPEC_MODIFIES_FLG, RIVER_SPEC_MODIFIES_OP1 | RIVER_SPEC_MODIFIES_FLG, RIVER_SPEC_MODIFIES_OP1 | RIVER_SPEC_MODIFIES_FLG, RIVER_SPEC_MODIFIES_OP1 | RIVER_SPEC_MODIFIES_FLG },
};
