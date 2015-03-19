#include "riverinternl.h"
#include "modrm32.h"

extern const WORD specTbl00[];
extern const WORD specTbl0F[];
extern const WORD specTblExt[][8];

extern const TranslateOpcodeFunc TranslateOpcodeTable00[];
extern const TranslateOpcodeFunc TranslateOpcodeTable0F[];

extern const TranslateOperandsFunc TranslateOperandsTable00[];
extern const TranslateOperandsFunc TranslateOperandsTable0F[];

void TrackModifiedRegisters(RiverCodeGen *cg, struct RiverInstruction *ri) {
	//bool modifiysp = true;
	if (RIVER_SPEC_MODIFIES_xSP & ri->specifiers) {
		cg->NextReg(RIVER_REG_xSP);
		//modifiysp = false;
	}

	if ((RIVER_SPEC_MODIFIES_OP2 & ri->specifiers) && (RIVER_OPTYPE_REG & ri->opTypes[1])) {
		ri->operands[1].asRegister.versioned = cg->NextReg(ri->operands[1].asRegister.name);
	}

	if ((RIVER_SPEC_MODIFIES_OP1 & ri->specifiers) && (RIVER_OPTYPE_REG & ri->opTypes[0])) {
		ri->operands[0].asRegister.versioned = cg->NextReg(ri->operands[0].asRegister.name);
	}
}

void InitRiverInstruction(RiverCodeGen *cg, struct RiverInstruction *ri, BYTE **px86, DWORD *pFlags) {
	ri->modifiers = 0;
	ri->specifiers = 0;
	ri->subOpCode = 0;
	ri->opTypes[0] = ri->opTypes[1] = RIVER_OPTYPE_NONE;

	*pFlags = 0;
	do {
		const TranslateOpcodeFunc *translateOpcodeTable = TranslateOpcodeTable00;

		if (RIVER_MODIFIER_EXT & ri->modifiers) {
			translateOpcodeTable = TranslateOpcodeTable0F;
		}

		*pFlags &= ~RIVER_FLAG_PFX;
		translateOpcodeTable[**px86](cg, ri, px86, pFlags);
	} while (*pFlags & RIVER_FLAG_PFX);

	const TranslateOperandsFunc *translateOperandsTable = TranslateOperandsTable00;
	if (ri->modifiers & RIVER_MODIFIER_EXT) {
		translateOperandsTable = TranslateOperandsTable0F;
	}

	translateOperandsTable[ri->opCode](cg, ri, px86);
	TrackModifiedRegisters(cg, ri);
}

/* helper functions */
WORD GetSpecifiers(RiverInstruction *ri) {
	const WORD *specTbl = specTbl00;

	if (RIVER_MODIFIER_EXT & ri->modifiers) {
		specTbl = specTbl0F;
	}

	WORD tmp = specTbl[ri->opCode];

	if (tmp & RIVER_SPEC_MODIFIES_EXT) {
		tmp = specTblExt[tmp & 0x7FFF][ri->subOpCode];
	}

	return tmp;
}

void TranslateImmOperand(BYTE opIdx, RiverInstruction *ri, BYTE **px86, BYTE immSize) {
	ri->opTypes[opIdx] = RIVER_OPTYPE_IMM | immSize;
	switch (immSize) {
		case RIVER_OPSIZE_8 :
			ri->operands[opIdx].asImm8 = *((BYTE *)(*px86));;
			(*px86)++;
			break;
		case RIVER_OPSIZE_16:
			ri->operands[opIdx].asImm16 = *((WORD *)(*px86));;
			(*px86) += 2;
			break;
		case RIVER_OPSIZE_32:
			ri->operands[opIdx].asImm32 = *((DWORD *)(*px86));;
			(*px86) += 4;
			break;
	}
}

void TranslateRegisterOperand(RiverCodeGen *cg, BYTE opIdx, RiverInstruction *ri, BYTE reg) {
	BYTE opType = RIVER_OPTYPE_REG;
	if (RIVER_MODIFIER_O8 & ri->modifiers) {
		opType |= RIVER_OPSIZE_8;
	}
	else if (RIVER_MODIFIER_O16 & ri->modifiers) {
		opType |= RIVER_OPSIZE_16;
	}

	ri->opTypes[opIdx] = opType;
	ri->operands[opIdx].asRegister.versioned = cg->GetCurrentReg(reg);
}

void TranslateSIBByte(RiverCodeGen *cg, BYTE opIdx, RiverInstruction *ri, BYTE **px86) {
	RiverAddress *rAddr = ri->operands[opIdx].asAddress;

	rAddr->sib = **px86;

	BYTE scale = (**px86) >> 6;
	BYTE index = ((**px86) >> 3) & 0x07;
	BYTE base = (**px86) & 0x07;

	static const BYTE scales[] = { 1, 2, 4, 8 };

	rAddr->type |= RIVER_ADDR_SCALE;
	rAddr->scale = scales[scale];

	rAddr->index.versioned = cg->GetCurrentReg(index);
	rAddr->base.versioned = cg->GetCurrentReg(base);

	(*px86)++;
}

void TranslateModRMOperand(RiverCodeGen *cg, BYTE opIdx, RiverInstruction *ri, BYTE **px86) {
	RiverAddress *rAddr;
	BYTE mod = (**px86) >> 6;
	BYTE rm = **px86 & 0x07;

	if (0x03 == mod) {
		TranslateRegisterOperand(cg, opIdx, ri, rm);
		(*px86)++;
		return;
	}
	
	rAddr = cg->AllocAddr(); //new RiverAddress;
	rAddr->type = 0;
	rAddr->modRM = **px86;
	
	BYTE opType = RIVER_OPTYPE_MEM;
	if (RIVER_MODIFIER_O8 & ri->modifiers) {
		opType |= RIVER_OPSIZE_8;
	}
	else if (RIVER_MODIFIER_O16 & ri->modifiers) {
		opType |= RIVER_OPSIZE_16;
	}

	ri->opTypes[opIdx] = opType;
	ri->operands[opIdx].asAddress = rAddr;

	switch (mod) {
		case 0: 
			if (0x05 == rm) {
				rAddr->type |= RIVER_ADDR_DISP32;
				rAddr->base.versioned = RIVER_REG_NONE;
			} else {
				rAddr->type |= RIVER_ADDR_BASE;
			}
			break;
		case 1: 
			rAddr->type |= RIVER_ADDR_BASE | RIVER_ADDR_DISP8;
			break;
		case 2: 
			rAddr->type |= RIVER_ADDR_BASE | RIVER_ADDR_DISP32;
			break;
		case 3:	 break;
	}

	(*px86)++;
	if (0x04 == rm) {
		TranslateSIBByte(cg, opIdx, ri, px86); 
	} else {
		rAddr->base.versioned = cg->GetCurrentReg(rm);
	}

	switch (rAddr->type & (RIVER_ADDR_DISP8 | RIVER_ADDR_DISP16 | RIVER_ADDR_DISP32)) {
		case RIVER_ADDR_DISP8 :
			rAddr->disp.d8 = *((BYTE *)(*px86));
			(*px86)++;
			break;
		case RIVER_ADDR_DISP16 :
			rAddr->disp.d16 = *((WORD *)(*px86));
			(*px86) += 2;
			break;
		case RIVER_ADDR_DISP32 :
			rAddr->disp.d32 = *((DWORD *)(*px86));
			(*px86) += 4;
			break;
		default :
			__asm int 3;
	}
}

/* classic opcode decoders */

static void TranslateUnknownInstr(RiverCodeGen *cg, RiverInstruction *ri, BYTE **px86, DWORD *pFlags) {
	__asm int 3;
	(*px86)++;
}

// handles +r opcodes such as 0x50+r
// base template argument corresponds to op eax
template <BYTE base> void TranslatePlusRegInstr(RiverCodeGen *cg, RiverInstruction *ri, BYTE **px86, DWORD *pFlags) {
	ri->opCode = base;
	ri->specifiers = GetSpecifiers(ri);
	TranslateRegisterOperand(cg, 0, ri, **px86 - base);
	(*px86)++;
}

// translate single opcode instruction
template <int modifiers = 0, int flags = 0> void TranslateDefaultInstr(RiverCodeGen *cg, RiverInstruction *ri, BYTE **px86, DWORD *pFlags) {
	ri->opCode = **px86;
	ri->modifiers |= modifiers;
	ri->specifiers = GetSpecifiers(ri);
	*pFlags |= flags;
	(*px86)++;
}

static void TranslateExtendedInstr(RiverCodeGen *cg, RiverInstruction *ri, BYTE **px86, DWORD *pFlags) {
	ri->opCode = **px86;
	(*px86)++; 
	
	ri->subOpCode = ((**px86) >> 3) & 0x07;
	ri->specifiers = GetSpecifiers(ri);
}

template <BYTE instrLen> void TranslateRelBranchInstr(RiverCodeGen *cg, RiverInstruction *ri, BYTE **px86, DWORD *pFlags) {
	ri->opCode = **px86;
	ri->specifiers = GetSpecifiers(ri);
	ri->opTypes[1] = RIVER_OPTYPE_IMM | RIVER_OPSIZE_32;
	ri->operands[1].asImm32 = (DWORD)(*px86) + instrLen;
	*pFlags |= RIVER_FLAG_BRANCH;
	(*px86)++;
}

/* operand decoders */
static void TranslateUnknownOp(RiverCodeGen *cg, RiverInstruction *ri, BYTE **px86) {
	__asm int 3;
}

static void TranslateNoOp(RiverCodeGen *cg, RiverInstruction *ri, BYTE **px86) {
}

static void TranslateRegModRM(RiverCodeGen *cg, RiverInstruction *ri, BYTE **px86) {
	//BYTE mod = (**px86) >> 6;
	BYTE sec = ((**px86) >> 3) & 0x07;
	//BYTE rm = **px86 & 0x07;

	TranslateRegisterOperand(cg, 0, ri, sec);
	TranslateModRMOperand(cg, 1, ri, px86);
}

static void TranslateModRMReg(RiverCodeGen *cg, RiverInstruction *ri, BYTE **px86) {
	//BYTE mod = (**px86) >> 6;
	BYTE sec = ((**px86) >> 3) & 0x07;
	//BYTE rm = **px86 & 0x07;

	TranslateRegisterOperand(cg, 1, ri, sec);
	TranslateModRMOperand(cg, 0, ri, px86);
}

static void TranslateModRMImm8(RiverCodeGen *cg, RiverInstruction *ri, BYTE **px86) {
	TranslateModRMOperand(cg, 0, ri, px86);
	TranslateImmOperand(1, ri, px86, RIVER_OPSIZE_8);
}

static void TranslateImm8(RiverCodeGen *cg, RiverInstruction *ri, BYTE **px86) {
	TranslateImmOperand(0, ri, px86, RIVER_OPSIZE_8);
}

static void TranslateImm32(RiverCodeGen *cg, RiverInstruction *ri, BYTE **px86) {
	TranslateImmOperand(0, ri, px86, RIVER_OPSIZE_32);
}


/* Specifier tables */

const WORD specTbl00[] = {
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
};

const WORD specTbl0F[] = {
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
};

const WORD specTblExt[][8] = {
	{ RIVER_SPEC_MODIFIES_OP1, RIVER_SPEC_MODIFIES_OP1, RIVER_SPEC_MODIFIES_OP1, RIVER_SPEC_MODIFIES_OP1, RIVER_SPEC_MODIFIES_OP1, RIVER_SPEC_MODIFIES_OP1, RIVER_SPEC_MODIFIES_OP1, 0 }
};

/* Opcode translation */

const TranslateOpcodeFunc TranslateOpcodeTable00[] = {
	/*0x00*/ TranslateUnknownInstr, TranslateUnknownInstr, TranslateUnknownInstr, TranslateUnknownInstr,
	/*0x04*/ TranslateUnknownInstr, TranslateUnknownInstr, TranslateUnknownInstr, TranslateUnknownInstr,
	/*0x08*/ TranslateUnknownInstr, TranslateUnknownInstr, TranslateUnknownInstr, TranslateUnknownInstr,
	/*0x0C*/ TranslateUnknownInstr, TranslateUnknownInstr, TranslateUnknownInstr, TranslateUnknownInstr,

	/*0x10*/ TranslateUnknownInstr, TranslateUnknownInstr, TranslateUnknownInstr, TranslateUnknownInstr,
	/*0x14*/ TranslateUnknownInstr, TranslateUnknownInstr, TranslateUnknownInstr, TranslateUnknownInstr,
	/*0x18*/ TranslateUnknownInstr, TranslateUnknownInstr, TranslateUnknownInstr, TranslateUnknownInstr,
	/*0x1C*/ TranslateUnknownInstr, TranslateUnknownInstr, TranslateUnknownInstr, TranslateUnknownInstr,

	/*0x20*/ TranslateUnknownInstr, TranslateUnknownInstr, TranslateUnknownInstr, TranslateUnknownInstr,
	/*0x24*/ TranslateUnknownInstr, TranslateUnknownInstr, TranslateUnknownInstr, TranslateUnknownInstr,
	/*0x28*/ TranslateUnknownInstr, TranslateUnknownInstr, TranslateUnknownInstr, TranslateUnknownInstr,
	/*0x2C*/ TranslateUnknownInstr, TranslateUnknownInstr, TranslateUnknownInstr, TranslateUnknownInstr,

	/*0x30*/ TranslateUnknownInstr, TranslateUnknownInstr, TranslateUnknownInstr, TranslateDefaultInstr,
	/*0x34*/ TranslateUnknownInstr, TranslateUnknownInstr, TranslateUnknownInstr, TranslateUnknownInstr,
	/*0x38*/ TranslateUnknownInstr, TranslateUnknownInstr, TranslateUnknownInstr, TranslateDefaultInstr,
	/*0x3C*/ TranslateUnknownInstr, TranslateUnknownInstr, TranslateUnknownInstr, TranslateUnknownInstr,

	/*0x40*/ TranslatePlusRegInstr<0x40>, TranslatePlusRegInstr<0x40>, TranslatePlusRegInstr<0x40>, TranslatePlusRegInstr<0x40>,
	/*0x44*/ TranslatePlusRegInstr<0x40>, TranslatePlusRegInstr<0x40>, TranslatePlusRegInstr<0x40>, TranslatePlusRegInstr<0x40>,
	/*0x48*/ TranslatePlusRegInstr<0x48>, TranslatePlusRegInstr<0x48>, TranslatePlusRegInstr<0x48>, TranslatePlusRegInstr<0x48>,
	/*0x4C*/ TranslatePlusRegInstr<0x48>, TranslatePlusRegInstr<0x48>, TranslatePlusRegInstr<0x48>, TranslatePlusRegInstr<0x48>,

	/*0x50*/ TranslatePlusRegInstr<0x50>, TranslatePlusRegInstr<0x50>, TranslatePlusRegInstr<0x50>, TranslatePlusRegInstr<0x50>,
	/*0x54*/ TranslatePlusRegInstr<0x50>, TranslatePlusRegInstr<0x50>, TranslatePlusRegInstr<0x50>, TranslatePlusRegInstr<0x50>,
	/*0x58*/ TranslatePlusRegInstr<0x58>, TranslatePlusRegInstr<0x58>, TranslatePlusRegInstr<0x58>, TranslatePlusRegInstr<0x58>,
	/*0x5C*/ TranslatePlusRegInstr<0x58>, TranslatePlusRegInstr<0x58>, TranslatePlusRegInstr<0x58>, TranslatePlusRegInstr<0x58>,

	/*0x60*/ TranslateUnknownInstr, TranslateUnknownInstr, TranslateUnknownInstr, TranslateUnknownInstr,
	/*0x64*/ TranslateUnknownInstr, TranslateUnknownInstr, TranslateUnknownInstr, TranslateUnknownInstr,
	/*0x68*/ TranslateUnknownInstr, TranslateUnknownInstr, TranslateUnknownInstr, TranslateUnknownInstr,
	/*0x6C*/ TranslateUnknownInstr, TranslateUnknownInstr, TranslateUnknownInstr, TranslateUnknownInstr,

	/*0x70*/ TranslateRelBranchInstr<2>, TranslateRelBranchInstr<2>, TranslateRelBranchInstr<2>, TranslateRelBranchInstr<2>,
	/*0x74*/ TranslateRelBranchInstr<2>, TranslateRelBranchInstr<2>, TranslateRelBranchInstr<2>, TranslateRelBranchInstr<2>,
	/*0x78*/ TranslateRelBranchInstr<2>, TranslateRelBranchInstr<2>, TranslateRelBranchInstr<2>, TranslateRelBranchInstr<2>,
	/*0x7C*/ TranslateRelBranchInstr<2>, TranslateRelBranchInstr<2>, TranslateRelBranchInstr<2>, TranslateRelBranchInstr<2>,
	
	/*0x80*/ TranslateUnknownInstr, TranslateUnknownInstr, TranslateUnknownInstr, TranslateExtendedInstr,
	/*0x84*/ TranslateUnknownInstr, TranslateUnknownInstr, TranslateUnknownInstr, TranslateUnknownInstr,
	/*0x88*/ TranslateUnknownInstr, TranslateDefaultInstr, TranslateUnknownInstr, TranslateDefaultInstr,
	/*0x8C*/ TranslateUnknownInstr, TranslateUnknownInstr, TranslateUnknownInstr, TranslateUnknownInstr,

	/*0x90*/ TranslateUnknownInstr, TranslatePlusRegInstr<0x90>, TranslatePlusRegInstr<0x90>, TranslatePlusRegInstr<0x90>,
	/*0x94*/ TranslatePlusRegInstr<0x90>, TranslatePlusRegInstr<0x90>, TranslatePlusRegInstr<0x90>, TranslatePlusRegInstr<0x90>,
	/*0x98*/ TranslateUnknownInstr, TranslateUnknownInstr, TranslateUnknownInstr, TranslateUnknownInstr,
	/*0x9C*/ TranslateUnknownInstr, TranslateUnknownInstr, TranslateUnknownInstr, TranslateUnknownInstr,

	/*0xA0*/ TranslateUnknownInstr, TranslateUnknownInstr, TranslateUnknownInstr, TranslateUnknownInstr,
	/*0xA4*/ TranslateUnknownInstr, TranslateUnknownInstr, TranslateUnknownInstr, TranslateUnknownInstr,
	/*0xA8*/ TranslateUnknownInstr, TranslateUnknownInstr, TranslateUnknownInstr, TranslateUnknownInstr,
	/*0xAC*/ TranslateUnknownInstr, TranslateUnknownInstr, TranslateUnknownInstr, TranslateUnknownInstr,

	/*0xB0*/ TranslateUnknownInstr, TranslateUnknownInstr, TranslateUnknownInstr, TranslateUnknownInstr,
	/*0xB4*/ TranslateUnknownInstr, TranslateUnknownInstr, TranslateUnknownInstr, TranslateUnknownInstr,
	/*0xB8*/ TranslateUnknownInstr, TranslateUnknownInstr, TranslateUnknownInstr, TranslateUnknownInstr,
	/*0xBC*/ TranslateUnknownInstr, TranslateUnknownInstr, TranslateUnknownInstr, TranslateUnknownInstr,

	/*0xC0*/ TranslateUnknownInstr, TranslateUnknownInstr, TranslateUnknownInstr, TranslateDefaultInstr<0, RIVER_FLAG_BRANCH>,
	/*0xC4*/ TranslateUnknownInstr, TranslateUnknownInstr, TranslateUnknownInstr, TranslateUnknownInstr,
	/*0xC8*/ TranslateUnknownInstr, TranslateUnknownInstr, TranslateUnknownInstr, TranslateUnknownInstr,
	/*0xCC*/ TranslateUnknownInstr, TranslateUnknownInstr, TranslateUnknownInstr, TranslateUnknownInstr,

	/*0xD0*/ TranslateUnknownInstr, TranslateUnknownInstr, TranslateUnknownInstr, TranslateUnknownInstr,
	/*0xD4*/ TranslateUnknownInstr, TranslateUnknownInstr, TranslateUnknownInstr, TranslateUnknownInstr,
	/*0xD8*/ TranslateUnknownInstr, TranslateUnknownInstr, TranslateUnknownInstr, TranslateUnknownInstr,
	/*0xDC*/ TranslateUnknownInstr, TranslateUnknownInstr, TranslateUnknownInstr, TranslateUnknownInstr,

	/*0xE0*/ TranslateUnknownInstr, TranslateUnknownInstr, TranslateUnknownInstr, TranslateUnknownInstr,
	/*0xE4*/ TranslateUnknownInstr, TranslateUnknownInstr, TranslateUnknownInstr, TranslateUnknownInstr,
	/*0xE8*/ TranslateUnknownInstr, TranslateRelBranchInstr<5>, TranslateUnknownInstr, TranslateUnknownInstr,
	/*0xEC*/ TranslateUnknownInstr, TranslateUnknownInstr, TranslateUnknownInstr, TranslateUnknownInstr,

	/*0xF0*/ TranslateUnknownInstr, TranslateUnknownInstr, TranslateUnknownInstr, TranslateUnknownInstr,
	/*0xF4*/ TranslateUnknownInstr, TranslateUnknownInstr, TranslateUnknownInstr, TranslateUnknownInstr,
	/*0xF8*/ TranslateUnknownInstr, TranslateUnknownInstr, TranslateUnknownInstr, TranslateUnknownInstr,
	/*0xFC*/ TranslateUnknownInstr, TranslateUnknownInstr, TranslateUnknownInstr, TranslateUnknownInstr
};

const TranslateOpcodeFunc TranslateOpcodeTable0F[] = {
	/*0x00*/ TranslateUnknownInstr, TranslateUnknownInstr, TranslateUnknownInstr, TranslateUnknownInstr,
	/*0x04*/ TranslateUnknownInstr, TranslateUnknownInstr, TranslateUnknownInstr, TranslateUnknownInstr,
	/*0x08*/ TranslateUnknownInstr, TranslateUnknownInstr, TranslateUnknownInstr, TranslateUnknownInstr,
	/*0x0C*/ TranslateUnknownInstr, TranslateUnknownInstr, TranslateUnknownInstr, TranslateUnknownInstr,

	/*0x10*/ TranslateUnknownInstr, TranslateUnknownInstr, TranslateUnknownInstr, TranslateUnknownInstr,
	/*0x14*/ TranslateUnknownInstr, TranslateUnknownInstr, TranslateUnknownInstr, TranslateUnknownInstr,
	/*0x18*/ TranslateUnknownInstr, TranslateUnknownInstr, TranslateUnknownInstr, TranslateUnknownInstr,
	/*0x1C*/ TranslateUnknownInstr, TranslateUnknownInstr, TranslateUnknownInstr, TranslateUnknownInstr,

	/*0x20*/ TranslateUnknownInstr, TranslateUnknownInstr, TranslateUnknownInstr, TranslateUnknownInstr,
	/*0x24*/ TranslateUnknownInstr, TranslateUnknownInstr, TranslateUnknownInstr, TranslateUnknownInstr,
	/*0x28*/ TranslateUnknownInstr, TranslateUnknownInstr, TranslateUnknownInstr, TranslateUnknownInstr,
	/*0x2C*/ TranslateUnknownInstr, TranslateUnknownInstr, TranslateUnknownInstr, TranslateUnknownInstr,

	/*0x30*/ TranslateUnknownInstr, TranslateUnknownInstr, TranslateUnknownInstr, TranslateUnknownInstr,
	/*0x34*/ TranslateUnknownInstr, TranslateUnknownInstr, TranslateUnknownInstr, TranslateUnknownInstr,
	/*0x38*/ TranslateUnknownInstr, TranslateUnknownInstr, TranslateUnknownInstr, TranslateUnknownInstr,
	/*0x3C*/ TranslateUnknownInstr, TranslateUnknownInstr, TranslateUnknownInstr, TranslateUnknownInstr,

	/*0x40*/ TranslateUnknownInstr, TranslateUnknownInstr, TranslateUnknownInstr, TranslateUnknownInstr,
	/*0x44*/ TranslateUnknownInstr, TranslateUnknownInstr, TranslateUnknownInstr, TranslateUnknownInstr,
	/*0x48*/ TranslateUnknownInstr, TranslateUnknownInstr, TranslateUnknownInstr, TranslateUnknownInstr,
	/*0x4C*/ TranslateUnknownInstr, TranslateUnknownInstr, TranslateUnknownInstr, TranslateUnknownInstr,

	/*0x50*/ TranslateUnknownInstr, TranslateUnknownInstr, TranslateUnknownInstr, TranslateUnknownInstr,
	/*0x54*/ TranslateUnknownInstr, TranslateUnknownInstr, TranslateUnknownInstr, TranslateUnknownInstr,
	/*0x58*/ TranslateUnknownInstr, TranslateUnknownInstr, TranslateUnknownInstr, TranslateUnknownInstr,
	/*0x5C*/ TranslateUnknownInstr, TranslateUnknownInstr, TranslateUnknownInstr, TranslateUnknownInstr,

	/*0x60*/ TranslateUnknownInstr, TranslateUnknownInstr, TranslateUnknownInstr, TranslateUnknownInstr,
	/*0x64*/ TranslateUnknownInstr, TranslateUnknownInstr, TranslateUnknownInstr, TranslateUnknownInstr,
	/*0x68*/ TranslateUnknownInstr, TranslateUnknownInstr, TranslateUnknownInstr, TranslateUnknownInstr,
	/*0x6C*/ TranslateUnknownInstr, TranslateUnknownInstr, TranslateUnknownInstr, TranslateUnknownInstr,

	/*0x70*/ TranslateUnknownInstr, TranslateUnknownInstr, TranslateUnknownInstr, TranslateUnknownInstr,
	/*0x74*/ TranslateUnknownInstr, TranslateUnknownInstr, TranslateUnknownInstr, TranslateUnknownInstr,
	/*0x78*/ TranslateUnknownInstr, TranslateUnknownInstr, TranslateUnknownInstr, TranslateUnknownInstr,
	/*0x7C*/ TranslateUnknownInstr, TranslateUnknownInstr, TranslateUnknownInstr, TranslateUnknownInstr,

	/*0x80*/ TranslateUnknownInstr, TranslateUnknownInstr, TranslateUnknownInstr, TranslateUnknownInstr,
	/*0x84*/ TranslateUnknownInstr, TranslateUnknownInstr, TranslateUnknownInstr, TranslateUnknownInstr,
	/*0x88*/ TranslateUnknownInstr, TranslateUnknownInstr, TranslateUnknownInstr, TranslateUnknownInstr,
	/*0x8C*/ TranslateUnknownInstr, TranslateUnknownInstr, TranslateUnknownInstr, TranslateUnknownInstr,

	/*0x90*/ TranslateUnknownInstr, TranslateUnknownInstr, TranslateUnknownInstr, TranslateUnknownInstr,
	/*0x94*/ TranslateUnknownInstr, TranslateUnknownInstr, TranslateUnknownInstr, TranslateUnknownInstr,
	/*0x98*/ TranslateUnknownInstr, TranslateUnknownInstr, TranslateUnknownInstr, TranslateUnknownInstr,
	/*0x9C*/ TranslateUnknownInstr, TranslateUnknownInstr, TranslateUnknownInstr, TranslateUnknownInstr,

	/*0xA0*/ TranslateUnknownInstr, TranslateUnknownInstr, TranslateUnknownInstr, TranslateUnknownInstr,
	/*0xA4*/ TranslateUnknownInstr, TranslateUnknownInstr, TranslateUnknownInstr, TranslateUnknownInstr,
	/*0xA8*/ TranslateUnknownInstr, TranslateUnknownInstr, TranslateUnknownInstr, TranslateUnknownInstr,
	/*0xAC*/ TranslateUnknownInstr, TranslateUnknownInstr, TranslateUnknownInstr, TranslateUnknownInstr,

	/*0xB0*/ TranslateUnknownInstr, TranslateUnknownInstr, TranslateUnknownInstr, TranslateUnknownInstr,
	/*0xB4*/ TranslateUnknownInstr, TranslateUnknownInstr, TranslateUnknownInstr, TranslateUnknownInstr,
	/*0xB8*/ TranslateUnknownInstr, TranslateUnknownInstr, TranslateUnknownInstr, TranslateUnknownInstr,
	/*0xBC*/ TranslateUnknownInstr, TranslateUnknownInstr, TranslateUnknownInstr, TranslateUnknownInstr,

	/*0xC0*/ TranslateUnknownInstr, TranslateUnknownInstr, TranslateUnknownInstr, TranslateUnknownInstr,
	/*0xC4*/ TranslateUnknownInstr, TranslateUnknownInstr, TranslateUnknownInstr, TranslateUnknownInstr,
	/*0xC8*/ TranslateUnknownInstr, TranslateUnknownInstr, TranslateUnknownInstr, TranslateUnknownInstr,
	/*0xCC*/ TranslateUnknownInstr, TranslateUnknownInstr, TranslateUnknownInstr, TranslateUnknownInstr,

	/*0xD0*/ TranslateUnknownInstr, TranslateUnknownInstr, TranslateUnknownInstr, TranslateUnknownInstr,
	/*0xD4*/ TranslateUnknownInstr, TranslateUnknownInstr, TranslateUnknownInstr, TranslateUnknownInstr,
	/*0xD8*/ TranslateUnknownInstr, TranslateUnknownInstr, TranslateUnknownInstr, TranslateUnknownInstr,
	/*0xDC*/ TranslateUnknownInstr, TranslateUnknownInstr, TranslateUnknownInstr, TranslateUnknownInstr,

	/*0xE0*/ TranslateUnknownInstr, TranslateUnknownInstr, TranslateUnknownInstr, TranslateUnknownInstr,
	/*0xE4*/ TranslateUnknownInstr, TranslateUnknownInstr, TranslateUnknownInstr, TranslateUnknownInstr,
	/*0xE8*/ TranslateUnknownInstr, TranslateUnknownInstr, TranslateUnknownInstr, TranslateUnknownInstr,
	/*0xEC*/ TranslateUnknownInstr, TranslateUnknownInstr, TranslateUnknownInstr, TranslateUnknownInstr,

	/*0xF0*/ TranslateUnknownInstr, TranslateUnknownInstr, TranslateUnknownInstr, TranslateUnknownInstr,
	/*0xF4*/ TranslateUnknownInstr, TranslateUnknownInstr, TranslateUnknownInstr, TranslateUnknownInstr,
	/*0xF8*/ TranslateUnknownInstr, TranslateUnknownInstr, TranslateUnknownInstr, TranslateUnknownInstr,
	/*0xFC*/ TranslateUnknownInstr, TranslateUnknownInstr, TranslateUnknownInstr, TranslateUnknownInstr
};

const TranslateOperandsFunc TranslateOperandsTable00[] = {
	/*0x00*/ TranslateUnknownOp, TranslateUnknownOp, TranslateUnknownOp, TranslateUnknownOp,
	/*0x04*/ TranslateUnknownOp, TranslateUnknownOp, TranslateUnknownOp, TranslateUnknownOp,
	/*0x08*/ TranslateUnknownOp, TranslateUnknownOp, TranslateUnknownOp, TranslateUnknownOp,
	/*0x0C*/ TranslateUnknownOp, TranslateUnknownOp, TranslateUnknownOp, TranslateUnknownOp,

	/*0x10*/ TranslateUnknownOp, TranslateUnknownOp, TranslateUnknownOp, TranslateUnknownOp,
	/*0x14*/ TranslateUnknownOp, TranslateUnknownOp, TranslateUnknownOp, TranslateUnknownOp,
	/*0x18*/ TranslateUnknownOp, TranslateUnknownOp, TranslateUnknownOp, TranslateUnknownOp,
	/*0x1C*/ TranslateUnknownOp, TranslateUnknownOp, TranslateUnknownOp, TranslateUnknownOp,

	/*0x20*/ TranslateUnknownOp, TranslateUnknownOp, TranslateUnknownOp, TranslateUnknownOp,
	/*0x24*/ TranslateUnknownOp, TranslateUnknownOp, TranslateUnknownOp, TranslateUnknownOp,
	/*0x28*/ TranslateUnknownOp, TranslateUnknownOp, TranslateUnknownOp, TranslateUnknownOp,
	/*0x2C*/ TranslateUnknownOp, TranslateUnknownOp, TranslateUnknownOp, TranslateUnknownOp,

	/*0x30*/ TranslateUnknownOp, TranslateUnknownOp, TranslateUnknownOp, TranslateRegModRM,
	/*0x34*/ TranslateUnknownOp, TranslateUnknownOp, TranslateUnknownOp, TranslateUnknownOp,
	/*0x38*/ TranslateUnknownOp, TranslateUnknownOp, TranslateUnknownOp, TranslateRegModRM,
	/*0x3C*/ TranslateUnknownOp, TranslateUnknownOp, TranslateUnknownOp, TranslateUnknownOp,

	/*0x40*/ TranslateNoOp, TranslateNoOp, TranslateNoOp, TranslateNoOp,
	/*0x44*/ TranslateNoOp, TranslateNoOp, TranslateNoOp, TranslateNoOp,
	/*0x48*/ TranslateNoOp, TranslateNoOp, TranslateNoOp, TranslateNoOp,
	/*0x4C*/ TranslateNoOp, TranslateNoOp, TranslateNoOp, TranslateNoOp,

	/*0x50*/ TranslateNoOp, TranslateNoOp, TranslateNoOp, TranslateNoOp,
	/*0x54*/ TranslateNoOp, TranslateNoOp, TranslateNoOp, TranslateNoOp,
	/*0x58*/ TranslateNoOp, TranslateNoOp, TranslateNoOp, TranslateNoOp,
	/*0x5C*/ TranslateNoOp, TranslateNoOp, TranslateNoOp, TranslateNoOp,

	/*0x60*/ TranslateUnknownOp, TranslateUnknownOp, TranslateUnknownOp, TranslateUnknownOp,
	/*0x64*/ TranslateUnknownOp, TranslateUnknownOp, TranslateUnknownOp, TranslateUnknownOp,
	/*0x68*/ TranslateUnknownOp, TranslateUnknownOp, TranslateUnknownOp, TranslateUnknownOp,
	/*0x6C*/ TranslateUnknownOp, TranslateUnknownOp, TranslateUnknownOp, TranslateUnknownOp,

	/*0x70*/ TranslateImm8, TranslateImm8, TranslateImm8, TranslateImm8,
	/*0x74*/ TranslateImm8, TranslateImm8, TranslateImm8, TranslateImm8,
	/*0x78*/ TranslateImm8, TranslateImm8, TranslateImm8, TranslateImm8,
	/*0x7C*/ TranslateImm8, TranslateImm8, TranslateImm8, TranslateImm8,

	/*0x80*/ TranslateUnknownOp, TranslateUnknownOp, TranslateUnknownOp, TranslateModRMImm8,
	/*0x84*/ TranslateUnknownOp, TranslateUnknownOp, TranslateUnknownOp, TranslateUnknownOp,
	/*0x88*/ TranslateUnknownOp, TranslateModRMReg, TranslateUnknownOp, TranslateRegModRM,
	/*0x8C*/ TranslateUnknownOp, TranslateUnknownOp, TranslateUnknownOp, TranslateUnknownOp,

	/*0x90*/ TranslateNoOp, TranslateNoOp, TranslateNoOp, TranslateNoOp,
	/*0x94*/ TranslateNoOp, TranslateNoOp, TranslateNoOp, TranslateNoOp,
	/*0x98*/ TranslateUnknownOp, TranslateUnknownOp, TranslateUnknownOp, TranslateUnknownOp,
	/*0x9C*/ TranslateUnknownOp, TranslateUnknownOp, TranslateUnknownOp, TranslateUnknownOp,

	/*0xA0*/ TranslateUnknownOp, TranslateUnknownOp, TranslateUnknownOp, TranslateUnknownOp,
	/*0xA4*/ TranslateUnknownOp, TranslateUnknownOp, TranslateUnknownOp, TranslateUnknownOp,
	/*0xA8*/ TranslateUnknownOp, TranslateUnknownOp, TranslateUnknownOp, TranslateUnknownOp,
	/*0xAC*/ TranslateUnknownOp, TranslateUnknownOp, TranslateUnknownOp, TranslateUnknownOp,

	/*0xB0*/ TranslateUnknownOp, TranslateUnknownOp, TranslateUnknownOp, TranslateUnknownOp,
	/*0xB4*/ TranslateUnknownOp, TranslateUnknownOp, TranslateUnknownOp, TranslateUnknownOp,
	/*0xB8*/ TranslateUnknownOp, TranslateUnknownOp, TranslateUnknownOp, TranslateUnknownOp,
	/*0xBC*/ TranslateUnknownOp, TranslateUnknownOp, TranslateUnknownOp, TranslateUnknownOp,

	/*0xC0*/ TranslateUnknownOp, TranslateUnknownOp, TranslateUnknownOp, TranslateNoOp,
	/*0xC4*/ TranslateUnknownOp, TranslateUnknownOp, TranslateUnknownOp, TranslateUnknownOp,
	/*0xC8*/ TranslateUnknownOp, TranslateUnknownOp, TranslateUnknownOp, TranslateUnknownOp,
	/*0xCC*/ TranslateUnknownOp, TranslateUnknownOp, TranslateUnknownOp, TranslateUnknownOp,

	/*0xD0*/ TranslateUnknownOp, TranslateUnknownOp, TranslateUnknownOp, TranslateUnknownOp,
	/*0xD4*/ TranslateUnknownOp, TranslateUnknownOp, TranslateUnknownOp, TranslateUnknownOp,
	/*0xD8*/ TranslateUnknownOp, TranslateUnknownOp, TranslateUnknownOp, TranslateUnknownOp,
	/*0xDC*/ TranslateUnknownOp, TranslateUnknownOp, TranslateUnknownOp, TranslateUnknownOp,

	/*0xE0*/ TranslateUnknownOp, TranslateUnknownOp, TranslateUnknownOp, TranslateUnknownOp,
	/*0xE4*/ TranslateUnknownOp, TranslateUnknownOp, TranslateUnknownOp, TranslateUnknownOp,
	/*0xE8*/ TranslateUnknownOp, TranslateImm32, TranslateUnknownOp, TranslateUnknownOp,
	/*0xEC*/ TranslateUnknownOp, TranslateUnknownOp, TranslateUnknownOp, TranslateUnknownOp,

	/*0xF0*/ TranslateUnknownOp, TranslateUnknownOp, TranslateUnknownOp, TranslateUnknownOp,
	/*0xF4*/ TranslateUnknownOp, TranslateUnknownOp, TranslateUnknownOp, TranslateUnknownOp,
	/*0xF8*/ TranslateUnknownOp, TranslateUnknownOp, TranslateUnknownOp, TranslateUnknownOp,
	/*0xFC*/ TranslateUnknownOp, TranslateUnknownOp, TranslateUnknownOp, TranslateUnknownOp
};

const TranslateOperandsFunc TranslateOperandsTable0F[] = {
	/*0x00*/ TranslateUnknownOp, TranslateUnknownOp, TranslateUnknownOp, TranslateUnknownOp,
	/*0x04*/ TranslateUnknownOp, TranslateUnknownOp, TranslateUnknownOp, TranslateUnknownOp,
	/*0x08*/ TranslateUnknownOp, TranslateUnknownOp, TranslateUnknownOp, TranslateUnknownOp,
	/*0x0C*/ TranslateUnknownOp, TranslateUnknownOp, TranslateUnknownOp, TranslateUnknownOp,

	/*0x10*/ TranslateUnknownOp, TranslateUnknownOp, TranslateUnknownOp, TranslateUnknownOp,
	/*0x14*/ TranslateUnknownOp, TranslateUnknownOp, TranslateUnknownOp, TranslateUnknownOp,
	/*0x18*/ TranslateUnknownOp, TranslateUnknownOp, TranslateUnknownOp, TranslateUnknownOp,
	/*0x1C*/ TranslateUnknownOp, TranslateUnknownOp, TranslateUnknownOp, TranslateUnknownOp,

	/*0x20*/ TranslateUnknownOp, TranslateUnknownOp, TranslateUnknownOp, TranslateUnknownOp,
	/*0x24*/ TranslateUnknownOp, TranslateUnknownOp, TranslateUnknownOp, TranslateUnknownOp,
	/*0x28*/ TranslateUnknownOp, TranslateUnknownOp, TranslateUnknownOp, TranslateUnknownOp,
	/*0x2C*/ TranslateUnknownOp, TranslateUnknownOp, TranslateUnknownOp, TranslateUnknownOp,

	/*0x30*/ TranslateUnknownOp, TranslateUnknownOp, TranslateUnknownOp, TranslateUnknownOp,
	/*0x34*/ TranslateUnknownOp, TranslateUnknownOp, TranslateUnknownOp, TranslateUnknownOp,
	/*0x38*/ TranslateUnknownOp, TranslateUnknownOp, TranslateUnknownOp, TranslateUnknownOp,
	/*0x3C*/ TranslateUnknownOp, TranslateUnknownOp, TranslateUnknownOp, TranslateUnknownOp,

	/*0x40*/ TranslateUnknownOp, TranslateUnknownOp, TranslateUnknownOp, TranslateUnknownOp,
	/*0x44*/ TranslateUnknownOp, TranslateUnknownOp, TranslateUnknownOp, TranslateUnknownOp,
	/*0x48*/ TranslateUnknownOp, TranslateUnknownOp, TranslateUnknownOp, TranslateUnknownOp,
	/*0x4C*/ TranslateUnknownOp, TranslateUnknownOp, TranslateUnknownOp, TranslateUnknownOp,

	/*0x50*/ TranslateUnknownOp, TranslateUnknownOp, TranslateUnknownOp, TranslateUnknownOp,
	/*0x54*/ TranslateUnknownOp, TranslateUnknownOp, TranslateUnknownOp, TranslateUnknownOp,
	/*0x58*/ TranslateUnknownOp, TranslateUnknownOp, TranslateUnknownOp, TranslateUnknownOp,
	/*0x5C*/ TranslateUnknownOp, TranslateUnknownOp, TranslateUnknownOp, TranslateUnknownOp,

	/*0x60*/ TranslateUnknownOp, TranslateUnknownOp, TranslateUnknownOp, TranslateUnknownOp,
	/*0x64*/ TranslateUnknownOp, TranslateUnknownOp, TranslateUnknownOp, TranslateUnknownOp,
	/*0x68*/ TranslateUnknownOp, TranslateUnknownOp, TranslateUnknownOp, TranslateUnknownOp,
	/*0x6C*/ TranslateUnknownOp, TranslateUnknownOp, TranslateUnknownOp, TranslateUnknownOp,

	/*0x70*/ TranslateUnknownOp, TranslateUnknownOp, TranslateUnknownOp, TranslateUnknownOp,
	/*0x74*/ TranslateUnknownOp, TranslateUnknownOp, TranslateUnknownOp, TranslateUnknownOp,
	/*0x78*/ TranslateUnknownOp, TranslateUnknownOp, TranslateUnknownOp, TranslateUnknownOp,
	/*0x7C*/ TranslateUnknownOp, TranslateUnknownOp, TranslateUnknownOp, TranslateUnknownOp,

	/*0x80*/ TranslateUnknownOp, TranslateUnknownOp, TranslateUnknownOp, TranslateUnknownOp,
	/*0x84*/ TranslateUnknownOp, TranslateUnknownOp, TranslateUnknownOp, TranslateUnknownOp,
	/*0x88*/ TranslateUnknownOp, TranslateUnknownOp, TranslateUnknownOp, TranslateUnknownOp,
	/*0x8C*/ TranslateUnknownOp, TranslateUnknownOp, TranslateUnknownOp, TranslateUnknownOp,

	/*0x90*/ TranslateUnknownOp, TranslateUnknownOp, TranslateUnknownOp, TranslateUnknownOp,
	/*0x94*/ TranslateUnknownOp, TranslateUnknownOp, TranslateUnknownOp, TranslateUnknownOp,
	/*0x98*/ TranslateUnknownOp, TranslateUnknownOp, TranslateUnknownOp, TranslateUnknownOp,
	/*0x9C*/ TranslateUnknownOp, TranslateUnknownOp, TranslateUnknownOp, TranslateUnknownOp,

	/*0xA0*/ TranslateUnknownOp, TranslateUnknownOp, TranslateUnknownOp, TranslateUnknownOp,
	/*0xA4*/ TranslateUnknownOp, TranslateUnknownOp, TranslateUnknownOp, TranslateUnknownOp,
	/*0xA8*/ TranslateUnknownOp, TranslateUnknownOp, TranslateUnknownOp, TranslateUnknownOp,
	/*0xAC*/ TranslateUnknownOp, TranslateUnknownOp, TranslateUnknownOp, TranslateUnknownOp,

	/*0xB0*/ TranslateUnknownOp, TranslateUnknownOp, TranslateUnknownOp, TranslateUnknownOp,
	/*0xB4*/ TranslateUnknownOp, TranslateUnknownOp, TranslateUnknownOp, TranslateUnknownOp,
	/*0xB8*/ TranslateUnknownOp, TranslateUnknownOp, TranslateUnknownOp, TranslateUnknownOp,
	/*0xBC*/ TranslateUnknownOp, TranslateUnknownOp, TranslateUnknownOp, TranslateUnknownOp,

	/*0xC0*/ TranslateUnknownOp, TranslateUnknownOp, TranslateUnknownOp, TranslateUnknownOp,
	/*0xC4*/ TranslateUnknownOp, TranslateUnknownOp, TranslateUnknownOp, TranslateUnknownOp,
	/*0xC8*/ TranslateUnknownOp, TranslateUnknownOp, TranslateUnknownOp, TranslateUnknownOp,
	/*0xCC*/ TranslateUnknownOp, TranslateUnknownOp, TranslateUnknownOp, TranslateUnknownOp,

	/*0xD0*/ TranslateUnknownOp, TranslateUnknownOp, TranslateUnknownOp, TranslateUnknownOp,
	/*0xD4*/ TranslateUnknownOp, TranslateUnknownOp, TranslateUnknownOp, TranslateUnknownOp,
	/*0xD8*/ TranslateUnknownOp, TranslateUnknownOp, TranslateUnknownOp, TranslateUnknownOp,
	/*0xDC*/ TranslateUnknownOp, TranslateUnknownOp, TranslateUnknownOp, TranslateUnknownOp,

	/*0xE0*/ TranslateUnknownOp, TranslateUnknownOp, TranslateUnknownOp, TranslateUnknownOp,
	/*0xE4*/ TranslateUnknownOp, TranslateUnknownOp, TranslateUnknownOp, TranslateUnknownOp,
	/*0xE8*/ TranslateUnknownOp, TranslateUnknownOp, TranslateUnknownOp, TranslateUnknownOp,
	/*0xEC*/ TranslateUnknownOp, TranslateUnknownOp, TranslateUnknownOp, TranslateUnknownOp,

	/*0xF0*/ TranslateUnknownOp, TranslateUnknownOp, TranslateUnknownOp, TranslateUnknownOp,
	/*0xF4*/ TranslateUnknownOp, TranslateUnknownOp, TranslateUnknownOp, TranslateUnknownOp,
	/*0xF8*/ TranslateUnknownOp, TranslateUnknownOp, TranslateUnknownOp, TranslateUnknownOp,
	/*0xFC*/ TranslateUnknownOp, TranslateUnknownOp, TranslateUnknownOp, TranslateUnknownOp
};