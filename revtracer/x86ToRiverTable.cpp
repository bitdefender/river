#include "riverinternl.h"
#include "modrm32.h"

extern const WORD specTbl00[];
extern const WORD specTbl0F[];

/* helper functions */
WORD GetSpecifiers(RiverInstruction *ri) {
	const WORD *specTbl = specTbl00;

	if (RIVER_MODIFIER_EXT & ri->modifiers) {
		specTbl = specTbl0F;
	}

	return specTbl[ri->opCode];
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

void TranslateRegisterOperand(BYTE opIdx, RiverInstruction *ri, BYTE reg) {
	BYTE opType = RIVER_OPTYPE_REG;
	if (RIVER_MODIFIER_O8 & ri->modifiers) {
		opType |= RIVER_OPSIZE_8;
	}
	else if (RIVER_MODIFIER_O16 & ri->modifiers) {
		opType |= RIVER_OPSIZE_16;
	}

	ri->opTypes[opIdx] = opType;

	if (ri->specifiers && (1 << opIdx)) {
		ri->operands[opIdx].asRegister.name = reg; //ask for a new register version	
	} else {
		ri->operands[opIdx].asRegister.name = reg;
	}
}

void TranslateSIBByte(BYTE opIdx, RiverInstruction *ri, BYTE **px86) {
	RiverAddress *rAddr = ri->operands[opIdx].asAddress;

	rAddr->sib = **px86;

	BYTE scale = (**px86) >> 6;
	BYTE index = ((**px86) >> 3) & 0x07;
	BYTE base = (**px86) & 0x07;

	static const BYTE scales[] = { 1, 2, 4, 8 };

	rAddr->type |= RIVER_ADDR_SCALE;
	rAddr->scale = scales[scale];

	rAddr->index.name = index;

	rAddr->base.name = base;

	(*px86)++;
}

void TranslateModRMOperand(BYTE opIdx, RiverInstruction *ri, BYTE **px86) {
	RiverAddress *rAddr;
	BYTE mod = (**px86) >> 6;
	BYTE rm = **px86 & 0x07;

	if (0x03 == mod) {
		TranslateRegisterOperand(opIdx, ri, rm);
		(*px86)++;
		return;
	}
	
	rAddr = new RiverAddress;
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
		TranslateSIBByte(opIdx, ri, px86); 
	} else {
		rAddr->base.name = rm;
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

void TranslateUnknownInstr(_exec_env *pEnv, RiverInstruction *ri, BYTE **px86, DWORD *pFlags) {
	__asm int 3;
	(*px86)++;
}

// handles +r opcodes such as 0x50+r
// base template argument corresponds to op eax
template <BYTE base> void TranslatePlusRegInstr(_exec_env *pEnv, RiverInstruction *ri, BYTE **px86, DWORD *pFlags) {
	ri->opCode = base;
	ri->specifiers = GetSpecifiers(ri);
	TranslateRegisterOperand(0, ri, **px86 - base);
	(*px86)++;
}

// translate single opcode instruction
void TranslateDefaultInstr(_exec_env *pEnv, RiverInstruction *ri, BYTE **px86, DWORD *pFlags) {
	ri->opCode = **px86;
	ri->specifiers = GetSpecifiers(ri);
	(*px86)++;
}

void TranslateBranchInstr(_exec_env *pEnv, RiverInstruction *ri, BYTE **px86, DWORD *pFlags) {
	ri->opCode = **px86;
	ri->specifiers = GetSpecifiers(ri);
	*pFlags |= RIVER_FLAG_BRANCH;
	(*px86)++;
}

//translate prefix
template <WORD flag> void TranslateDefaultInstr(_exec_env *pEnv, RiverInstruction *ri, BYTE **px86, DWORD *pFlags) {
	ri->modifiers |= flag;
	(*px86)++;
}

/* operand decoders */
void TranslateUnknownOp(_exec_env *pEnv, RiverInstruction *ri, BYTE **px86) {
	__asm int 3;
}

void TranslateNoOp(_exec_env *pEnv, RiverInstruction *ri, BYTE **px86) {
}

void TranslateRegModRM(_exec_env *pEnv, RiverInstruction *ri, BYTE **px86) {
	//BYTE mod = (**px86) >> 6;
	BYTE sec = ((**px86) >> 3) & 0x07;
	//BYTE rm = **px86 & 0x07;

	TranslateRegisterOperand(0, ri, sec);
	TranslateModRMOperand(1, ri, px86);
}

void TranslateImm8(_exec_env *pEnv, RiverInstruction *ri, BYTE **px86) {
	TranslateImmOperand(0, ri, px86, RIVER_OPSIZE_8);
}


/* Specifier tables */

extern "C" const WORD specTbl00[] = {
	/*0x00*/ 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF,
	/*0x08*/ 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF,

	/*0x10*/ 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF,
	/*0x18*/ 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF,

	/*0x20*/ 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF,
	/*0x28*/ 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF,

	/*0x30*/ 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF,
	/*0x38*/ 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF,

	/*0x40*/ 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF,
	/*0x48*/ 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF,

	/*0x50*/ 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF,
	/*0x58*/ 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF,

	/*0x60*/ 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF,
	/*0x68*/ 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF,

	/*0x70*/ 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF,
	/*0x78*/ 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF,

	/*0x80*/ 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF,
	/*0x88*/ 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF,

	/*0x90*/ 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF,
	/*0x98*/ 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF,

	/*0xA0*/ 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF,
	/*0xA8*/ 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF,

	/*0xB0*/ 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF,
	/*0xB8*/ 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF,

	/*0xC0*/ 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF,
	/*0xC8*/ 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF,

	/*0xD0*/ 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF,
	/*0xD8*/ 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF,

	/*0xE0*/ 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF,
	/*0xE8*/ 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF,

	/*0xF0*/ 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF,
	/*0xF8*/ 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF
};

extern "C" const WORD specTbl0F[] = {
	/*0x00*/ 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF,
	/*0x08*/ 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF,

	/*0x10*/ 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF,
	/*0x18*/ 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF,

	/*0x20*/ 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF,
	/*0x28*/ 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF,

	/*0x30*/ 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF,
	/*0x38*/ 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF,

	/*0x40*/ 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF,
	/*0x48*/ 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF,

	/*0x50*/ 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF,
	/*0x58*/ 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF,

	/*0x60*/ 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF,
	/*0x68*/ 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF,

	/*0x70*/ 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF,
	/*0x78*/ 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF,

	/*0x80*/ 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF,
	/*0x88*/ 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF,

	/*0x90*/ 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF,
	/*0x98*/ 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF,

	/*0xA0*/ 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF,
	/*0xA8*/ 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF,

	/*0xB0*/ 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF,
	/*0xB8*/ 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF,

	/*0xC0*/ 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF,
	/*0xC8*/ 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF,

	/*0xD0*/ 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF,
	/*0xD8*/ 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF,

	/*0xE0*/ 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF,
	/*0xE8*/ 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF,

	/*0xF0*/ 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF,
	/*0xF8*/ 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF
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

	/*0x30*/ TranslateUnknownInstr, TranslateUnknownInstr, TranslateUnknownInstr, TranslateUnknownInstr,
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

	/*0x70*/ TranslateBranchInstr, TranslateBranchInstr, TranslateBranchInstr, TranslateBranchInstr,
	/*0x74*/ TranslateBranchInstr, TranslateBranchInstr, TranslateBranchInstr, TranslateBranchInstr,
	/*0x78*/ TranslateBranchInstr, TranslateBranchInstr, TranslateBranchInstr, TranslateBranchInstr,
	/*0x7C*/ TranslateBranchInstr, TranslateBranchInstr, TranslateBranchInstr, TranslateBranchInstr,
	
	/*0x80*/ TranslateUnknownInstr, TranslateUnknownInstr, TranslateUnknownInstr, TranslateUnknownInstr,
	/*0x84*/ TranslateUnknownInstr, TranslateUnknownInstr, TranslateUnknownInstr, TranslateUnknownInstr,
	/*0x88*/ TranslateUnknownInstr, TranslateUnknownInstr, TranslateUnknownInstr, TranslateDefaultInstr,
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

	/*0x30*/ TranslateUnknownOp, TranslateUnknownOp, TranslateUnknownOp, TranslateUnknownOp,
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

	/*0x80*/ TranslateUnknownOp, TranslateUnknownOp, TranslateUnknownOp, TranslateUnknownOp,
	/*0x84*/ TranslateUnknownOp, TranslateUnknownOp, TranslateUnknownOp, TranslateUnknownOp,
	/*0x88*/ TranslateUnknownOp, TranslateUnknownOp, TranslateUnknownOp, TranslateRegModRM,
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