#include "RiverX86Disassembler.h"
#include "CodeGen.h"

extern const WORD specTbl[2][0x100];
extern const WORD _specTblExt[][8];

WORD GetSpecifiers(RiverInstruction &ri) {
	//const WORD *specTbl = specTbl00;
	DWORD dwTable = (RIVER_MODIFIER_EXT & ri.modifiers) ? 1 : 0;
	WORD tmp = specTbl[dwTable][ri.opCode];

	if (tmp & RIVER_SPEC_MODIFIES_EXT) {
		tmp = _specTblExt[tmp & 0x7FFF][ri.subOpCode];
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
	rOut.opTypes[0] = rOut.opTypes[1] = RIVER_OPTYPE_NONE;

	flags = 0;
	do {
		dwTable = (rOut.modifiers & RIVER_MODIFIER_EXT) ? 1 : 0;

		flags &= ~RIVER_FLAG_PFX;
		(this->*disassembleOpcodes[dwTable][*px86])(px86, rOut, flags);
	} while (flags & RIVER_FLAG_PFX);

	dwTable = (rOut.modifiers & RIVER_MODIFIER_EXT) ? 1 : 0;
	
	(this->*disassembleOperands[dwTable][rOut.opCode])(px86, rOut);
	TrackModifiedRegisters(rOut);
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
		ri.operands[opIdx].asImm32 = *((DWORD *)px86);
		px86 += 4;
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

void RiverX86Disassembler::DisassembleImm8(BYTE *&px86, RiverInstruction &ri) {
	DisassembleImmOp(0, px86, ri, RIVER_OPSIZE_8);
}

void RiverX86Disassembler::DisassembleImm32(BYTE *&px86, RiverInstruction &ri) {
	DisassembleImmOp(0, px86, ri, RIVER_OPSIZE_32);
}





/* =========================================== */
/* Disssembly tables                           */
/* =========================================== */

RiverX86Disassembler::DisassembleOpcodeFunc RiverX86Disassembler::disassembleOpcodes[2][0x100] = {
	{
		/*0x00*/ &RiverX86Disassembler::DisassembleUnkInstr, &RiverX86Disassembler::DisassembleUnkInstr, &RiverX86Disassembler::DisassembleUnkInstr, &RiverX86Disassembler::DisassembleUnkInstr,
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

		/*0x30*/ &RiverX86Disassembler::DisassembleUnkInstr, &RiverX86Disassembler::DisassembleUnkInstr, &RiverX86Disassembler::DisassembleUnkInstr, &RiverX86Disassembler::DisassembleDefaultInstr,
		/*0x34*/ &RiverX86Disassembler::DisassembleUnkInstr, &RiverX86Disassembler::DisassembleUnkInstr, &RiverX86Disassembler::DisassembleUnkInstr, &RiverX86Disassembler::DisassembleUnkInstr,
		/*0x38*/ &RiverX86Disassembler::DisassembleUnkInstr, &RiverX86Disassembler::DisassembleUnkInstr, &RiverX86Disassembler::DisassembleUnkInstr, &RiverX86Disassembler::DisassembleDefaultInstr,
		/*0x3C*/ &RiverX86Disassembler::DisassembleUnkInstr, &RiverX86Disassembler::DisassembleUnkInstr, &RiverX86Disassembler::DisassembleUnkInstr, &RiverX86Disassembler::DisassembleUnkInstr,

		/*0x40*/ &RiverX86Disassembler::DisassemblePlusRegInstr<0x40>, &RiverX86Disassembler::DisassemblePlusRegInstr<0x40>, &RiverX86Disassembler::DisassemblePlusRegInstr<0x40>, &RiverX86Disassembler::DisassemblePlusRegInstr<0x40>,
		/*0x44*/ &RiverX86Disassembler::DisassemblePlusRegInstr<0x40>, &RiverX86Disassembler::DisassemblePlusRegInstr<0x40>, &RiverX86Disassembler::DisassemblePlusRegInstr<0x40>, &RiverX86Disassembler::DisassemblePlusRegInstr<0x40>,
		/*0x48*/ &RiverX86Disassembler::DisassemblePlusRegInstr<0x48>, &RiverX86Disassembler::DisassemblePlusRegInstr<0x48>, &RiverX86Disassembler::DisassemblePlusRegInstr<0x48>, &RiverX86Disassembler::DisassemblePlusRegInstr<0x48>,
		/*0x4C*/ &RiverX86Disassembler::DisassemblePlusRegInstr<0x48>, &RiverX86Disassembler::DisassemblePlusRegInstr<0x48>, &RiverX86Disassembler::DisassemblePlusRegInstr<0x48>, &RiverX86Disassembler::DisassemblePlusRegInstr<0x48>,

		/*0x50*/ &RiverX86Disassembler::DisassemblePlusRegInstr<0x50>, &RiverX86Disassembler::DisassemblePlusRegInstr<0x50>, &RiverX86Disassembler::DisassemblePlusRegInstr<0x50>, &RiverX86Disassembler::DisassemblePlusRegInstr<0x50>,
		/*0x54*/ &RiverX86Disassembler::DisassemblePlusRegInstr<0x50>, &RiverX86Disassembler::DisassemblePlusRegInstr<0x50>, &RiverX86Disassembler::DisassemblePlusRegInstr<0x50>, &RiverX86Disassembler::DisassemblePlusRegInstr<0x50>,
		/*0x58*/ &RiverX86Disassembler::DisassemblePlusRegInstr<0x58>, &RiverX86Disassembler::DisassemblePlusRegInstr<0x58>, &RiverX86Disassembler::DisassemblePlusRegInstr<0x58>, &RiverX86Disassembler::DisassemblePlusRegInstr<0x58>,
		/*0x5C*/ &RiverX86Disassembler::DisassemblePlusRegInstr<0x58>, &RiverX86Disassembler::DisassemblePlusRegInstr<0x58>, &RiverX86Disassembler::DisassemblePlusRegInstr<0x58>, &RiverX86Disassembler::DisassemblePlusRegInstr<0x58>,

		/*0x60*/ &RiverX86Disassembler::DisassembleUnkInstr, &RiverX86Disassembler::DisassembleUnkInstr, &RiverX86Disassembler::DisassembleUnkInstr, &RiverX86Disassembler::DisassembleUnkInstr,
		/*0x64*/ &RiverX86Disassembler::DisassembleUnkInstr, &RiverX86Disassembler::DisassembleUnkInstr, &RiverX86Disassembler::DisassembleUnkInstr, &RiverX86Disassembler::DisassembleUnkInstr,
		/*0x68*/ &RiverX86Disassembler::DisassembleUnkInstr, &RiverX86Disassembler::DisassembleUnkInstr, &RiverX86Disassembler::DisassembleUnkInstr, &RiverX86Disassembler::DisassembleUnkInstr,
		/*0x6C*/ &RiverX86Disassembler::DisassembleUnkInstr, &RiverX86Disassembler::DisassembleUnkInstr, &RiverX86Disassembler::DisassembleUnkInstr, &RiverX86Disassembler::DisassembleUnkInstr,

		/*0x70*/ &RiverX86Disassembler::DisassembleRelJmpInstr<2>, &RiverX86Disassembler::DisassembleRelJmpInstr<2>, &RiverX86Disassembler::DisassembleRelJmpInstr<2>, &RiverX86Disassembler::DisassembleRelJmpInstr<2>,
		/*0x74*/ &RiverX86Disassembler::DisassembleRelJmpInstr<2>, &RiverX86Disassembler::DisassembleRelJmpInstr<2>, &RiverX86Disassembler::DisassembleRelJmpInstr<2>, &RiverX86Disassembler::DisassembleRelJmpInstr<2>,
		/*0x78*/ &RiverX86Disassembler::DisassembleRelJmpInstr<2>, &RiverX86Disassembler::DisassembleRelJmpInstr<2>, &RiverX86Disassembler::DisassembleRelJmpInstr<2>, &RiverX86Disassembler::DisassembleRelJmpInstr<2>,
		/*0x7C*/ &RiverX86Disassembler::DisassembleRelJmpInstr<2>, &RiverX86Disassembler::DisassembleRelJmpInstr<2>, &RiverX86Disassembler::DisassembleRelJmpInstr<2>, &RiverX86Disassembler::DisassembleRelJmpInstr<2>,

		/*0x80*/ &RiverX86Disassembler::DisassembleUnkInstr, &RiverX86Disassembler::DisassembleUnkInstr, &RiverX86Disassembler::DisassembleUnkInstr, &RiverX86Disassembler::DisassembleExtInstr,
		/*0x84*/ &RiverX86Disassembler::DisassembleUnkInstr, &RiverX86Disassembler::DisassembleUnkInstr, &RiverX86Disassembler::DisassembleUnkInstr, &RiverX86Disassembler::DisassembleUnkInstr,
		/*0x88*/ &RiverX86Disassembler::DisassembleUnkInstr, &RiverX86Disassembler::DisassembleDefaultInstr, &RiverX86Disassembler::DisassembleUnkInstr, &RiverX86Disassembler::DisassembleDefaultInstr,
		/*0x8C*/ &RiverX86Disassembler::DisassembleUnkInstr, &RiverX86Disassembler::DisassembleUnkInstr, &RiverX86Disassembler::DisassembleUnkInstr, &RiverX86Disassembler::DisassembleUnkInstr,

		/*0x90*/ &RiverX86Disassembler::DisassembleUnkInstr, &RiverX86Disassembler::DisassemblePlusRegInstr<0x90>, &RiverX86Disassembler::DisassemblePlusRegInstr<0x90>, &RiverX86Disassembler::DisassemblePlusRegInstr<0x90>,
		/*0x94*/ &RiverX86Disassembler::DisassemblePlusRegInstr<0x90>, &RiverX86Disassembler::DisassemblePlusRegInstr<0x90>, &RiverX86Disassembler::DisassemblePlusRegInstr<0x90>, &RiverX86Disassembler::DisassemblePlusRegInstr<0x90>,
		/*0x98*/ &RiverX86Disassembler::DisassembleUnkInstr, &RiverX86Disassembler::DisassembleUnkInstr, &RiverX86Disassembler::DisassembleUnkInstr, &RiverX86Disassembler::DisassembleUnkInstr,
		/*0x9C*/ &RiverX86Disassembler::DisassembleUnkInstr, &RiverX86Disassembler::DisassembleUnkInstr, &RiverX86Disassembler::DisassembleUnkInstr, &RiverX86Disassembler::DisassembleUnkInstr,

		/*0xA0*/ &RiverX86Disassembler::DisassembleUnkInstr, &RiverX86Disassembler::DisassembleUnkInstr, &RiverX86Disassembler::DisassembleUnkInstr, &RiverX86Disassembler::DisassembleUnkInstr,
		/*0xA4*/ &RiverX86Disassembler::DisassembleUnkInstr, &RiverX86Disassembler::DisassembleUnkInstr, &RiverX86Disassembler::DisassembleUnkInstr, &RiverX86Disassembler::DisassembleUnkInstr,
		/*0xA8*/ &RiverX86Disassembler::DisassembleUnkInstr, &RiverX86Disassembler::DisassembleUnkInstr, &RiverX86Disassembler::DisassembleUnkInstr, &RiverX86Disassembler::DisassembleUnkInstr,
		/*0xAC*/ &RiverX86Disassembler::DisassembleUnkInstr, &RiverX86Disassembler::DisassembleUnkInstr, &RiverX86Disassembler::DisassembleUnkInstr, &RiverX86Disassembler::DisassembleUnkInstr,

		/*0xB0*/ &RiverX86Disassembler::DisassembleUnkInstr, &RiverX86Disassembler::DisassembleUnkInstr, &RiverX86Disassembler::DisassembleUnkInstr, &RiverX86Disassembler::DisassembleUnkInstr,
		/*0xB4*/ &RiverX86Disassembler::DisassembleUnkInstr, &RiverX86Disassembler::DisassembleUnkInstr, &RiverX86Disassembler::DisassembleUnkInstr, &RiverX86Disassembler::DisassembleUnkInstr,
		/*0xB8*/ &RiverX86Disassembler::DisassembleUnkInstr, &RiverX86Disassembler::DisassembleUnkInstr, &RiverX86Disassembler::DisassembleUnkInstr, &RiverX86Disassembler::DisassembleUnkInstr,
		/*0xBC*/ &RiverX86Disassembler::DisassembleUnkInstr, &RiverX86Disassembler::DisassembleUnkInstr, &RiverX86Disassembler::DisassembleUnkInstr, &RiverX86Disassembler::DisassembleUnkInstr,

		/*0xC0*/ &RiverX86Disassembler::DisassembleUnkInstr, &RiverX86Disassembler::DisassembleUnkInstr, &RiverX86Disassembler::DisassembleUnkInstr, &RiverX86Disassembler::DisassembleDefaultInstr<0, RIVER_FLAG_BRANCH>,
		/*0xC4*/ &RiverX86Disassembler::DisassembleUnkInstr, &RiverX86Disassembler::DisassembleUnkInstr, &RiverX86Disassembler::DisassembleUnkInstr, &RiverX86Disassembler::DisassembleUnkInstr,
		/*0xC8*/ &RiverX86Disassembler::DisassembleUnkInstr, &RiverX86Disassembler::DisassembleUnkInstr, &RiverX86Disassembler::DisassembleUnkInstr, &RiverX86Disassembler::DisassembleUnkInstr,
		/*0xCC*/ &RiverX86Disassembler::DisassembleUnkInstr, &RiverX86Disassembler::DisassembleUnkInstr, &RiverX86Disassembler::DisassembleUnkInstr, &RiverX86Disassembler::DisassembleUnkInstr,

		/*0xD0*/ &RiverX86Disassembler::DisassembleUnkInstr, &RiverX86Disassembler::DisassembleUnkInstr, &RiverX86Disassembler::DisassembleUnkInstr, &RiverX86Disassembler::DisassembleUnkInstr,
		/*0xD4*/ &RiverX86Disassembler::DisassembleUnkInstr, &RiverX86Disassembler::DisassembleUnkInstr, &RiverX86Disassembler::DisassembleUnkInstr, &RiverX86Disassembler::DisassembleUnkInstr,
		/*0xD8*/ &RiverX86Disassembler::DisassembleUnkInstr, &RiverX86Disassembler::DisassembleUnkInstr, &RiverX86Disassembler::DisassembleUnkInstr, &RiverX86Disassembler::DisassembleUnkInstr,
		/*0xDC*/ &RiverX86Disassembler::DisassembleUnkInstr, &RiverX86Disassembler::DisassembleUnkInstr, &RiverX86Disassembler::DisassembleUnkInstr, &RiverX86Disassembler::DisassembleUnkInstr,

		/*0xE0*/ &RiverX86Disassembler::DisassembleUnkInstr, &RiverX86Disassembler::DisassembleUnkInstr, &RiverX86Disassembler::DisassembleUnkInstr, &RiverX86Disassembler::DisassembleUnkInstr,
		/*0xE4*/ &RiverX86Disassembler::DisassembleUnkInstr, &RiverX86Disassembler::DisassembleUnkInstr, &RiverX86Disassembler::DisassembleUnkInstr, &RiverX86Disassembler::DisassembleUnkInstr,
		/*0xE8*/ &RiverX86Disassembler::DisassembleUnkInstr, &RiverX86Disassembler::DisassembleRelJmpInstr<5>, &RiverX86Disassembler::DisassembleUnkInstr, &RiverX86Disassembler::DisassembleUnkInstr,
		/*0xEC*/ &RiverX86Disassembler::DisassembleUnkInstr, &RiverX86Disassembler::DisassembleUnkInstr, &RiverX86Disassembler::DisassembleUnkInstr, &RiverX86Disassembler::DisassembleUnkInstr,

		/*0xF0*/ &RiverX86Disassembler::DisassembleUnkInstr, &RiverX86Disassembler::DisassembleUnkInstr, &RiverX86Disassembler::DisassembleUnkInstr, &RiverX86Disassembler::DisassembleUnkInstr,
		/*0xF4*/ &RiverX86Disassembler::DisassembleUnkInstr, &RiverX86Disassembler::DisassembleUnkInstr, &RiverX86Disassembler::DisassembleUnkInstr, &RiverX86Disassembler::DisassembleUnkInstr,
		/*0xF8*/ &RiverX86Disassembler::DisassembleUnkInstr, &RiverX86Disassembler::DisassembleUnkInstr, &RiverX86Disassembler::DisassembleUnkInstr, &RiverX86Disassembler::DisassembleUnkInstr,
		/*0xFC*/ &RiverX86Disassembler::DisassembleUnkInstr, &RiverX86Disassembler::DisassembleUnkInstr, &RiverX86Disassembler::DisassembleUnkInstr, &RiverX86Disassembler::DisassembleUnkInstr
	}, {
		/*0x00*/ &RiverX86Disassembler::DisassembleUnkInstr, &RiverX86Disassembler::DisassembleUnkInstr, &RiverX86Disassembler::DisassembleUnkInstr, &RiverX86Disassembler::DisassembleUnkInstr,
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

		/*0x30*/ &RiverX86Disassembler::DisassembleUnkInstr, &RiverX86Disassembler::DisassembleUnkInstr, &RiverX86Disassembler::DisassembleUnkInstr, &RiverX86Disassembler::DisassembleUnkInstr,
		/*0x34*/ &RiverX86Disassembler::DisassembleUnkInstr, &RiverX86Disassembler::DisassembleUnkInstr, &RiverX86Disassembler::DisassembleUnkInstr, &RiverX86Disassembler::DisassembleUnkInstr,
		/*0x38*/ &RiverX86Disassembler::DisassembleUnkInstr, &RiverX86Disassembler::DisassembleUnkInstr, &RiverX86Disassembler::DisassembleUnkInstr, &RiverX86Disassembler::DisassembleUnkInstr,
		/*0x3C*/ &RiverX86Disassembler::DisassembleUnkInstr, &RiverX86Disassembler::DisassembleUnkInstr, &RiverX86Disassembler::DisassembleUnkInstr, &RiverX86Disassembler::DisassembleUnkInstr,

		/*0x40*/ &RiverX86Disassembler::DisassembleUnkInstr, &RiverX86Disassembler::DisassembleUnkInstr, &RiverX86Disassembler::DisassembleUnkInstr, &RiverX86Disassembler::DisassembleUnkInstr,
		/*0x44*/ &RiverX86Disassembler::DisassembleUnkInstr, &RiverX86Disassembler::DisassembleUnkInstr, &RiverX86Disassembler::DisassembleUnkInstr, &RiverX86Disassembler::DisassembleUnkInstr,
		/*0x48*/ &RiverX86Disassembler::DisassembleUnkInstr, &RiverX86Disassembler::DisassembleUnkInstr, &RiverX86Disassembler::DisassembleUnkInstr, &RiverX86Disassembler::DisassembleUnkInstr,
		/*0x4C*/ &RiverX86Disassembler::DisassembleUnkInstr, &RiverX86Disassembler::DisassembleUnkInstr, &RiverX86Disassembler::DisassembleUnkInstr, &RiverX86Disassembler::DisassembleUnkInstr,

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

		/*0x80*/ &RiverX86Disassembler::DisassembleUnkInstr, &RiverX86Disassembler::DisassembleUnkInstr, &RiverX86Disassembler::DisassembleUnkInstr, &RiverX86Disassembler::DisassembleUnkInstr,
		/*0x84*/ &RiverX86Disassembler::DisassembleUnkInstr, &RiverX86Disassembler::DisassembleUnkInstr, &RiverX86Disassembler::DisassembleUnkInstr, &RiverX86Disassembler::DisassembleUnkInstr,
		/*0x88*/ &RiverX86Disassembler::DisassembleUnkInstr, &RiverX86Disassembler::DisassembleUnkInstr, &RiverX86Disassembler::DisassembleUnkInstr, &RiverX86Disassembler::DisassembleUnkInstr,
		/*0x8C*/ &RiverX86Disassembler::DisassembleUnkInstr, &RiverX86Disassembler::DisassembleUnkInstr, &RiverX86Disassembler::DisassembleUnkInstr, &RiverX86Disassembler::DisassembleUnkInstr,

		/*0x90*/ &RiverX86Disassembler::DisassembleUnkInstr, &RiverX86Disassembler::DisassembleUnkInstr, &RiverX86Disassembler::DisassembleUnkInstr, &RiverX86Disassembler::DisassembleUnkInstr,
		/*0x94*/ &RiverX86Disassembler::DisassembleUnkInstr, &RiverX86Disassembler::DisassembleUnkInstr, &RiverX86Disassembler::DisassembleUnkInstr, &RiverX86Disassembler::DisassembleUnkInstr,
		/*0x98*/ &RiverX86Disassembler::DisassembleUnkInstr, &RiverX86Disassembler::DisassembleUnkInstr, &RiverX86Disassembler::DisassembleUnkInstr, &RiverX86Disassembler::DisassembleUnkInstr,
		/*0x9C*/ &RiverX86Disassembler::DisassembleUnkInstr, &RiverX86Disassembler::DisassembleUnkInstr, &RiverX86Disassembler::DisassembleUnkInstr, &RiverX86Disassembler::DisassembleUnkInstr,

		/*0xA0*/ &RiverX86Disassembler::DisassembleUnkInstr, &RiverX86Disassembler::DisassembleUnkInstr, &RiverX86Disassembler::DisassembleUnkInstr, &RiverX86Disassembler::DisassembleUnkInstr,
		/*0xA4*/ &RiverX86Disassembler::DisassembleUnkInstr, &RiverX86Disassembler::DisassembleUnkInstr, &RiverX86Disassembler::DisassembleUnkInstr, &RiverX86Disassembler::DisassembleUnkInstr,
		/*0xA8*/ &RiverX86Disassembler::DisassembleUnkInstr, &RiverX86Disassembler::DisassembleUnkInstr, &RiverX86Disassembler::DisassembleUnkInstr, &RiverX86Disassembler::DisassembleUnkInstr,
		/*0xAC*/ &RiverX86Disassembler::DisassembleUnkInstr, &RiverX86Disassembler::DisassembleUnkInstr, &RiverX86Disassembler::DisassembleUnkInstr, &RiverX86Disassembler::DisassembleUnkInstr,

		/*0xB0*/ &RiverX86Disassembler::DisassembleUnkInstr, &RiverX86Disassembler::DisassembleUnkInstr, &RiverX86Disassembler::DisassembleUnkInstr, &RiverX86Disassembler::DisassembleUnkInstr,
		/*0xB4*/ &RiverX86Disassembler::DisassembleUnkInstr, &RiverX86Disassembler::DisassembleUnkInstr, &RiverX86Disassembler::DisassembleUnkInstr, &RiverX86Disassembler::DisassembleUnkInstr,
		/*0xB8*/ &RiverX86Disassembler::DisassembleUnkInstr, &RiverX86Disassembler::DisassembleUnkInstr, &RiverX86Disassembler::DisassembleUnkInstr, &RiverX86Disassembler::DisassembleUnkInstr,
		/*0xBC*/ &RiverX86Disassembler::DisassembleUnkInstr, &RiverX86Disassembler::DisassembleUnkInstr, &RiverX86Disassembler::DisassembleUnkInstr, &RiverX86Disassembler::DisassembleUnkInstr,

		/*0xC0*/ &RiverX86Disassembler::DisassembleUnkInstr, &RiverX86Disassembler::DisassembleUnkInstr, &RiverX86Disassembler::DisassembleUnkInstr, &RiverX86Disassembler::DisassembleUnkInstr,
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
		/*0x00*/ &RiverX86Disassembler::DisassembleUnkOp, &RiverX86Disassembler::DisassembleUnkOp, &RiverX86Disassembler::DisassembleUnkOp, &RiverX86Disassembler::DisassembleUnkOp,
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

		/*0x30*/ &RiverX86Disassembler::DisassembleUnkOp, &RiverX86Disassembler::DisassembleUnkOp, &RiverX86Disassembler::DisassembleUnkOp, &RiverX86Disassembler::DisassembleRegModRM,
		/*0x34*/ &RiverX86Disassembler::DisassembleUnkOp, &RiverX86Disassembler::DisassembleUnkOp, &RiverX86Disassembler::DisassembleUnkOp, &RiverX86Disassembler::DisassembleUnkOp,
		/*0x38*/ &RiverX86Disassembler::DisassembleUnkOp, &RiverX86Disassembler::DisassembleUnkOp, &RiverX86Disassembler::DisassembleUnkOp, &RiverX86Disassembler::DisassembleRegModRM,
		/*0x3C*/ &RiverX86Disassembler::DisassembleUnkOp, &RiverX86Disassembler::DisassembleUnkOp, &RiverX86Disassembler::DisassembleUnkOp, &RiverX86Disassembler::DisassembleUnkOp,

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
		/*0x68*/ &RiverX86Disassembler::DisassembleUnkOp, &RiverX86Disassembler::DisassembleUnkOp, &RiverX86Disassembler::DisassembleUnkOp, &RiverX86Disassembler::DisassembleUnkOp,
		/*0x6C*/ &RiverX86Disassembler::DisassembleUnkOp, &RiverX86Disassembler::DisassembleUnkOp, &RiverX86Disassembler::DisassembleUnkOp, &RiverX86Disassembler::DisassembleUnkOp,

		/*0x70*/ &RiverX86Disassembler::DisassembleImm8, &RiverX86Disassembler::DisassembleImm8, &RiverX86Disassembler::DisassembleImm8, &RiverX86Disassembler::DisassembleImm8,
		/*0x74*/ &RiverX86Disassembler::DisassembleImm8, &RiverX86Disassembler::DisassembleImm8, &RiverX86Disassembler::DisassembleImm8, &RiverX86Disassembler::DisassembleImm8,
		/*0x78*/ &RiverX86Disassembler::DisassembleImm8, &RiverX86Disassembler::DisassembleImm8, &RiverX86Disassembler::DisassembleImm8, &RiverX86Disassembler::DisassembleImm8,
		/*0x7C*/ &RiverX86Disassembler::DisassembleImm8, &RiverX86Disassembler::DisassembleImm8, &RiverX86Disassembler::DisassembleImm8, &RiverX86Disassembler::DisassembleImm8,

		/*0x80*/ &RiverX86Disassembler::DisassembleUnkOp, &RiverX86Disassembler::DisassembleUnkOp, &RiverX86Disassembler::DisassembleUnkOp, &RiverX86Disassembler::DisassembleModRMImm8,
		/*0x84*/ &RiverX86Disassembler::DisassembleUnkOp, &RiverX86Disassembler::DisassembleUnkOp, &RiverX86Disassembler::DisassembleUnkOp, &RiverX86Disassembler::DisassembleUnkOp,
		/*0x88*/ &RiverX86Disassembler::DisassembleUnkOp, &RiverX86Disassembler::DisassembleModRMReg, &RiverX86Disassembler::DisassembleUnkOp, &RiverX86Disassembler::DisassembleRegModRM,
		/*0x8C*/ &RiverX86Disassembler::DisassembleUnkOp, &RiverX86Disassembler::DisassembleUnkOp, &RiverX86Disassembler::DisassembleUnkOp, &RiverX86Disassembler::DisassembleUnkOp,

		/*0x90*/ &RiverX86Disassembler::DisassembleNoOp, &RiverX86Disassembler::DisassembleNoOp, &RiverX86Disassembler::DisassembleNoOp, &RiverX86Disassembler::DisassembleNoOp,
		/*0x94*/ &RiverX86Disassembler::DisassembleNoOp, &RiverX86Disassembler::DisassembleNoOp, &RiverX86Disassembler::DisassembleNoOp, &RiverX86Disassembler::DisassembleNoOp,
		/*0x98*/ &RiverX86Disassembler::DisassembleUnkOp, &RiverX86Disassembler::DisassembleUnkOp, &RiverX86Disassembler::DisassembleUnkOp, &RiverX86Disassembler::DisassembleUnkOp,
		/*0x9C*/ &RiverX86Disassembler::DisassembleUnkOp, &RiverX86Disassembler::DisassembleUnkOp, &RiverX86Disassembler::DisassembleUnkOp, &RiverX86Disassembler::DisassembleUnkOp,

		/*0xA0*/ &RiverX86Disassembler::DisassembleUnkOp, &RiverX86Disassembler::DisassembleUnkOp, &RiverX86Disassembler::DisassembleUnkOp, &RiverX86Disassembler::DisassembleUnkOp,
		/*0xA4*/ &RiverX86Disassembler::DisassembleUnkOp, &RiverX86Disassembler::DisassembleUnkOp, &RiverX86Disassembler::DisassembleUnkOp, &RiverX86Disassembler::DisassembleUnkOp,
		/*0xA8*/ &RiverX86Disassembler::DisassembleUnkOp, &RiverX86Disassembler::DisassembleUnkOp, &RiverX86Disassembler::DisassembleUnkOp, &RiverX86Disassembler::DisassembleUnkOp,
		/*0xAC*/ &RiverX86Disassembler::DisassembleUnkOp, &RiverX86Disassembler::DisassembleUnkOp, &RiverX86Disassembler::DisassembleUnkOp, &RiverX86Disassembler::DisassembleUnkOp,

		/*0xB0*/ &RiverX86Disassembler::DisassembleUnkOp, &RiverX86Disassembler::DisassembleUnkOp, &RiverX86Disassembler::DisassembleUnkOp, &RiverX86Disassembler::DisassembleUnkOp,
		/*0xB4*/ &RiverX86Disassembler::DisassembleUnkOp, &RiverX86Disassembler::DisassembleUnkOp, &RiverX86Disassembler::DisassembleUnkOp, &RiverX86Disassembler::DisassembleUnkOp,
		/*0xB8*/ &RiverX86Disassembler::DisassembleUnkOp, &RiverX86Disassembler::DisassembleUnkOp, &RiverX86Disassembler::DisassembleUnkOp, &RiverX86Disassembler::DisassembleUnkOp,
		/*0xBC*/ &RiverX86Disassembler::DisassembleUnkOp, &RiverX86Disassembler::DisassembleUnkOp, &RiverX86Disassembler::DisassembleUnkOp, &RiverX86Disassembler::DisassembleUnkOp,

		/*0xC0*/ &RiverX86Disassembler::DisassembleUnkOp, &RiverX86Disassembler::DisassembleUnkOp, &RiverX86Disassembler::DisassembleUnkOp, &RiverX86Disassembler::DisassembleNoOp,
		/*0xC4*/ &RiverX86Disassembler::DisassembleUnkOp, &RiverX86Disassembler::DisassembleUnkOp, &RiverX86Disassembler::DisassembleUnkOp, &RiverX86Disassembler::DisassembleUnkOp,
		/*0xC8*/ &RiverX86Disassembler::DisassembleUnkOp, &RiverX86Disassembler::DisassembleUnkOp, &RiverX86Disassembler::DisassembleUnkOp, &RiverX86Disassembler::DisassembleUnkOp,
		/*0xCC*/ &RiverX86Disassembler::DisassembleUnkOp, &RiverX86Disassembler::DisassembleUnkOp, &RiverX86Disassembler::DisassembleUnkOp, &RiverX86Disassembler::DisassembleUnkOp,

		/*0xD0*/ &RiverX86Disassembler::DisassembleUnkOp, &RiverX86Disassembler::DisassembleUnkOp, &RiverX86Disassembler::DisassembleUnkOp, &RiverX86Disassembler::DisassembleUnkOp,
		/*0xD4*/ &RiverX86Disassembler::DisassembleUnkOp, &RiverX86Disassembler::DisassembleUnkOp, &RiverX86Disassembler::DisassembleUnkOp, &RiverX86Disassembler::DisassembleUnkOp,
		/*0xD8*/ &RiverX86Disassembler::DisassembleUnkOp, &RiverX86Disassembler::DisassembleUnkOp, &RiverX86Disassembler::DisassembleUnkOp, &RiverX86Disassembler::DisassembleUnkOp,
		/*0xDC*/ &RiverX86Disassembler::DisassembleUnkOp, &RiverX86Disassembler::DisassembleUnkOp, &RiverX86Disassembler::DisassembleUnkOp, &RiverX86Disassembler::DisassembleUnkOp,

		/*0xE0*/ &RiverX86Disassembler::DisassembleUnkOp, &RiverX86Disassembler::DisassembleUnkOp, &RiverX86Disassembler::DisassembleUnkOp, &RiverX86Disassembler::DisassembleUnkOp,
		/*0xE4*/ &RiverX86Disassembler::DisassembleUnkOp, &RiverX86Disassembler::DisassembleUnkOp, &RiverX86Disassembler::DisassembleUnkOp, &RiverX86Disassembler::DisassembleUnkOp,
		/*0xE8*/ &RiverX86Disassembler::DisassembleUnkOp, &RiverX86Disassembler::DisassembleImm32, &RiverX86Disassembler::DisassembleUnkOp, &RiverX86Disassembler::DisassembleUnkOp,
		/*0xEC*/ &RiverX86Disassembler::DisassembleUnkOp, &RiverX86Disassembler::DisassembleUnkOp, &RiverX86Disassembler::DisassembleUnkOp, &RiverX86Disassembler::DisassembleUnkOp,

		/*0xF0*/ &RiverX86Disassembler::DisassembleUnkOp, &RiverX86Disassembler::DisassembleUnkOp, &RiverX86Disassembler::DisassembleUnkOp, &RiverX86Disassembler::DisassembleUnkOp,
		/*0xF4*/ &RiverX86Disassembler::DisassembleUnkOp, &RiverX86Disassembler::DisassembleUnkOp, &RiverX86Disassembler::DisassembleUnkOp, &RiverX86Disassembler::DisassembleUnkOp,
		/*0xF8*/ &RiverX86Disassembler::DisassembleUnkOp, &RiverX86Disassembler::DisassembleUnkOp, &RiverX86Disassembler::DisassembleUnkOp, &RiverX86Disassembler::DisassembleUnkOp,
		/*0xFC*/ &RiverX86Disassembler::DisassembleUnkOp, &RiverX86Disassembler::DisassembleUnkOp, &RiverX86Disassembler::DisassembleUnkOp, &RiverX86Disassembler::DisassembleUnkOp
	}, {
		/*0x00*/ &RiverX86Disassembler::DisassembleUnkOp, &RiverX86Disassembler::DisassembleUnkOp, &RiverX86Disassembler::DisassembleUnkOp, &RiverX86Disassembler::DisassembleUnkOp,
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

		/*0x30*/ &RiverX86Disassembler::DisassembleUnkOp, &RiverX86Disassembler::DisassembleUnkOp, &RiverX86Disassembler::DisassembleUnkOp, &RiverX86Disassembler::DisassembleUnkOp,
		/*0x34*/ &RiverX86Disassembler::DisassembleUnkOp, &RiverX86Disassembler::DisassembleUnkOp, &RiverX86Disassembler::DisassembleUnkOp, &RiverX86Disassembler::DisassembleUnkOp,
		/*0x38*/ &RiverX86Disassembler::DisassembleUnkOp, &RiverX86Disassembler::DisassembleUnkOp, &RiverX86Disassembler::DisassembleUnkOp, &RiverX86Disassembler::DisassembleUnkOp,
		/*0x3C*/ &RiverX86Disassembler::DisassembleUnkOp, &RiverX86Disassembler::DisassembleUnkOp, &RiverX86Disassembler::DisassembleUnkOp, &RiverX86Disassembler::DisassembleUnkOp,

		/*0x40*/ &RiverX86Disassembler::DisassembleUnkOp, &RiverX86Disassembler::DisassembleUnkOp, &RiverX86Disassembler::DisassembleUnkOp, &RiverX86Disassembler::DisassembleUnkOp,
		/*0x44*/ &RiverX86Disassembler::DisassembleUnkOp, &RiverX86Disassembler::DisassembleUnkOp, &RiverX86Disassembler::DisassembleUnkOp, &RiverX86Disassembler::DisassembleUnkOp,
		/*0x48*/ &RiverX86Disassembler::DisassembleUnkOp, &RiverX86Disassembler::DisassembleUnkOp, &RiverX86Disassembler::DisassembleUnkOp, &RiverX86Disassembler::DisassembleUnkOp,
		/*0x4C*/ &RiverX86Disassembler::DisassembleUnkOp, &RiverX86Disassembler::DisassembleUnkOp, &RiverX86Disassembler::DisassembleUnkOp, &RiverX86Disassembler::DisassembleUnkOp,

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

		/*0x80*/ &RiverX86Disassembler::DisassembleUnkOp, &RiverX86Disassembler::DisassembleUnkOp, &RiverX86Disassembler::DisassembleUnkOp, &RiverX86Disassembler::DisassembleUnkOp,
		/*0x84*/ &RiverX86Disassembler::DisassembleUnkOp, &RiverX86Disassembler::DisassembleUnkOp, &RiverX86Disassembler::DisassembleUnkOp, &RiverX86Disassembler::DisassembleUnkOp,
		/*0x88*/ &RiverX86Disassembler::DisassembleUnkOp, &RiverX86Disassembler::DisassembleUnkOp, &RiverX86Disassembler::DisassembleUnkOp, &RiverX86Disassembler::DisassembleUnkOp,
		/*0x8C*/ &RiverX86Disassembler::DisassembleUnkOp, &RiverX86Disassembler::DisassembleUnkOp, &RiverX86Disassembler::DisassembleUnkOp, &RiverX86Disassembler::DisassembleUnkOp,

		/*0x90*/ &RiverX86Disassembler::DisassembleUnkOp, &RiverX86Disassembler::DisassembleUnkOp, &RiverX86Disassembler::DisassembleUnkOp, &RiverX86Disassembler::DisassembleUnkOp,
		/*0x94*/ &RiverX86Disassembler::DisassembleUnkOp, &RiverX86Disassembler::DisassembleUnkOp, &RiverX86Disassembler::DisassembleUnkOp, &RiverX86Disassembler::DisassembleUnkOp,
		/*0x98*/ &RiverX86Disassembler::DisassembleUnkOp, &RiverX86Disassembler::DisassembleUnkOp, &RiverX86Disassembler::DisassembleUnkOp, &RiverX86Disassembler::DisassembleUnkOp,
		/*0x9C*/ &RiverX86Disassembler::DisassembleUnkOp, &RiverX86Disassembler::DisassembleUnkOp, &RiverX86Disassembler::DisassembleUnkOp, &RiverX86Disassembler::DisassembleUnkOp,

		/*0xA0*/ &RiverX86Disassembler::DisassembleUnkOp, &RiverX86Disassembler::DisassembleUnkOp, &RiverX86Disassembler::DisassembleUnkOp, &RiverX86Disassembler::DisassembleUnkOp,
		/*0xA4*/ &RiverX86Disassembler::DisassembleUnkOp, &RiverX86Disassembler::DisassembleUnkOp, &RiverX86Disassembler::DisassembleUnkOp, &RiverX86Disassembler::DisassembleUnkOp,
		/*0xA8*/ &RiverX86Disassembler::DisassembleUnkOp, &RiverX86Disassembler::DisassembleUnkOp, &RiverX86Disassembler::DisassembleUnkOp, &RiverX86Disassembler::DisassembleUnkOp,
		/*0xAC*/ &RiverX86Disassembler::DisassembleUnkOp, &RiverX86Disassembler::DisassembleUnkOp, &RiverX86Disassembler::DisassembleUnkOp, &RiverX86Disassembler::DisassembleUnkOp,

		/*0xB0*/ &RiverX86Disassembler::DisassembleUnkOp, &RiverX86Disassembler::DisassembleUnkOp, &RiverX86Disassembler::DisassembleUnkOp, &RiverX86Disassembler::DisassembleUnkOp,
		/*0xB4*/ &RiverX86Disassembler::DisassembleUnkOp, &RiverX86Disassembler::DisassembleUnkOp, &RiverX86Disassembler::DisassembleUnkOp, &RiverX86Disassembler::DisassembleUnkOp,
		/*0xB8*/ &RiverX86Disassembler::DisassembleUnkOp, &RiverX86Disassembler::DisassembleUnkOp, &RiverX86Disassembler::DisassembleUnkOp, &RiverX86Disassembler::DisassembleUnkOp,
		/*0xBC*/ &RiverX86Disassembler::DisassembleUnkOp, &RiverX86Disassembler::DisassembleUnkOp, &RiverX86Disassembler::DisassembleUnkOp, &RiverX86Disassembler::DisassembleUnkOp,

		/*0xC0*/ &RiverX86Disassembler::DisassembleUnkOp, &RiverX86Disassembler::DisassembleUnkOp, &RiverX86Disassembler::DisassembleUnkOp, &RiverX86Disassembler::DisassembleUnkOp,
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



/* =========================================== */
/* Specifier tables                            */
/* =========================================== */

const WORD specTbl[2][0x100] = {
	{
		/*0x00*/ 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF,
		/*0x04*/ 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF,
		/*0x08*/ 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF,
		/*0x0C*/ 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF,

		/*0x10*/ 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF,
		/*0x14*/ 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF,
		/*0x18*/ 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF,
		/*0x1C*/ 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF,

		/*0x20*/ 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF,
		/*0x24*/ 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF,
		/*0x28*/ 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF,
		/*0x2C*/ 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF,

		/*0x30*/ 0xFFFF, 0xFFFF, 0xFFFF, RIVER_SPEC_MODIFIES_OP1 | RIVER_SPEC_MODIFIES_FLG,
		/*0x34*/ 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF,
		/*0x38*/ RIVER_SPEC_MODIFIES_FLG, RIVER_SPEC_MODIFIES_FLG, RIVER_SPEC_MODIFIES_FLG, RIVER_SPEC_MODIFIES_FLG,
		/*0x3C*/ RIVER_SPEC_MODIFIES_FLG, RIVER_SPEC_MODIFIES_FLG, 0xFFFF, 0xFFFF,

		/*0x40*/ RIVER_SPEC_MODIFIES_OP1 | RIVER_SPEC_MODIFIES_FLG, RIVER_SPEC_MODIFIES_OP1 | RIVER_SPEC_MODIFIES_FLG, RIVER_SPEC_MODIFIES_OP1 | RIVER_SPEC_MODIFIES_FLG, RIVER_SPEC_MODIFIES_OP1 | RIVER_SPEC_MODIFIES_FLG,
		/*0x44*/ RIVER_SPEC_MODIFIES_OP1 | RIVER_SPEC_MODIFIES_FLG, RIVER_SPEC_MODIFIES_OP1 | RIVER_SPEC_MODIFIES_FLG, RIVER_SPEC_MODIFIES_OP1 | RIVER_SPEC_MODIFIES_FLG, RIVER_SPEC_MODIFIES_OP1 | RIVER_SPEC_MODIFIES_FLG,
		/*0x48*/ RIVER_SPEC_MODIFIES_OP1 | RIVER_SPEC_MODIFIES_FLG, RIVER_SPEC_MODIFIES_OP1 | RIVER_SPEC_MODIFIES_FLG, RIVER_SPEC_MODIFIES_OP1 | RIVER_SPEC_MODIFIES_FLG, RIVER_SPEC_MODIFIES_OP1 | RIVER_SPEC_MODIFIES_FLG,
		/*0x4C*/ RIVER_SPEC_MODIFIES_OP1 | RIVER_SPEC_MODIFIES_FLG, RIVER_SPEC_MODIFIES_OP1 | RIVER_SPEC_MODIFIES_FLG, RIVER_SPEC_MODIFIES_OP1 | RIVER_SPEC_MODIFIES_FLG, RIVER_SPEC_MODIFIES_OP1 | RIVER_SPEC_MODIFIES_FLG,

		/*0x50*/ RIVER_SPEC_MODIFIES_xSP, RIVER_SPEC_MODIFIES_xSP, RIVER_SPEC_MODIFIES_xSP, RIVER_SPEC_MODIFIES_xSP,
		/*0x54*/ RIVER_SPEC_MODIFIES_xSP, RIVER_SPEC_MODIFIES_xSP, RIVER_SPEC_MODIFIES_xSP, RIVER_SPEC_MODIFIES_xSP,
		/*0x58*/ RIVER_SPEC_MODIFIES_OP1 | RIVER_SPEC_MODIFIES_xSP, RIVER_SPEC_MODIFIES_OP1 | RIVER_SPEC_MODIFIES_xSP, RIVER_SPEC_MODIFIES_OP1 | RIVER_SPEC_MODIFIES_xSP, RIVER_SPEC_MODIFIES_OP1 | RIVER_SPEC_MODIFIES_xSP,
		/*0x5C*/ RIVER_SPEC_MODIFIES_OP1 | RIVER_SPEC_MODIFIES_xSP, RIVER_SPEC_MODIFIES_OP1 | RIVER_SPEC_MODIFIES_xSP, RIVER_SPEC_MODIFIES_OP1 | RIVER_SPEC_MODIFIES_xSP, RIVER_SPEC_MODIFIES_OP1 | RIVER_SPEC_MODIFIES_xSP,

		/*0x60*/ 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF,
		/*0x64*/ 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF,
		/*0x68*/ 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF,
		/*0x6C*/ 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF,

		/*0x70*/ 0, 0, 0, 0,
		/*0x74*/ 0, 0, 0, 0,
		/*0x78*/ 0, 0, 0, 0,
		/*0x7C*/ 0, 0, 0, 0,

		/*0x80*/ RIVER_SPEC_MODIFIES_EXT, RIVER_SPEC_MODIFIES_EXT, RIVER_SPEC_MODIFIES_EXT, RIVER_SPEC_MODIFIES_EXT,
		/*0x84*/ 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF,
		/*0x88*/ 0xFFFF, RIVER_SPEC_MODIFIES_OP1, 0xFFFF, RIVER_SPEC_MODIFIES_OP1,
		/*0x8C*/ 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF,

		/*0x90*/ 0, 0, 0, 0,
		/*0x94*/ 0, 0, 0, 0,
		/*0x98*/ 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF,
		/*0x9C*/ 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF,

		/*0xA0*/ 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF,
		/*0xA4*/ 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF,
		/*0xA8*/ 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF,
		/*0xAC*/ 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF,

		/*0xB0*/ 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF,
		/*0xB4*/ 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF,
		/*0xB8*/ 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF,
		/*0xBC*/ 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF,

		/*0xC0*/ 0xFFFF, 0xFFFF, 0xFFFF, RIVER_SPEC_MODIFIES_xSP,
		/*0xC4*/ 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF,
		/*0xC8*/ 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF,
		/*0xCC*/ 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF,

		/*0xD0*/ 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF,
		/*0xD4*/ 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF,
		/*0xD8*/ 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF,
		/*0xDC*/ 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF,

		/*0xE0*/ 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF,
		/*0xE4*/ 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF,
		/*0xE8*/ 0xFFFF, 0, 0, 0,
		/*0xEC*/ 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF,

		/*0xF0*/ 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF,
		/*0xF4*/ 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF,
		/*0xF8*/ 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF,
		/*0xFC*/ 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF
	}, {
		/*0x00*/ 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF,
		/*0x04*/ 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF,
		/*0x08*/ 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF,
		/*0x0C*/ 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF,

		/*0x10*/ 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF,
		/*0x14*/ 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF,
		/*0x18*/ 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF,
		/*0x1C*/ 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF,

		/*0x20*/ 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF,
		/*0x24*/ 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF,
		/*0x28*/ 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF,
		/*0x2C*/ 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF,

		/*0x30*/ 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF,
		/*0x34*/ 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF,
		/*0x38*/ 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF,
		/*0x3C*/ 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF,

		/*0x40*/ 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF,
		/*0x44*/ 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF,
		/*0x48*/ 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF,
		/*0x4C*/ 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF,

		/*0x50*/ 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF,
		/*0x54*/ 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF,
		/*0x58*/ 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF,
		/*0x5C*/ 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF,

		/*0x60*/ 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF,
		/*0x64*/ 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF,
		/*0x68*/ 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF,
		/*0x6C*/ 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF,

		/*0x70*/ 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF,
		/*0x74*/ 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF,
		/*0x78*/ 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF,
		/*0x7C*/ 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF,

		/*0x80*/ 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF,
		/*0x84*/ 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF,
		/*0x88*/ 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF,
		/*0x8C*/ 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF,

		/*0x90*/ 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF,
		/*0x94*/ 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF,
		/*0x98*/ 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF,
		/*0x9C*/ 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF,

		/*0xA0*/ 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF,
		/*0xA4*/ 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF,
		/*0xA8*/ 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF,
		/*0xAC*/ 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF,

		/*0xB0*/ 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF,
		/*0xB4*/ 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF,
		/*0xB8*/ 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF,
		/*0xBC*/ 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF,

		/*0xC0*/ 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF,
		/*0xC4*/ 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF,
		/*0xC8*/ 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF,
		/*0xCC*/ 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF,

		/*0xD0*/ 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF,
		/*0xD4*/ 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF,
		/*0xD8*/ 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF,
		/*0xDC*/ 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF,

		/*0xE0*/ 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF,
		/*0xE4*/ 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF,
		/*0xE8*/ 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF,
		/*0xEC*/ 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF,

		/*0xF0*/ 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF,
		/*0xF4*/ 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF,
		/*0xF8*/ 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF,
		/*0xFC*/ 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF
	}
};

const WORD _specTblExt[][8] = {
	{ RIVER_SPEC_MODIFIES_OP1, RIVER_SPEC_MODIFIES_OP1, RIVER_SPEC_MODIFIES_OP1, RIVER_SPEC_MODIFIES_OP1, RIVER_SPEC_MODIFIES_OP1, RIVER_SPEC_MODIFIES_OP1, RIVER_SPEC_MODIFIES_OP1, 0 }
};
