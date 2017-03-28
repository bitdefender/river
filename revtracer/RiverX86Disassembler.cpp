#include "RiverX86Disassembler.h"
#include "CodeGen.h"

extern const WORD specTbl[2][0x100];
extern const WORD _specTblExt[][8];

WORD GetSpecifiers(RiverInstruction &ri) {
	//const WORD *specTbl = specTbl00;
	DWORD dwTable = (RIVER_MODIFIER_EXT & ri.modifiers) ? 1 : 0;
	WORD tmp = specTbl[dwTable][ri.opCode];

	if (tmp == 0xFF) {
		DEBUG_BREAK;
	}

	if (tmp & RIVER_SPEC_MODIFIES_EXT) {
		tmp = _specTblExt[tmp & 0x7F][ri.subOpCode];
	}

	if (tmp == 0xFF) {
		DEBUG_BREAK;
	}

	return tmp;
}

void RiverX86Disassembler::TrackModifiedRegisters(RiverInstruction &ri) {
	//bool modifiysp = true;
	if (RIVER_SPEC_MODIFIES_xSP & ri.specifiers) {
		codegen->NextReg(RIVER_REG_xSP);
		//modifiysp = false;
	}

	if ((RIVER_SPEC_MODIFIES_OP2 & ri.specifiers) && (RIVER_OPTYPE_REG == RIVER_OPTYPE(ri.opTypes[1]))) {
		ri.operands[1].asRegister.versioned = codegen->NextReg(ri.operands[1].asRegister.name);
	}

	if ((RIVER_SPEC_MODIFIES_OP1 & ri.specifiers) && (RIVER_OPTYPE_REG == RIVER_OPTYPE(ri.opTypes[0]))) {
		ri.operands[0].asRegister.versioned = codegen->NextReg(ri.operands[0].asRegister.name);
	}
}

void RiverX86Disassembler::TrackFlagUsage(RiverInstruction &ri) {
	DWORD dwTable = (RIVER_MODIFIER_EXT & ri.modifiers) ? 1 : 0;
	BYTE tmp = modFlags[dwTable][ri.opCode];

	if (tmp == 0xFF) {
		DEBUG_BREAK;
	}

	if (tmp & RIVER_SPEC_FLAG_EXT) {
		tmp = 0; //modFlagExt[tmp & 0x7F][ri.subOpCode];
	}

	if (tmp == 0xFF) {
		DEBUG_BREAK;
	}

	ri.modFlags = tmp;

	tmp = testFlags[dwTable][ri.opCode];

	if (tmp == 0xFF) {
		DEBUG_BREAK;
	}

	if (tmp & RIVER_SPEC_FLAG_EXT) {
		tmp = 0; //testFlagExt[tmp & 0x7F][ri.subOpCode];
	}

	if (tmp == 0xFF) {
		DEBUG_BREAK;
	}

	ri.testFlags = tmp;
}

bool RiverX86Disassembler::Init(RiverCodeGen *cg) {
	codegen = cg;
	return true;
}

bool RiverX86Disassembler::Translate(BYTE *&px86, RiverInstruction &rOut, DWORD &flags) {
	DWORD dwTable = 0;
	
	rOut.modifiers = 0;
	rOut.specifiers = 0;
	rOut.family = 0;
	rOut.subOpCode = 0;
	rOut.opTypes[0] = rOut.opTypes[1] = rOut.opTypes[2] = rOut.opTypes[3] = RIVER_OPTYPE_NONE;
	rOut.instructionAddress = (DWORD)px86;
	rOut.modFlags = 0;
	rOut.testFlags = 0;

	flags = 0;
	do {
		dwTable = (rOut.modifiers & RIVER_MODIFIER_EXT) ? 1 : 0;

		flags &= ~RIVER_FLAG_PFX;
		(this->*disassembleOpcodes[dwTable][*px86])(px86, rOut, flags);
	} while (flags & RIVER_FLAG_PFX);

	dwTable = (rOut.modifiers & RIVER_MODIFIER_EXT) ? 1 : 0;
	
	(this->*disassembleOperands[dwTable][rOut.opCode])(px86, rOut);
	TrackModifiedRegisters(rOut);
	TrackFlagUsage(rOut);
	rOut.TrackEspAsParameter();
	rOut.TrackUnusedRegisters();
	return true;
}

/* =========================================== */
/* Opcode disassemblers                        */
/* =========================================== */

void RiverX86Disassembler::DisassembleUnkInstr(BYTE *&px86, RiverInstruction &ri, DWORD &flags) {
	static BYTE opcode, extPrefix;
	static DWORD address;

	revtracerAPI.dbgPrintFunc(PRINT_ERROR | PRINT_DISASSEMBLY, "Disassembling unknown instruction %02x %02x \n", ri.modifiers & RIVER_MODIFIER_EXT ? 0x0F : 0x00, *px86);
	opcode = *px86;
	extPrefix = (ri.modifiers & RIVER_MODIFIER_EXT) ? 0x0F : 0x00;
	address = ri.instructionAddress;
	DEBUG_BREAK;
	/*exit(0);*/
	px86++;
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

	WORD tmpMod = 0;

	BYTE opType = RIVER_OPTYPE_MEM;
	if (RIVER_MODIFIER_O8 & sz) {
		opType |= RIVER_OPSIZE_8;
		tmpMod |= RIVER_MODIFIER_O8;
	} else if (RIVER_MODIFIER_O16 & sz) {
		opType |= RIVER_OPSIZE_16;
		tmpMod |= RIVER_MODIFIER_O16;
	}

	rAddr = codegen->AllocAddr(ri.modifiers); //new RiverAddress;
	rAddr->DecodeFromx86(*codegen, px86, extra, ri.modifiers | tmpMod);
	
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
	DEBUG_BREAK;
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

void RiverX86Disassembler::DisassembleModRMRegImm8(BYTE *&px86, RiverInstruction &ri) {
	BYTE sec;
	DisassembleModRMOp(0, px86, ri, sec);
	DisassembleRegOp(1, ri, sec);
	DisassembleImmOp(2, px86, ri, RIVER_OPSIZE_8);
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
/* Specifier tables                            */
/* =========================================== */

const WORD specTbl[2][0x100] = {
		{
			/*0x00*/ RIVER_SPEC_MODIFIES_OP1 | RIVER_SPEC_MODIFIES_FLG,
			/*0x01*/ RIVER_SPEC_MODIFIES_OP1 | RIVER_SPEC_MODIFIES_FLG,
			/*0x02*/ RIVER_SPEC_MODIFIES_OP1 | RIVER_SPEC_MODIFIES_FLG,
			/*0x03*/ RIVER_SPEC_MODIFIES_OP1 | RIVER_SPEC_MODIFIES_FLG,
			/*0x04*/ RIVER_SPEC_MODIFIES_OP1 | RIVER_SPEC_MODIFIES_FLG,
			/*0x05*/ RIVER_SPEC_MODIFIES_OP1 | RIVER_SPEC_MODIFIES_FLG,
			/*0x06*/ RIVER_SPEC_MODIFIES_xSP | RIVER_SPEC_IGNORES_FLG,
			/*0x07*/ 0xFF,
			/*0x08*/ RIVER_SPEC_MODIFIES_OP1 | RIVER_SPEC_MODIFIES_FLG,
			/*0x09*/ RIVER_SPEC_MODIFIES_OP1 | RIVER_SPEC_MODIFIES_FLG,
			/*0x0A*/ RIVER_SPEC_MODIFIES_OP1 | RIVER_SPEC_MODIFIES_FLG,
			/*0x0B*/ RIVER_SPEC_MODIFIES_OP1 | RIVER_SPEC_MODIFIES_FLG,
			/*0x0C*/ RIVER_SPEC_MODIFIES_OP1 | RIVER_SPEC_MODIFIES_FLG,
			/*0x0D*/ RIVER_SPEC_MODIFIES_OP1 | RIVER_SPEC_MODIFIES_FLG,
			/*0x0E*/ 0xFF,
			/*0x0F*/ 0,
			/*0x10*/ RIVER_SPEC_MODIFIES_OP1 | RIVER_SPEC_MODIFIES_FLG,
			/*0x11*/ RIVER_SPEC_MODIFIES_OP1 | RIVER_SPEC_MODIFIES_FLG,
			/*0x12*/ RIVER_SPEC_MODIFIES_OP1 | RIVER_SPEC_MODIFIES_FLG,
			/*0x13*/ RIVER_SPEC_MODIFIES_OP1 | RIVER_SPEC_MODIFIES_FLG,
			/*0x14*/ RIVER_SPEC_MODIFIES_OP1 | RIVER_SPEC_MODIFIES_FLG,
			/*0x15*/ RIVER_SPEC_MODIFIES_OP1 | RIVER_SPEC_MODIFIES_FLG,
			/*0x16*/ RIVER_SPEC_MODIFIES_xSP | RIVER_SPEC_IGNORES_FLG,
			/*0x17*/ 0xFF,
			/*0x18*/ RIVER_SPEC_MODIFIES_OP1 | RIVER_SPEC_MODIFIES_FLG,
			/*0x19*/ RIVER_SPEC_MODIFIES_OP1 | RIVER_SPEC_MODIFIES_FLG,
			/*0x1A*/ RIVER_SPEC_MODIFIES_OP1 | RIVER_SPEC_MODIFIES_FLG,
			/*0x1B*/ RIVER_SPEC_MODIFIES_OP1 | RIVER_SPEC_MODIFIES_FLG,
			/*0x1C*/ RIVER_SPEC_MODIFIES_OP1 | RIVER_SPEC_MODIFIES_FLG,
			/*0x1D*/ RIVER_SPEC_MODIFIES_OP1 | RIVER_SPEC_MODIFIES_FLG,
			/*0x1E*/ 0xFF,
			/*0x1F*/ 0xFF,
			/*0x20*/ RIVER_SPEC_MODIFIES_OP1 | RIVER_SPEC_MODIFIES_FLG,
			/*0x21*/ RIVER_SPEC_MODIFIES_OP1 | RIVER_SPEC_MODIFIES_FLG,
			/*0x22*/ RIVER_SPEC_MODIFIES_OP1 | RIVER_SPEC_MODIFIES_FLG,
			/*0x23*/ RIVER_SPEC_MODIFIES_OP1 | RIVER_SPEC_MODIFIES_FLG,
			/*0x24*/ RIVER_SPEC_MODIFIES_OP1 | RIVER_SPEC_MODIFIES_FLG,
			/*0x25*/ RIVER_SPEC_MODIFIES_OP1 | RIVER_SPEC_MODIFIES_FLG,
			/*0x26*/ 0xFF,
			/*0x27*/ 0xFF,
			/*0x28*/ RIVER_SPEC_MODIFIES_OP1 | RIVER_SPEC_MODIFIES_FLG,
			/*0x29*/ RIVER_SPEC_MODIFIES_OP1 | RIVER_SPEC_MODIFIES_FLG,
			/*0x2A*/ RIVER_SPEC_MODIFIES_OP1 | RIVER_SPEC_MODIFIES_FLG,
			/*0x2B*/ RIVER_SPEC_MODIFIES_OP1 | RIVER_SPEC_MODIFIES_FLG,
			/*0x2C*/ RIVER_SPEC_MODIFIES_OP1 | RIVER_SPEC_MODIFIES_FLG,
			/*0x2D*/ RIVER_SPEC_MODIFIES_OP1 | RIVER_SPEC_MODIFIES_FLG,
			/*0x2E*/ 0xFF,
			/*0x2F*/ 0xFF,
			/*0x30*/ RIVER_SPEC_MODIFIES_OP1 | RIVER_SPEC_MODIFIES_FLG,
			/*0x31*/ RIVER_SPEC_MODIFIES_OP1 | RIVER_SPEC_MODIFIES_FLG,
			/*0x32*/ RIVER_SPEC_MODIFIES_OP1 | RIVER_SPEC_MODIFIES_FLG,
			/*0x33*/ RIVER_SPEC_MODIFIES_OP1 | RIVER_SPEC_MODIFIES_FLG,
			/*0x34*/ RIVER_SPEC_MODIFIES_OP1 | RIVER_SPEC_MODIFIES_FLG,
			/*0x35*/ RIVER_SPEC_MODIFIES_OP1 | RIVER_SPEC_MODIFIES_FLG,
			/*0x36*/ 0xFF,
			/*0x37*/ 0xFF,
			/*0x38*/ RIVER_SPEC_MODIFIES_FLG,
			/*0x39*/ RIVER_SPEC_MODIFIES_FLG,
			/*0x3A*/ RIVER_SPEC_MODIFIES_FLG,
			/*0x3B*/ RIVER_SPEC_MODIFIES_FLG,
			/*0x3C*/ RIVER_SPEC_MODIFIES_FLG,
			/*0x3D*/ RIVER_SPEC_MODIFIES_FLG,
			/*0x3E*/ 0xFF,
			/*0x3F*/ 0xFF,
			/*0x40*/ RIVER_SPEC_MODIFIES_OP1 | RIVER_SPEC_MODIFIES_FLG,
			/*0x41*/ RIVER_SPEC_MODIFIES_OP1 | RIVER_SPEC_MODIFIES_FLG,
			/*0x42*/ RIVER_SPEC_MODIFIES_OP1 | RIVER_SPEC_MODIFIES_FLG,
			/*0x43*/ RIVER_SPEC_MODIFIES_OP1 | RIVER_SPEC_MODIFIES_FLG,
			/*0x44*/ RIVER_SPEC_MODIFIES_OP1 | RIVER_SPEC_MODIFIES_FLG,
			/*0x45*/ RIVER_SPEC_MODIFIES_OP1 | RIVER_SPEC_MODIFIES_FLG,
			/*0x46*/ RIVER_SPEC_MODIFIES_OP1 | RIVER_SPEC_MODIFIES_FLG,
			/*0x47*/ RIVER_SPEC_MODIFIES_OP1 | RIVER_SPEC_MODIFIES_FLG,
			/*0x48*/ RIVER_SPEC_MODIFIES_OP1 | RIVER_SPEC_MODIFIES_FLG,
			/*0x49*/ RIVER_SPEC_MODIFIES_OP1 | RIVER_SPEC_MODIFIES_FLG,
			/*0x4A*/ RIVER_SPEC_MODIFIES_OP1 | RIVER_SPEC_MODIFIES_FLG,
			/*0x4B*/ RIVER_SPEC_MODIFIES_OP1 | RIVER_SPEC_MODIFIES_FLG,
			/*0x4C*/ RIVER_SPEC_MODIFIES_OP1 | RIVER_SPEC_MODIFIES_FLG,
			/*0x4D*/ RIVER_SPEC_MODIFIES_OP1 | RIVER_SPEC_MODIFIES_FLG,
			/*0x4E*/ RIVER_SPEC_MODIFIES_OP1 | RIVER_SPEC_MODIFIES_FLG,
			/*0x4F*/ RIVER_SPEC_MODIFIES_OP1 | RIVER_SPEC_MODIFIES_FLG,
			/*0x50*/ RIVER_SPEC_MODIFIES_OP2 | RIVER_SPEC_MODIFIES_OP3 | RIVER_SPEC_IGNORES_FLG | RIVER_SPEC_IGNORES_OP3,
			/*0x51*/ RIVER_SPEC_MODIFIES_OP2 | RIVER_SPEC_MODIFIES_OP3 | RIVER_SPEC_IGNORES_FLG | RIVER_SPEC_IGNORES_OP3,
			/*0x52*/ RIVER_SPEC_MODIFIES_OP2 | RIVER_SPEC_MODIFIES_OP3 | RIVER_SPEC_IGNORES_FLG | RIVER_SPEC_IGNORES_OP3,
			/*0x53*/ RIVER_SPEC_MODIFIES_OP2 | RIVER_SPEC_MODIFIES_OP3 | RIVER_SPEC_IGNORES_FLG | RIVER_SPEC_IGNORES_OP3,
			/*0x54*/ RIVER_SPEC_MODIFIES_OP2 | RIVER_SPEC_MODIFIES_OP3 | RIVER_SPEC_IGNORES_FLG | RIVER_SPEC_IGNORES_OP3,
			/*0x55*/ RIVER_SPEC_MODIFIES_OP2 | RIVER_SPEC_MODIFIES_OP3 | RIVER_SPEC_IGNORES_FLG | RIVER_SPEC_IGNORES_OP3,
			/*0x56*/ RIVER_SPEC_MODIFIES_OP2 | RIVER_SPEC_MODIFIES_OP3 | RIVER_SPEC_IGNORES_FLG | RIVER_SPEC_IGNORES_OP3,
			/*0x57*/ RIVER_SPEC_MODIFIES_OP2 | RIVER_SPEC_MODIFIES_OP3 | RIVER_SPEC_IGNORES_FLG | RIVER_SPEC_IGNORES_OP3,
			/*0x58*/ RIVER_SPEC_MODIFIES_OP1 | RIVER_SPEC_MODIFIES_OP2 | RIVER_SPEC_IGNORES_FLG | RIVER_SPEC_IGNORES_OP1,
			/*0x59*/ RIVER_SPEC_MODIFIES_OP1 | RIVER_SPEC_MODIFIES_OP2 | RIVER_SPEC_IGNORES_FLG | RIVER_SPEC_IGNORES_OP1,
			/*0x5A*/ RIVER_SPEC_MODIFIES_OP1 | RIVER_SPEC_MODIFIES_OP2 | RIVER_SPEC_IGNORES_FLG | RIVER_SPEC_IGNORES_OP1,
			/*0x5B*/ RIVER_SPEC_MODIFIES_OP1 | RIVER_SPEC_MODIFIES_OP2 | RIVER_SPEC_IGNORES_FLG | RIVER_SPEC_IGNORES_OP1,
			/*0x5C*/ RIVER_SPEC_MODIFIES_OP1 | RIVER_SPEC_MODIFIES_OP2 | RIVER_SPEC_IGNORES_FLG | RIVER_SPEC_IGNORES_OP1,
			/*0x5D*/ RIVER_SPEC_MODIFIES_OP1 | RIVER_SPEC_MODIFIES_OP2 | RIVER_SPEC_IGNORES_FLG | RIVER_SPEC_IGNORES_OP1,
			/*0x5E*/ RIVER_SPEC_MODIFIES_OP1 | RIVER_SPEC_MODIFIES_OP2 | RIVER_SPEC_IGNORES_FLG | RIVER_SPEC_IGNORES_OP1,
			/*0x5F*/ RIVER_SPEC_MODIFIES_OP1 | RIVER_SPEC_MODIFIES_OP2 | RIVER_SPEC_IGNORES_FLG | RIVER_SPEC_IGNORES_OP1,
			/*0x60*/ 0xFF,
			/*0x61*/ 0xFF,
			/*0x62*/ 0xFF,
			/*0x63*/ 0xFF,
			/*0x64*/ 0,
			/*0x65*/ 0,
			/*0x66*/ 0,
			/*0x67*/ 0,
			/*0x68*/ RIVER_SPEC_MODIFIES_OP2 | RIVER_SPEC_MODIFIES_OP3 | RIVER_SPEC_IGNORES_FLG | RIVER_SPEC_IGNORES_OP3,
			/*0x69*/ RIVER_SPEC_MODIFIES_OP1 | RIVER_SPEC_MODIFIES_FLG,
			/*0x6A*/ RIVER_SPEC_MODIFIES_OP2 | RIVER_SPEC_MODIFIES_OP3 | RIVER_SPEC_IGNORES_FLG | RIVER_SPEC_IGNORES_OP3,
			/*0x6B*/ RIVER_SPEC_MODIFIES_OP1 | RIVER_SPEC_MODIFIES_FLG,
			/*0x6C*/ 0xFF,
			/*0x6D*/ 0xFF,
			/*0x6E*/ 0xFF,
			/*0x6F*/ 0xFF,
			/*0x70*/ 0,
			/*0x71*/ 0,
			/*0x72*/ 0,
			/*0x73*/ 0,
			/*0x74*/ 0,
			/*0x75*/ 0,
			/*0x76*/ 0,
			/*0x77*/ 0,
			/*0x78*/ 0,
			/*0x79*/ 0,
			/*0x7A*/ 0,
			/*0x7B*/ 0,
			/*0x7C*/ 0,
			/*0x7D*/ 0,
			/*0x7E*/ 0,
			/*0x7F*/ 0,
			/*0x80*/ RIVER_SPEC_MODIFIES_EXT,
			/*0x81*/ RIVER_SPEC_MODIFIES_EXT,
			/*0x82*/ RIVER_SPEC_MODIFIES_EXT,
			/*0x83*/ RIVER_SPEC_MODIFIES_EXT,
			/*0x84*/ RIVER_SPEC_MODIFIES_FLG,
			/*0x85*/ RIVER_SPEC_MODIFIES_FLG,
			/*0x86*/ RIVER_SPEC_MODIFIES_OP1 | RIVER_SPEC_MODIFIES_OP2 | RIVER_SPEC_IGNORES_FLG,
			/*0x87*/ RIVER_SPEC_MODIFIES_OP1 | RIVER_SPEC_MODIFIES_OP2 | RIVER_SPEC_IGNORES_FLG,
			/*0x88*/ RIVER_SPEC_MODIFIES_OP1 | RIVER_SPEC_IGNORES_FLG | RIVER_SPEC_IGNORES_OP1,
			/*0x89*/ RIVER_SPEC_MODIFIES_OP1 | RIVER_SPEC_IGNORES_FLG | RIVER_SPEC_IGNORES_OP1,
			/*0x8A*/ RIVER_SPEC_MODIFIES_OP1 | RIVER_SPEC_IGNORES_FLG | RIVER_SPEC_IGNORES_OP1,
			/*0x8B*/ RIVER_SPEC_MODIFIES_OP1 | RIVER_SPEC_IGNORES_FLG | RIVER_SPEC_IGNORES_OP1,
			/*0x8C*/ 0xFF,
			/*0x8D*/ RIVER_SPEC_MODIFIES_OP1 | RIVER_SPEC_IGNORES_FLG | RIVER_SPEC_IGNORES_OP1 | RIVER_SPEC_IGNORES_MEMORY,
			/*0x8E*/ 0xFF,
			/*0x8F*/ 0xFF,
			/*0x90*/ RIVER_SPEC_MODIFIES_OP1 | RIVER_SPEC_MODIFIES_OP2 | RIVER_SPEC_IGNORES_FLG,
			/*0x91*/ RIVER_SPEC_MODIFIES_OP1 | RIVER_SPEC_MODIFIES_OP2 | RIVER_SPEC_IGNORES_FLG,
			/*0x92*/ RIVER_SPEC_MODIFIES_OP1 | RIVER_SPEC_MODIFIES_OP2 | RIVER_SPEC_IGNORES_FLG,
			/*0x93*/ RIVER_SPEC_MODIFIES_OP1 | RIVER_SPEC_MODIFIES_OP2 | RIVER_SPEC_IGNORES_FLG,
			/*0x94*/ RIVER_SPEC_MODIFIES_OP1 | RIVER_SPEC_MODIFIES_OP2 | RIVER_SPEC_IGNORES_FLG,
			/*0x95*/ RIVER_SPEC_MODIFIES_OP1 | RIVER_SPEC_MODIFIES_OP2 | RIVER_SPEC_IGNORES_FLG,
			/*0x96*/ RIVER_SPEC_MODIFIES_OP1 | RIVER_SPEC_MODIFIES_OP2 | RIVER_SPEC_IGNORES_FLG,
			/*0x97*/ RIVER_SPEC_MODIFIES_OP1 | RIVER_SPEC_MODIFIES_OP2 | RIVER_SPEC_IGNORES_FLG,
			/*0x98*/ 0xFF,
			/*0x99*/ RIVER_SPEC_MODIFIES_OP1 | RIVER_SPEC_IGNORES_FLG | RIVER_SPEC_IGNORES_OP1,
			/*0x9A*/ 0xFF,
			/*0x9B*/ 0xFF,
			/*0x9C*/ 0xFF,
			/*0x9D*/ 0xFF,
			/*0x9E*/ 0xFF,
			/*0x9F*/ 0xFF,
			/*0xA0*/ RIVER_SPEC_MODIFIES_OP1 | RIVER_SPEC_IGNORES_FLG | RIVER_SPEC_IGNORES_OP1,
			/*0xA1*/ RIVER_SPEC_MODIFIES_OP1 | RIVER_SPEC_IGNORES_FLG | RIVER_SPEC_IGNORES_OP1,
			/*0xA2*/ RIVER_SPEC_MODIFIES_OP1 | RIVER_SPEC_IGNORES_FLG | RIVER_SPEC_IGNORES_OP1,
			/*0xA3*/ RIVER_SPEC_MODIFIES_OP1 | RIVER_SPEC_IGNORES_FLG | RIVER_SPEC_IGNORES_OP1,
			/*0xA4*/ RIVER_SPEC_MODIFIES_OP1 | RIVER_SPEC_MODIFIES_OP2 | RIVER_SPEC_MODIFIES_OP3 | RIVER_SPEC_IGNORES_OP3,
			/*0xA5*/ RIVER_SPEC_MODIFIES_OP1 | RIVER_SPEC_MODIFIES_OP2 | RIVER_SPEC_MODIFIES_OP3 | RIVER_SPEC_IGNORES_OP3, 
			/*0xA6*/ RIVER_SPEC_MODIFIES_OP1 | RIVER_SPEC_MODIFIES_OP2 | RIVER_SPEC_MODIFIES_FLG,
			/*0xA7*/ RIVER_SPEC_MODIFIES_OP1 | RIVER_SPEC_MODIFIES_OP2 | RIVER_SPEC_MODIFIES_FLG,
			/*0xA8*/ RIVER_SPEC_MODIFIES_FLG,
			/*0xA9*/ RIVER_SPEC_MODIFIES_FLG,
			/*0xAA*/ RIVER_SPEC_MODIFIES_OP1 | RIVER_SPEC_MODIFIES_OP3 | RIVER_SPEC_IGNORES_OP3,
			/*0xAB*/ RIVER_SPEC_MODIFIES_OP1 | RIVER_SPEC_MODIFIES_OP3 | RIVER_SPEC_IGNORES_OP3,
			/*0xAC*/ RIVER_SPEC_MODIFIES_OP1 | RIVER_SPEC_MODIFIES_OP3 | RIVER_SPEC_IGNORES_OP3,
			/*0xAD*/ RIVER_SPEC_MODIFIES_OP1 | RIVER_SPEC_MODIFIES_OP3 | RIVER_SPEC_IGNORES_OP3,
			/*0xAE*/ RIVER_SPEC_MODIFIES_OP1 | RIVER_SPEC_MODIFIES_FLG,
			/*0xAF*/ RIVER_SPEC_MODIFIES_OP1 | RIVER_SPEC_MODIFIES_FLG,
			/*0xB0*/ RIVER_SPEC_MODIFIES_OP1 | RIVER_SPEC_IGNORES_FLG | RIVER_SPEC_IGNORES_OP1,
			/*0xB1*/ RIVER_SPEC_MODIFIES_OP1 | RIVER_SPEC_IGNORES_FLG | RIVER_SPEC_IGNORES_OP1,
			/*0xB2*/ RIVER_SPEC_MODIFIES_OP1 | RIVER_SPEC_IGNORES_FLG | RIVER_SPEC_IGNORES_OP1,
			/*0xB3*/ RIVER_SPEC_MODIFIES_OP1 | RIVER_SPEC_IGNORES_FLG | RIVER_SPEC_IGNORES_OP1,
			/*0xB4*/ RIVER_SPEC_MODIFIES_OP1 | RIVER_SPEC_IGNORES_FLG | RIVER_SPEC_IGNORES_OP1,
			/*0xB5*/ RIVER_SPEC_MODIFIES_OP1 | RIVER_SPEC_IGNORES_FLG | RIVER_SPEC_IGNORES_OP1,
			/*0xB6*/ RIVER_SPEC_MODIFIES_OP1 | RIVER_SPEC_IGNORES_FLG | RIVER_SPEC_IGNORES_OP1,
			/*0xB7*/ RIVER_SPEC_MODIFIES_OP1 | RIVER_SPEC_IGNORES_FLG | RIVER_SPEC_IGNORES_OP1,
			/*0xB8*/ RIVER_SPEC_MODIFIES_OP1 | RIVER_SPEC_IGNORES_FLG | RIVER_SPEC_IGNORES_OP1,
			/*0xB9*/ RIVER_SPEC_MODIFIES_OP1 | RIVER_SPEC_IGNORES_FLG | RIVER_SPEC_IGNORES_OP1,
			/*0xBA*/ RIVER_SPEC_MODIFIES_OP1 | RIVER_SPEC_IGNORES_FLG | RIVER_SPEC_IGNORES_OP1,
			/*0xBB*/ RIVER_SPEC_MODIFIES_OP1 | RIVER_SPEC_IGNORES_FLG | RIVER_SPEC_IGNORES_OP1,
			/*0xBC*/ RIVER_SPEC_MODIFIES_OP1 | RIVER_SPEC_IGNORES_FLG | RIVER_SPEC_IGNORES_OP1,
			/*0xBD*/ RIVER_SPEC_MODIFIES_OP1 | RIVER_SPEC_IGNORES_FLG | RIVER_SPEC_IGNORES_OP1,
			/*0xBE*/ RIVER_SPEC_MODIFIES_OP1 | RIVER_SPEC_IGNORES_FLG | RIVER_SPEC_IGNORES_OP1,
			/*0xBF*/ RIVER_SPEC_MODIFIES_OP1 | RIVER_SPEC_IGNORES_FLG | RIVER_SPEC_IGNORES_OP1,
			/*0xC0*/ RIVER_SPEC_MODIFIES_OP1 | RIVER_SPEC_MODIFIES_FLG,
			/*0xC1*/ RIVER_SPEC_MODIFIES_OP1 | RIVER_SPEC_MODIFIES_FLG,
			/*0xC2*/ RIVER_SPEC_MODIFIES_OP2 | RIVER_SPEC_MODIFIES_OP3 | RIVER_SPEC_IGNORES_FLG,
			/*0xC3*/ RIVER_SPEC_MODIFIES_OP1 | RIVER_SPEC_MODIFIES_OP2 | RIVER_SPEC_IGNORES_FLG,
			/*0xC4*/ 0xFF,
			/*0xC5*/ 0xFF,
			/*0xC6*/ RIVER_SPEC_MODIFIES_OP1 | RIVER_SPEC_IGNORES_FLG | RIVER_SPEC_IGNORES_OP1,
			/*0xC7*/ RIVER_SPEC_MODIFIES_OP1 | RIVER_SPEC_IGNORES_FLG | RIVER_SPEC_IGNORES_OP1,
			/*0xC8*/ 0xFF,
			/*0xC9*/ RIVER_SPEC_MODIFIES_OP1 | RIVER_SPEC_MODIFIES_OP2 | RIVER_SPEC_IGNORES_OP1 | RIVER_SPEC_IGNORES_OP2 | RIVER_SPEC_IGNORES_FLG,
			/*0xCA*/ 0xFF,
			/*0xCB*/ 0xFF,
			/*0xCC*/ 0xFF,
			/*0xCD*/ 0xFF,
			/*0xCE*/ 0xFF,
			/*0xCF*/ 0xFF,
			/*0xD0*/ RIVER_SPEC_MODIFIES_OP1 | RIVER_SPEC_MODIFIES_FLG,
			/*0xD1*/ RIVER_SPEC_MODIFIES_OP1 | RIVER_SPEC_MODIFIES_FLG,
			/*0xD2*/ RIVER_SPEC_MODIFIES_OP1 | RIVER_SPEC_MODIFIES_FLG,
			/*0xD3*/ RIVER_SPEC_MODIFIES_OP1 | RIVER_SPEC_MODIFIES_FLG,
			/*0xD4*/ 0xFF,
			/*0xD5*/ 0xFF,
			/*0xD6*/ 0xFF,
			/*0xD7*/ 0xFF,
			/*0xD8*/ 0xFF,
			/*0xD9*/ 0xFF,
			/*0xDA*/ 0xFF,
			/*0xDB*/ 0xFF,
			/*0xDC*/ 0xFF,
			/*0xDD*/ 0xFF,
			/*0xDE*/ 0xFF,
			/*0xDF*/ 0xFF,
			/*0xE0*/ 0xFF,
			/*0xE1*/ 0xFF,
			/*0xE2*/ 0xFF,
			/*0xE3*/ 0,
			/*0xE4*/ 0xFF,
			/*0xE5*/ 0xFF,
			/*0xE6*/ 0xFF,
			/*0xE7*/ 0xFF,
			/*0xE8*/ RIVER_SPEC_MODIFIES_OP3 | RIVER_SPEC_MODIFIES_OP4 | RIVER_SPEC_IGNORES_FLG | RIVER_SPEC_IGNORES_OP4,
			/*0xE9*/ 0,
			/*0xEA*/ 0,
			/*0xEB*/ 0,
			/*0xEC*/ 0xFF,
			/*0xED*/ 0xFF,
			/*0xEE*/ 0xFF,
			/*0xEF*/ 0xFF,
			/*0xF0*/ 0,
			/*0xF1*/ 0xFF,
			/*0xF2*/ 0,
			/*0xF3*/ 0,
			/*0xF4*/ 0xFF,
			/*0xF5*/ 0xFF,
			/*0xF6*/ RIVER_SPEC_MODIFIES_EXT | 2,
			/*0xF7*/ RIVER_SPEC_MODIFIES_EXT | 2,
			/*0xF8*/ 0xFF,
			/*0xF9*/ 0xFF,
			/*0xFA*/ 0xFF,
			/*0xFB*/ 0xFF,
			/*0xFC*/ RIVER_SPEC_MODIFIES_FLG,
			/*0xFD*/ RIVER_SPEC_MODIFIES_FLG,
			/*0xFE*/ RIVER_SPEC_MODIFIES_OP1 | RIVER_SPEC_MODIFIES_FLG,
			/*0xFF*/ RIVER_SPEC_MODIFIES_EXT | 1
		}, {
			/*0x00*/ 0xFF,
			/*0x01*/ 0xFF,
			/*0x02*/ RIVER_SPEC_MODIFIES_OP1 | RIVER_SPEC_MODIFIES_FLG,
			/*0x03*/ RIVER_SPEC_MODIFIES_OP1 | RIVER_SPEC_MODIFIES_FLG,
			/*0x04*/ 0xFF,
			/*0x05*/ 0x00,
			/*0x06*/ 0xFF,
			/*0x07*/ 0xFF,
			/*0x08*/ 0xFF,
			/*0x09*/ 0xFF,
			/*0x0A*/ 0xFF,
			/*0x0B*/ 0xFF,
			/*0x0C*/ 0xFF,
			/*0x0D*/ 0xFF,
			/*0x0E*/ 0xFF,
			/*0x0F*/ 0,
			/*0x10*/ 0xFF,
			/*0x11*/ 0xFF,
			/*0x12*/ 0xFF,
			/*0x13*/ 0xFF,
			/*0x14*/ 0xFF,
			/*0x15*/ 0xFF,
			/*0x16*/ 0xFF,
			/*0x17*/ 0xFF,
			/*0x18*/ 0xFF,
			/*0x19*/ 0xFF,
			/*0x1A*/ 0xFF,
			/*0x1B*/ 0xFF,
			/*0x1C*/ 0xFF,
			/*0x1D*/ 0xFF,
			/*0x1E*/ 0xFF,
			/*0x1F*/ 0xFF,
			/*0x20*/ 0xFF,
			/*0x21*/ 0xFF,
			/*0x22*/ 0xFF,
			/*0x23*/ 0xFF,
			/*0x24*/ 0xFF,
			/*0x25*/ 0xFF,
			/*0x26*/ 0xFF,
			/*0x27*/ 0xFF,
			/*0x28*/ 0xFF,
			/*0x29*/ 0xFF,
			/*0x2A*/ 0xFF,
			/*0x2B*/ 0xFF,
			/*0x2C*/ 0xFF,
			/*0x2D*/ 0xFF,
			/*0x2E*/ 0xFF,
			/*0x2F*/ 0xFF,
			/*0x30*/ 0xFF,
			/*0x31*/ RIVER_SPEC_MODIFIES_OP1 | RIVER_SPEC_MODIFIES_OP2 | RIVER_SPEC_IGNORES_OP1 | RIVER_SPEC_IGNORES_OP2 | RIVER_SPEC_IGNORES_FLG,
			/*0x32*/ 0xFF,
			/*0x33*/ 0xFF,
			/*0x34*/ 0x00,
			/*0x35*/ 0xFF,
			/*0x36*/ 0xFF,
			/*0x37*/ 0xFF,
			/*0x38*/ 0xFF,
			/*0x39*/ 0xFF,
			/*0x3A*/ 0xFF,
			/*0x3B*/ 0xFF,
			/*0x3C*/ 0xFF,
			/*0x3D*/ 0xFF,
			/*0x3E*/ 0xFF,
			/*0x3F*/ 0xFF,
			/*0x40*/ RIVER_SPEC_MODIFIES_OP1,
			/*0x41*/ RIVER_SPEC_MODIFIES_OP1,
			/*0x42*/ RIVER_SPEC_MODIFIES_OP1,
			/*0x43*/ RIVER_SPEC_MODIFIES_OP1,
			/*0x44*/ RIVER_SPEC_MODIFIES_OP1,
			/*0x45*/ RIVER_SPEC_MODIFIES_OP1,
			/*0x46*/ RIVER_SPEC_MODIFIES_OP1,
			/*0x47*/ RIVER_SPEC_MODIFIES_OP1,
			/*0x48*/ RIVER_SPEC_MODIFIES_OP1,
			/*0x49*/ RIVER_SPEC_MODIFIES_OP1,
			/*0x4A*/ RIVER_SPEC_MODIFIES_OP1,
			/*0x4B*/ RIVER_SPEC_MODIFIES_OP1,
			/*0x4C*/ RIVER_SPEC_MODIFIES_OP1,
			/*0x4D*/ RIVER_SPEC_MODIFIES_OP1,
			/*0x4E*/ RIVER_SPEC_MODIFIES_OP1,
			/*0x4F*/ RIVER_SPEC_MODIFIES_OP1,
			/*0x50*/ 0xFF,
			/*0x51*/ 0xFF,
			/*0x52*/ 0xFF,
			/*0x53*/ 0xFF,
			/*0x54*/ 0xFF,
			/*0x55*/ 0xFF,
			/*0x56*/ 0xFF,
			/*0x57*/ 0xFF,
			/*0x58*/ 0xFF,
			/*0x59*/ 0xFF,
			/*0x5A*/ 0xFF,
			/*0x5B*/ 0xFF,
			/*0x5C*/ 0xFF,
			/*0x5D*/ 0xFF,
			/*0x5E*/ 0xFF,
			/*0x5F*/ 0xFF,
			/*0x60*/ 0xFF,
			/*0x61*/ 0xFF,
			/*0x62*/ 0xFF,
			/*0x63*/ 0xFF,
			/*0x64*/ 0xFF,
			/*0x65*/ 0xFF,
			/*0x66*/ 0xFF,
			/*0x67*/ 0xFF,
			/*0x68*/ 0xFF,
			/*0x69*/ 0xFF,
			/*0x6A*/ 0xFF,
			/*0x6B*/ 0xFF,
			/*0x6C*/ 0xFF,
			/*0x6D*/ 0xFF,
			/*0x6E*/ 0xFF,
			/*0x6F*/ 0xFF,
			/*0x70*/ 0xFF,
			/*0x71*/ 0xFF,
			/*0x72*/ 0xFF,
			/*0x73*/ 0xFF,
			/*0x74*/ 0xFF,
			/*0x75*/ 0xFF,
			/*0x76*/ 0xFF,
			/*0x77*/ 0xFF,
			/*0x78*/ 0xFF,
			/*0x79*/ 0xFF,
			/*0x7A*/ 0xFF,
			/*0x7B*/ 0xFF,
			/*0x7C*/ 0xFF,
			/*0x7D*/ 0xFF,
			/*0x7E*/ 0xFF,
			/*0x7F*/ 0xFF,
			/*0x80*/ 0,
			/*0x81*/ 0,
			/*0x82*/ 0,
			/*0x83*/ 0,
			/*0x84*/ 0,
			/*0x85*/ 0,
			/*0x86*/ 0,
			/*0x87*/ 0,
			/*0x88*/ 0,
			/*0x89*/ 0,
			/*0x8A*/ 0,
			/*0x8B*/ 0,
			/*0x8C*/ 0,
			/*0x8D*/ 0,
			/*0x8E*/ 0,
			/*0x8F*/ 0,
			/*0x90*/ RIVER_SPEC_MODIFIES_OP1 | RIVER_SPEC_IGNORES_OP1,
			/*0x91*/ RIVER_SPEC_MODIFIES_OP1 | RIVER_SPEC_IGNORES_OP1,
			/*0x92*/ RIVER_SPEC_MODIFIES_OP1 | RIVER_SPEC_IGNORES_OP1,
			/*0x93*/ RIVER_SPEC_MODIFIES_OP1 | RIVER_SPEC_IGNORES_OP1,
			/*0x94*/ RIVER_SPEC_MODIFIES_OP1 | RIVER_SPEC_IGNORES_OP1,
			/*0x95*/ RIVER_SPEC_MODIFIES_OP1 | RIVER_SPEC_IGNORES_OP1,
			/*0x96*/ RIVER_SPEC_MODIFIES_OP1 | RIVER_SPEC_IGNORES_OP1,
			/*0x97*/ RIVER_SPEC_MODIFIES_OP1 | RIVER_SPEC_IGNORES_OP1,
			/*0x98*/ RIVER_SPEC_MODIFIES_OP1 | RIVER_SPEC_IGNORES_OP1,
			/*0x99*/ RIVER_SPEC_MODIFIES_OP1 | RIVER_SPEC_IGNORES_OP1,
			/*0x9A*/ RIVER_SPEC_MODIFIES_OP1 | RIVER_SPEC_IGNORES_OP1,
			/*0x9B*/ RIVER_SPEC_MODIFIES_OP1 | RIVER_SPEC_IGNORES_OP1,
			/*0x9C*/ RIVER_SPEC_MODIFIES_OP1 | RIVER_SPEC_IGNORES_OP1,
			/*0x9D*/ RIVER_SPEC_MODIFIES_OP1 | RIVER_SPEC_IGNORES_OP1,
			/*0x9E*/ RIVER_SPEC_MODIFIES_OP1 | RIVER_SPEC_IGNORES_OP1,
			/*0x9F*/ RIVER_SPEC_MODIFIES_OP1 | RIVER_SPEC_IGNORES_OP1,
			/*0xA0*/ 0xFF,
			/*0xA1*/ 0xFF,
			/*0xA2*/ RIVER_SPEC_MODIFIES_CUSTOM,
			/*0xA3*/ RIVER_SPEC_MODIFIES_FLG,
			/*0xA4*/ RIVER_SPEC_MODIFIES_OP1 | RIVER_SPEC_MODIFIES_FLG,
			/*0xA5*/ RIVER_SPEC_MODIFIES_OP1 | RIVER_SPEC_MODIFIES_FLG,
			/*0xA6*/ 0xFF,
			/*0xA7*/ 0xFF,
			/*0xA8*/ 0xFF,
			/*0xA9*/ 0xFF,
			/*0xAA*/ 0xFF,
			/*0xAB*/ RIVER_SPEC_MODIFIES_OP1 | RIVER_SPEC_MODIFIES_FLG,
			/*0xAC*/ RIVER_SPEC_MODIFIES_OP1 | RIVER_SPEC_MODIFIES_FLG,
			/*0xAD*/ RIVER_SPEC_MODIFIES_OP1 | RIVER_SPEC_MODIFIES_FLG,
			/*0xAE*/ 0xFF,
			/*0xAF*/ RIVER_SPEC_MODIFIES_OP1 | RIVER_SPEC_MODIFIES_FLG,
			/*0xB0*/ RIVER_SPEC_MODIFIES_OP1 | RIVER_SPEC_MODIFIES_OP3 | RIVER_SPEC_MODIFIES_FLG,
			/*0xB1*/ RIVER_SPEC_MODIFIES_OP1 | RIVER_SPEC_MODIFIES_OP3 | RIVER_SPEC_MODIFIES_FLG,
			/*0xB2*/ 0xFF,
			/*0xB3*/ RIVER_SPEC_MODIFIES_OP1 | RIVER_SPEC_MODIFIES_FLG,
			/*0xB4*/ 0xFF,
			/*0xB5*/ 0xFF,
			/*0xB6*/ RIVER_SPEC_MODIFIES_OP1 | RIVER_SPEC_IGNORES_FLG | RIVER_SPEC_IGNORES_OP1,
			/*0xB7*/ RIVER_SPEC_MODIFIES_OP1 | RIVER_SPEC_IGNORES_FLG | RIVER_SPEC_IGNORES_OP1,
			/*0xB8*/ 0xFF,
			/*0xB9*/ 0xFF,
			/*0xBA*/ RIVER_SPEC_MODIFIES_EXT | 3,
			/*0xBB*/ 0xFF,
			/*0xBC*/ RIVER_SPEC_MODIFIES_OP1 | RIVER_SPEC_MODIFIES_FLG,
			/*0xBD*/ RIVER_SPEC_MODIFIES_OP1 | RIVER_SPEC_MODIFIES_FLG,
			/*0xBE*/ RIVER_SPEC_MODIFIES_OP1 | RIVER_SPEC_IGNORES_FLG | RIVER_SPEC_IGNORES_OP1,
			/*0xBF*/ RIVER_SPEC_MODIFIES_OP1 | RIVER_SPEC_IGNORES_FLG | RIVER_SPEC_IGNORES_OP1,
			/*0xC0*/ RIVER_SPEC_MODIFIES_OP2 | RIVER_SPEC_MODIFIES_OP1 | RIVER_SPEC_MODIFIES_FLG,
			/*0xC1*/ RIVER_SPEC_MODIFIES_OP2 | RIVER_SPEC_MODIFIES_OP1 | RIVER_SPEC_MODIFIES_FLG,
			/*0xC2*/ 0xFF,
			/*0xC3*/ 0xFF,
			/*0xC4*/ 0xFF,
			/*0xC5*/ 0xFF,
			/*0xC6*/ 0xFF,
			/*0xC7*/ RIVER_SPEC_MODIFIES_CUSTOM,
			/*0xC8*/ 0xFF,
			/*0xC9*/ 0xFF,
			/*0xCA*/ 0xFF,
			/*0xCB*/ 0xFF,
			/*0xCC*/ 0xFF,
			/*0xCD*/ 0xFF,
			/*0xCE*/ 0xFF,
			/*0xCF*/ 0xFF,
			/*0xD0*/ 0xFF,
			/*0xD1*/ 0xFF,
			/*0xD2*/ 0xFF,
			/*0xD3*/ 0xFF,
			/*0xD4*/ 0xFF,
			/*0xD5*/ 0xFF,
			/*0xD6*/ 0xFF,
			/*0xD7*/ 0xFF,
			/*0xD8*/ 0xFF,
			/*0xD9*/ 0xFF,
			/*0xDA*/ 0xFF,
			/*0xDB*/ 0xFF,
			/*0xDC*/ 0xFF,
			/*0xDD*/ 0xFF,
			/*0xDE*/ 0xFF,
			/*0xDF*/ 0xFF,
			/*0xE0*/ 0xFF,
			/*0xE1*/ 0xFF,
			/*0xE2*/ 0xFF,
			/*0xE3*/ 0xFF,
			/*0xE4*/ 0xFF,
			/*0xE5*/ 0xFF,
			/*0xE6*/ 0xFF,
			/*0xE7*/ 0xFF,
			/*0xE8*/ 0xFF,
			/*0xE9*/ 0xFF,
			/*0xEA*/ 0xFF,
			/*0xEB*/ 0xFF,
			/*0xEC*/ 0xFF,
			/*0xED*/ 0xFF,
			/*0xEE*/ 0xFF,
			/*0xEF*/ 0xFF,
			/*0xF0*/ 0xFF,
			/*0xF1*/ 0xFF,
			/*0xF2*/ 0xFF,
			/*0xF3*/ 0xFF,
			/*0xF4*/ 0xFF,
			/*0xF5*/ 0xFF,
			/*0xF6*/ 0xFF,
			/*0xF7*/ 0xFF,
			/*0xF8*/ 0xFF,
			/*0xF9*/ 0xFF,
			/*0xFA*/ 0xFF,
			/*0xFB*/ 0xFF,
			/*0xFC*/ 0xFF,
			/*0xFD*/ 0xFF,
			/*0xFE*/ 0xFF,
			/*0xFF*/ 0xFF
		}
};

const WORD _specTblExt[][8] = {
		{ 
			RIVER_SPEC_MODIFIES_OP1 | RIVER_SPEC_MODIFIES_FLG, 
			RIVER_SPEC_MODIFIES_OP1 | RIVER_SPEC_MODIFIES_FLG, 
			RIVER_SPEC_MODIFIES_OP1 | RIVER_SPEC_MODIFIES_FLG, 
			RIVER_SPEC_MODIFIES_OP1 | RIVER_SPEC_MODIFIES_FLG, 
			RIVER_SPEC_MODIFIES_OP1 | RIVER_SPEC_MODIFIES_FLG, 
			RIVER_SPEC_MODIFIES_OP1 | RIVER_SPEC_MODIFIES_FLG, 
			RIVER_SPEC_MODIFIES_OP1 | RIVER_SPEC_MODIFIES_FLG, 
			RIVER_SPEC_MODIFIES_FLG 
		},
		/* for 0xFF    */{ 
			RIVER_SPEC_MODIFIES_OP1 | RIVER_SPEC_MODIFIES_FLG, 
			RIVER_SPEC_MODIFIES_OP1 | RIVER_SPEC_MODIFIES_FLG, 
			RIVER_SPEC_MODIFIES_OP3 | RIVER_SPEC_MODIFIES_OP4 | RIVER_SPEC_IGNORES_FLG | RIVER_SPEC_IGNORES_OP4,
			0,
			0, 
			0, 
			RIVER_SPEC_MODIFIES_OP2 | RIVER_SPEC_MODIFIES_OP3 | RIVER_SPEC_IGNORES_FLG | RIVER_SPEC_IGNORES_OP3,
			0 
		},
		/* for 0xF6/F7 */{ 
			RIVER_SPEC_MODIFIES_FLG, 
			RIVER_SPEC_MODIFIES_FLG, 
			RIVER_SPEC_MODIFIES_OP1 | RIVER_SPEC_MODIFIES_FLG, 
			RIVER_SPEC_MODIFIES_OP1 | RIVER_SPEC_MODIFIES_FLG, 
			RIVER_SPEC_MODIFIES_OP2 | RIVER_SPEC_MODIFIES_OP3 | RIVER_SPEC_MODIFIES_FLG,
			RIVER_SPEC_MODIFIES_OP2 | RIVER_SPEC_MODIFIES_OP3 | RIVER_SPEC_MODIFIES_FLG,
			RIVER_SPEC_MODIFIES_OP2 | RIVER_SPEC_MODIFIES_OP3 | RIVER_SPEC_MODIFIES_FLG,
			RIVER_SPEC_MODIFIES_OP2 | RIVER_SPEC_MODIFIES_OP3 | RIVER_SPEC_MODIFIES_FLG
		},
		/* for 0x0FBA  */{ 0xFF, 0xFF, 0xFF, 0xFF, RIVER_SPEC_MODIFIES_FLG, RIVER_SPEC_MODIFIES_OP1 | RIVER_SPEC_MODIFIES_FLG, RIVER_SPEC_MODIFIES_OP1 | RIVER_SPEC_MODIFIES_FLG, RIVER_SPEC_MODIFIES_OP1 | RIVER_SPEC_MODIFIES_FLG },
};
