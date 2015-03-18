#include "riverinternl.h"
#include "modrm32.h"



inline void MakeNop(struct _river_instruction *pRiver) {
	memset(pRiver, 0, sizeof(*pRiver));
}

BYTE MissingOpcode(struct _exec_env *pEnv, DWORD *pFlags, BYTE *px86, struct _river_instruction *pRiver, DWORD *dwInstrCount) {
	__asm int 3;
}


/* 50+r - PUSH reg */
BYTE PushReg(struct _exec_env *pEnv, DWORD *pFlags, BYTE *px86, struct _river_instruction *pRiver, DWORD *dwInstrCount) {
	// riversave esp$c
	pRiver[0].opCode = 0x50;
	pRiver[0].opExt = 0;
	pRiver[0].opPfx = FLAG_RIVER;
	pRiver[0].Operands.unaryReg = GetCurrentReg(pEnv, REG_ESP);

	// lea esp$n, [esp$c - 4]
	pRiver[1].opCode = 0x8D;
	pRiver[1].opExt = 0; //for now
	pRiver[1].Operands.binaryRegMem.first = GetNextReg(pEnv, REG_ESP);
	pRiver[1].Operands.binaryRegMem.second.flags = 0; //for now, must specify disp8
	pRiver[1].Operands.binaryRegMem.second.scale = 1;
	pRiver[1].Operands.binaryRegMem.second.index = 0;
	pRiver[1].Operands.binaryRegMem.second.base = GetCurrentReg(pEnv, REG_ESP);
	pRiver[1].Operands.binaryRegMem.second.disp = 0xFC;
	NextReg(pEnv, REG_ESP);
	
	// riversave [esp$n]


	// mov [esp$n], reg$c


	return 1;
}




X86toRiverOp X86ToRiverInstructions[] = {
	/*0x00*/ MissingOpcode, MissingOpcode, MissingOpcode, MissingOpcode,
	/*0x04*/ MissingOpcode, MissingOpcode, MissingOpcode, MissingOpcode,
	/*0x08*/ MissingOpcode, MissingOpcode, MissingOpcode, MissingOpcode,
	/*0x0C*/ MissingOpcode, MissingOpcode, MissingOpcode, MissingOpcode,

	/*0x10*/ MissingOpcode, MissingOpcode, MissingOpcode, MissingOpcode,
	/*0x14*/ MissingOpcode, MissingOpcode, MissingOpcode, MissingOpcode,
	/*0x18*/ MissingOpcode, MissingOpcode, MissingOpcode, MissingOpcode,
	/*0x1C*/ MissingOpcode, MissingOpcode, MissingOpcode, MissingOpcode,

	/*0x20*/ MissingOpcode, MissingOpcode, MissingOpcode, MissingOpcode,
	/*0x24*/ MissingOpcode, MissingOpcode, MissingOpcode, MissingOpcode,
	/*0x28*/ MissingOpcode, MissingOpcode, MissingOpcode, MissingOpcode,
	/*0x2C*/ MissingOpcode, MissingOpcode, MissingOpcode, MissingOpcode,

	/*0x30*/ MissingOpcode, MissingOpcode, MissingOpcode, MissingOpcode,
	/*0x34*/ MissingOpcode, MissingOpcode, MissingOpcode, MissingOpcode,
	/*0x38*/ MissingOpcode, MissingOpcode, MissingOpcode, MissingOpcode,
	/*0x3C*/ MissingOpcode, MissingOpcode, MissingOpcode, MissingOpcode,

	/*0x40*/ MissingOpcode, MissingOpcode, MissingOpcode, MissingOpcode,
	/*0x44*/ MissingOpcode, MissingOpcode, MissingOpcode, MissingOpcode,
	/*0x48*/ MissingOpcode, MissingOpcode, MissingOpcode, MissingOpcode,
	/*0x4C*/ MissingOpcode, MissingOpcode, MissingOpcode, MissingOpcode,

	/*0x50*/ MissingOpcode, MissingOpcode, MissingOpcode, MissingOpcode,
	/*0x54*/ MissingOpcode, MissingOpcode, MissingOpcode, MissingOpcode,
	/*0x58*/ MissingOpcode, MissingOpcode, MissingOpcode, MissingOpcode,
	/*0x5C*/ MissingOpcode, MissingOpcode, MissingOpcode, MissingOpcode,

	/*0x60*/ MissingOpcode, MissingOpcode, MissingOpcode, MissingOpcode,
	/*0x64*/ MissingOpcode, MissingOpcode, MissingOpcode, MissingOpcode,
	/*0x68*/ MissingOpcode, MissingOpcode, MissingOpcode, MissingOpcode,
	/*0x6C*/ MissingOpcode, MissingOpcode, MissingOpcode, MissingOpcode,

	/*0x70*/ MissingOpcode, MissingOpcode, MissingOpcode, MissingOpcode,
	/*0x74*/ MissingOpcode, MissingOpcode, MissingOpcode, MissingOpcode,
	/*0x78*/ MissingOpcode, MissingOpcode, MissingOpcode, MissingOpcode,
	/*0x7C*/ MissingOpcode, MissingOpcode, MissingOpcode, MissingOpcode,

	/*0x80*/ MissingOpcode, MissingOpcode, MissingOpcode, MissingOpcode,
	/*0x84*/ MissingOpcode, MissingOpcode, MissingOpcode, MissingOpcode,
	/*0x88*/ MissingOpcode, MissingOpcode, MissingOpcode, MissingOpcode,
	/*0x8C*/ MissingOpcode, MissingOpcode, MissingOpcode, MissingOpcode,

	/*0x90*/ MissingOpcode, MissingOpcode, MissingOpcode, MissingOpcode,
	/*0x94*/ MissingOpcode, MissingOpcode, MissingOpcode, MissingOpcode,
	/*0x98*/ MissingOpcode, MissingOpcode, MissingOpcode, MissingOpcode,
	/*0x9C*/ MissingOpcode, MissingOpcode, MissingOpcode, MissingOpcode,

	/*0xA0*/ MissingOpcode, MissingOpcode, MissingOpcode, MissingOpcode,
	/*0xA4*/ MissingOpcode, MissingOpcode, MissingOpcode, MissingOpcode,
	/*0xA8*/ MissingOpcode, MissingOpcode, MissingOpcode, MissingOpcode,
	/*0xAC*/ MissingOpcode, MissingOpcode, MissingOpcode, MissingOpcode,

	/*0xB0*/ MissingOpcode, MissingOpcode, MissingOpcode, MissingOpcode,
	/*0xB4*/ MissingOpcode, MissingOpcode, MissingOpcode, MissingOpcode,
	/*0xB8*/ MissingOpcode, MissingOpcode, MissingOpcode, MissingOpcode,
	/*0xBC*/ MissingOpcode, MissingOpcode, MissingOpcode, MissingOpcode,

	/*0xC0*/ MissingOpcode, MissingOpcode, MissingOpcode, MissingOpcode,
	/*0xC4*/ MissingOpcode, MissingOpcode, MissingOpcode, MissingOpcode,
	/*0xC8*/ MissingOpcode, MissingOpcode, MissingOpcode, MissingOpcode,
	/*0xCC*/ MissingOpcode, MissingOpcode, MissingOpcode, MissingOpcode,

	/*0xD0*/ MissingOpcode, MissingOpcode, MissingOpcode, MissingOpcode,
	/*0xD4*/ MissingOpcode, MissingOpcode, MissingOpcode, MissingOpcode,
	/*0xD8*/ MissingOpcode, MissingOpcode, MissingOpcode, MissingOpcode,
	/*0xDC*/ MissingOpcode, MissingOpcode, MissingOpcode, MissingOpcode,

	/*0xE0*/ MissingOpcode, MissingOpcode, MissingOpcode, MissingOpcode,
	/*0xE4*/ MissingOpcode, MissingOpcode, MissingOpcode, MissingOpcode,
	/*0xE8*/ MissingOpcode, MissingOpcode, MissingOpcode, MissingOpcode,
	/*0xEC*/ MissingOpcode, MissingOpcode, MissingOpcode, MissingOpcode,

	/*0xF0*/ MissingOpcode, MissingOpcode, MissingOpcode, MissingOpcode,
	/*0xF4*/ MissingOpcode, MissingOpcode, MissingOpcode, MissingOpcode,
	/*0xF8*/ MissingOpcode, MissingOpcode, MissingOpcode, MissingOpcode,
	/*0xFC*/ MissingOpcode, MissingOpcode, MissingOpcode, MissingOpcode
};


typedef void(*x86TranslationFunction)(struct _exec_env *pEnv, BYTE *px86, struct _river_instruction *pRiver);

DWORD TranslateMissingOpcode(struct _exec_env *pEnv, BYTE *px86, struct _river_instruction *pRiver) {
	__asm int 3;
}


DWORD TranslatePushRegOpcode(struct _exec_env *pEnv, BYTE *px86, struct _river_instruction *pRiver) {
	pRiver->opCode = 0x50;
	//TranslateRegister(pEnv, *px86 - 0x50, &pRiver->Operands.unaryReg);
}

DWORD TranslateSingleOpcode(struct _exec_env *pEnv, BYTE *px86, struct _river_instruction *pRiver) {
	pRiver->opCode = *px86;
}





x86TranslationFunction TranslateOpcode0x00[] = {
	/*0x00*/ TranslateMissingOpcode, TranslateMissingOpcode, TranslateMissingOpcode, TranslateMissingOpcode,
	/*0x04*/ TranslateMissingOpcode, TranslateMissingOpcode, TranslateMissingOpcode, TranslateMissingOpcode,
	/*0x08*/ TranslateMissingOpcode, TranslateMissingOpcode, TranslateMissingOpcode, TranslateMissingOpcode,
	/*0x0C*/ TranslateMissingOpcode, TranslateMissingOpcode, TranslateMissingOpcode, TranslateMissingOpcode,

	/*0x10*/ TranslateMissingOpcode, TranslateMissingOpcode, TranslateMissingOpcode, TranslateMissingOpcode,
	/*0x14*/ TranslateMissingOpcode, TranslateMissingOpcode, TranslateMissingOpcode, TranslateMissingOpcode,
	/*0x18*/ TranslateMissingOpcode, TranslateMissingOpcode, TranslateMissingOpcode, TranslateMissingOpcode,
	/*0x1C*/ TranslateMissingOpcode, TranslateMissingOpcode, TranslateMissingOpcode, TranslateMissingOpcode,

	/*0x20*/ TranslateMissingOpcode, TranslateMissingOpcode, TranslateMissingOpcode, TranslateMissingOpcode,
	/*0x24*/ TranslateMissingOpcode, TranslateMissingOpcode, TranslateMissingOpcode, TranslateMissingOpcode,
	/*0x28*/ TranslateMissingOpcode, TranslateMissingOpcode, TranslateMissingOpcode, TranslateMissingOpcode,
	/*0x2C*/ TranslateMissingOpcode, TranslateMissingOpcode, TranslateMissingOpcode, TranslateMissingOpcode,

	/*0x30*/ TranslateMissingOpcode, TranslateMissingOpcode, TranslateMissingOpcode, TranslateMissingOpcode,
	/*0x34*/ TranslateMissingOpcode, TranslateMissingOpcode, TranslateMissingOpcode, TranslateMissingOpcode,
	/*0x38*/ TranslateMissingOpcode, TranslateMissingOpcode, TranslateMissingOpcode, TranslateMissingOpcode,
	/*0x3C*/ TranslateMissingOpcode, TranslateMissingOpcode, TranslateMissingOpcode, TranslateMissingOpcode,

	/*0x40*/ TranslateMissingOpcode, TranslateMissingOpcode, TranslateMissingOpcode, TranslateMissingOpcode,
	/*0x44*/ TranslateMissingOpcode, TranslateMissingOpcode, TranslateMissingOpcode, TranslateMissingOpcode,
	/*0x48*/ TranslateMissingOpcode, TranslateMissingOpcode, TranslateMissingOpcode, TranslateMissingOpcode,
	/*0x4C*/ TranslateMissingOpcode, TranslateMissingOpcode, TranslateMissingOpcode, TranslateMissingOpcode,

	/*0x50*/ TranslatePushRegOpcode, TranslatePushRegOpcode, TranslatePushRegOpcode, TranslatePushRegOpcode,
	/*0x54*/ TranslatePushRegOpcode, TranslatePushRegOpcode, TranslatePushRegOpcode, TranslatePushRegOpcode,
	/*0x58*/ TranslateMissingOpcode, TranslateMissingOpcode, TranslateMissingOpcode, TranslateMissingOpcode,
	/*0x5C*/ TranslateMissingOpcode, TranslateMissingOpcode, TranslateMissingOpcode, TranslateMissingOpcode,

	/*0x60*/ TranslateMissingOpcode, TranslateMissingOpcode, TranslateMissingOpcode, TranslateMissingOpcode,
	/*0x64*/ TranslateMissingOpcode, TranslateMissingOpcode, TranslateMissingOpcode, TranslateMissingOpcode,
	/*0x68*/ TranslateMissingOpcode, TranslateMissingOpcode, TranslateMissingOpcode, TranslateMissingOpcode,
	/*0x6C*/ TranslateMissingOpcode, TranslateMissingOpcode, TranslateMissingOpcode, TranslateMissingOpcode,

	/*0x70*/ TranslateMissingOpcode, TranslateMissingOpcode, TranslateMissingOpcode, TranslateMissingOpcode,
	/*0x74*/ TranslateMissingOpcode, TranslateMissingOpcode, TranslateMissingOpcode, TranslateMissingOpcode,
	/*0x78*/ TranslateMissingOpcode, TranslateMissingOpcode, TranslateMissingOpcode, TranslateMissingOpcode,
	/*0x7C*/ TranslateMissingOpcode, TranslateMissingOpcode, TranslateMissingOpcode, TranslateMissingOpcode,

	/*0x80*/ TranslateMissingOpcode, TranslateMissingOpcode, TranslateMissingOpcode, TranslateMissingOpcode,
	/*0x84*/ TranslateMissingOpcode, TranslateMissingOpcode, TranslateMissingOpcode, TranslateMissingOpcode,
	/*0x88*/ TranslateMissingOpcode, TranslateMissingOpcode, TranslateMissingOpcode, TranslateSingleOpcode,
	/*0x8C*/ TranslateMissingOpcode, TranslateMissingOpcode, TranslateMissingOpcode, TranslateMissingOpcode,

	/*0x90*/ TranslateMissingOpcode, TranslateMissingOpcode, TranslateMissingOpcode, TranslateMissingOpcode,
	/*0x94*/ TranslateMissingOpcode, TranslateMissingOpcode, TranslateMissingOpcode, TranslateMissingOpcode,
	/*0x98*/ TranslateMissingOpcode, TranslateMissingOpcode, TranslateMissingOpcode, TranslateMissingOpcode,
	/*0x9C*/ TranslateMissingOpcode, TranslateMissingOpcode, TranslateMissingOpcode, TranslateMissingOpcode,

	/*0xA0*/ TranslateMissingOpcode, TranslateMissingOpcode, TranslateMissingOpcode, TranslateMissingOpcode,
	/*0xA4*/ TranslateMissingOpcode, TranslateMissingOpcode, TranslateMissingOpcode, TranslateMissingOpcode,
	/*0xA8*/ TranslateMissingOpcode, TranslateMissingOpcode, TranslateMissingOpcode, TranslateMissingOpcode,
	/*0xAC*/ TranslateMissingOpcode, TranslateMissingOpcode, TranslateMissingOpcode, TranslateMissingOpcode,

	/*0xB0*/ TranslateMissingOpcode, TranslateMissingOpcode, TranslateMissingOpcode, TranslateMissingOpcode,
	/*0xB4*/ TranslateMissingOpcode, TranslateMissingOpcode, TranslateMissingOpcode, TranslateMissingOpcode,
	/*0xB8*/ TranslateMissingOpcode, TranslateMissingOpcode, TranslateMissingOpcode, TranslateMissingOpcode,
	/*0xBC*/ TranslateMissingOpcode, TranslateMissingOpcode, TranslateMissingOpcode, TranslateMissingOpcode,

	/*0xC0*/ TranslateMissingOpcode, TranslateMissingOpcode, TranslateMissingOpcode, TranslateMissingOpcode,
	/*0xC4*/ TranslateMissingOpcode, TranslateMissingOpcode, TranslateMissingOpcode, TranslateMissingOpcode,
	/*0xC8*/ TranslateMissingOpcode, TranslateMissingOpcode, TranslateMissingOpcode, TranslateMissingOpcode,
	/*0xCC*/ TranslateMissingOpcode, TranslateMissingOpcode, TranslateMissingOpcode, TranslateMissingOpcode,

	/*0xD0*/ TranslateMissingOpcode, TranslateMissingOpcode, TranslateMissingOpcode, TranslateMissingOpcode,
	/*0xD4*/ TranslateMissingOpcode, TranslateMissingOpcode, TranslateMissingOpcode, TranslateMissingOpcode,
	/*0xD8*/ TranslateMissingOpcode, TranslateMissingOpcode, TranslateMissingOpcode, TranslateMissingOpcode,
	/*0xDC*/ TranslateMissingOpcode, TranslateMissingOpcode, TranslateMissingOpcode, TranslateMissingOpcode,

	/*0xE0*/ TranslateMissingOpcode, TranslateMissingOpcode, TranslateMissingOpcode, TranslateMissingOpcode,
	/*0xE4*/ TranslateMissingOpcode, TranslateMissingOpcode, TranslateMissingOpcode, TranslateMissingOpcode,
	/*0xE8*/ TranslateMissingOpcode, TranslateMissingOpcode, TranslateMissingOpcode, TranslateMissingOpcode,
	/*0xEC*/ TranslateMissingOpcode, TranslateMissingOpcode, TranslateMissingOpcode, TranslateMissingOpcode,

	/*0xF0*/ TranslateMissingOpcode, TranslateMissingOpcode, TranslateMissingOpcode, TranslateMissingOpcode,
	/*0xF4*/ TranslateMissingOpcode, TranslateMissingOpcode, TranslateMissingOpcode, TranslateMissingOpcode,
	/*0xF8*/ TranslateMissingOpcode, TranslateMissingOpcode, TranslateMissingOpcode, TranslateMissingOpcode,
	/*0xFC*/ TranslateMissingOpcode, TranslateMissingOpcode, TranslateMissingOpcode, TranslateMissingOpcode
};

/* =============================================================================================================== */

void SetOperandType(struct _river_instruction *pRiver, BYTE opId, int type) {
	unsigned char mask = 0x0C << (opId << 2);
	pRiver->opTypes &= ~mask;
	pRiver->opTypes |= type << (opId << 2);
}

void TranslateRegister(struct _exec_env *pEnv, BYTE regId, river_reg_op *op) {
	*op = regId | GetCurrentRegister(pEnv, regId);
}

void TranslateMemory(struct _exec_env *pEnv, BYTE mod, BYTE rm, river_mem_op *op) {
	if (0x04 == rm) {
		//handle sib
	}
	else if ((0x00 == mod) && (0x05 == rm)) {
		//handle standalone disp32
	} else {
		op->scale = 1;
		op->index = 0;
		op->base = GetCurrentRegister(pEnv, rm);
	}

	if (0x01 == mod) {
		//handle disp8
	} else if (0x02 == mod) {
		//handle disp32
	}
}

DWORD TranslateMissingOperands(struct _exec_env *pEnv, BYTE *px86, struct _river_instruction *pRiver) {
	__asm int 3;
	return 0;
}

DWORD TranslateSlashReg0Operands(struct _exec_env *pEnv, BYTE *px86, struct _river_instruction *pRiver) {
	BYTE reg = (px86[1] >> 3) & 0x7;
	BYTE opType;

	TranslateRegister(pEnv, reg, &pRiver->operands.regOps[0]);
}

DWORD TranslateSlashReg1Operands(struct _exec_env *pEnv, BYTE *px86, struct _river_instruction *pRiver) {
	BYTE reg = (px86[1] >> 3) & 0x7;
	BYTE opType;

	TranslateRegister(pEnv, reg, &pRiver->operands.regOps[1]);
}

//DWORD TranslateMem


DWORD TranslatePlusRegOperands(struct _exec_env *pEnv, BYTE *px86, struct _river_instruction *pRiver) {
	TranslateRegister(pEnv, *px86 - pRiver->opCode, &pRiver->operands.regOps[0]);
}

DWORD TranslateRegModRMOperands(struct _exec_env *pEnv, BYTE *px86, struct _river_instruction *pRiver) {
	BYTE bModRM = px86[1];
	BYTE bMod = bModRM >> 6;
	BYTE bRM =  bModRM & 0x07;

	TranslateSlashReg0Operands(pEnv, px86, pRiver);

	if (bMod == 0x03) {
		SetOperandType(pRiver, 1, RIVER_OP_REG);
		TranslateRegister(pEnv, bRM, &pRiver->operands.regOps[0]);
	} else {
		SetOperandType(pRiver, 1, RIVER_OP_MEM);
		TranslateMemory(pEnv, bMod, bRM, &pRiver->operands.memOp);
	}
}

x86TranslationFunction TranslateOperands0x00[] = {
	/*0x00*/ TranslateMissingOperands, TranslateMissingOperands, TranslateMissingOperands, TranslateMissingOperands,
	/*0x04*/ TranslateMissingOperands, TranslateMissingOperands, TranslateMissingOperands, TranslateMissingOperands,
	/*0x08*/ TranslateMissingOperands, TranslateMissingOperands, TranslateMissingOperands, TranslateMissingOperands,
	/*0x0C*/ TranslateMissingOperands, TranslateMissingOperands, TranslateMissingOperands, TranslateMissingOperands,

	/*0x10*/ TranslateMissingOperands, TranslateMissingOperands, TranslateMissingOperands, TranslateMissingOperands,
	/*0x14*/ TranslateMissingOperands, TranslateMissingOperands, TranslateMissingOperands, TranslateMissingOperands,
	/*0x18*/ TranslateMissingOperands, TranslateMissingOperands, TranslateMissingOperands, TranslateMissingOperands,
	/*0x1C*/ TranslateMissingOperands, TranslateMissingOperands, TranslateMissingOperands, TranslateMissingOperands,

	/*0x20*/ TranslateMissingOperands, TranslateMissingOperands, TranslateMissingOperands, TranslateMissingOperands,
	/*0x24*/ TranslateMissingOperands, TranslateMissingOperands, TranslateMissingOperands, TranslateMissingOperands,
	/*0x28*/ TranslateMissingOperands, TranslateMissingOperands, TranslateMissingOperands, TranslateMissingOperands,
	/*0x2C*/ TranslateMissingOperands, TranslateMissingOperands, TranslateMissingOperands, TranslateMissingOperands,

	/*0x30*/ TranslateMissingOperands, TranslateMissingOperands, TranslateMissingOperands, TranslateMissingOperands,
	/*0x34*/ TranslateMissingOperands, TranslateMissingOperands, TranslateMissingOperands, TranslateMissingOperands,
	/*0x38*/ TranslateMissingOperands, TranslateMissingOperands, TranslateMissingOperands, TranslateMissingOperands,
	/*0x3C*/ TranslateMissingOperands, TranslateMissingOperands, TranslateMissingOperands, TranslateMissingOperands,

	/*0x40*/ TranslateMissingOperands, TranslateMissingOperands, TranslateMissingOperands, TranslateMissingOperands,
	/*0x44*/ TranslateMissingOperands, TranslateMissingOperands, TranslateMissingOperands, TranslateMissingOperands,
	/*0x48*/ TranslateMissingOperands, TranslateMissingOperands, TranslateMissingOperands, TranslateMissingOperands,
	/*0x4C*/ TranslateMissingOperands, TranslateMissingOperands, TranslateMissingOperands, TranslateMissingOperands,

	/*0x50*/ TranslatePlusRegOperands, TranslatePlusRegOperands, TranslatePlusRegOperands, TranslatePlusRegOperands,
	/*0x54*/ TranslatePlusRegOperands, TranslatePlusRegOperands, TranslatePlusRegOperands, TranslatePlusRegOperands,
	/*0x58*/ TranslateMissingOperands, TranslateMissingOperands, TranslateMissingOperands, TranslateMissingOperands,
	/*0x5C*/ TranslateMissingOperands, TranslateMissingOperands, TranslateMissingOperands, TranslateMissingOperands,

	/*0x60*/ TranslateMissingOperands, TranslateMissingOperands, TranslateMissingOperands, TranslateMissingOperands,
	/*0x64*/ TranslateMissingOperands, TranslateMissingOperands, TranslateMissingOperands, TranslateMissingOperands,
	/*0x68*/ TranslateMissingOperands, TranslateMissingOperands, TranslateMissingOperands, TranslateMissingOperands,
	/*0x6C*/ TranslateMissingOperands, TranslateMissingOperands, TranslateMissingOperands, TranslateMissingOperands,

	/*0x70*/ TranslateMissingOperands, TranslateMissingOperands, TranslateMissingOperands, TranslateMissingOperands,
	/*0x74*/ TranslateMissingOperands, TranslateMissingOperands, TranslateMissingOperands, TranslateMissingOperands,
	/*0x78*/ TranslateMissingOperands, TranslateMissingOperands, TranslateMissingOperands, TranslateMissingOperands,
	/*0x7C*/ TranslateMissingOperands, TranslateMissingOperands, TranslateMissingOperands, TranslateMissingOperands,

	/*0x80*/ TranslateMissingOperands, TranslateMissingOperands, TranslateMissingOperands, TranslateMissingOperands,
	/*0x84*/ TranslateMissingOperands, TranslateMissingOperands, TranslateMissingOperands, TranslateMissingOperands,
	/*0x88*/ TranslateMissingOperands, TranslateMissingOperands, TranslateMissingOperands, TranslateRegModRMOperands,
	/*0x8C*/ TranslateMissingOperands, TranslateMissingOperands, TranslateMissingOperands, TranslateMissingOperands,

	/*0x90*/ TranslateMissingOperands, TranslateMissingOperands, TranslateMissingOperands, TranslateMissingOperands,
	/*0x94*/ TranslateMissingOperands, TranslateMissingOperands, TranslateMissingOperands, TranslateMissingOperands,
	/*0x98*/ TranslateMissingOperands, TranslateMissingOperands, TranslateMissingOperands, TranslateMissingOperands,
	/*0x9C*/ TranslateMissingOperands, TranslateMissingOperands, TranslateMissingOperands, TranslateMissingOperands,

	/*0xA0*/ TranslateMissingOperands, TranslateMissingOperands, TranslateMissingOperands, TranslateMissingOperands,
	/*0xA4*/ TranslateMissingOperands, TranslateMissingOperands, TranslateMissingOperands, TranslateMissingOperands,
	/*0xA8*/ TranslateMissingOperands, TranslateMissingOperands, TranslateMissingOperands, TranslateMissingOperands,
	/*0xAC*/ TranslateMissingOperands, TranslateMissingOperands, TranslateMissingOperands, TranslateMissingOperands,

	/*0xB0*/ TranslateMissingOperands, TranslateMissingOperands, TranslateMissingOperands, TranslateMissingOperands,
	/*0xB4*/ TranslateMissingOperands, TranslateMissingOperands, TranslateMissingOperands, TranslateMissingOperands,
	/*0xB8*/ TranslateMissingOperands, TranslateMissingOperands, TranslateMissingOperands, TranslateMissingOperands,
	/*0xBC*/ TranslateMissingOperands, TranslateMissingOperands, TranslateMissingOperands, TranslateMissingOperands,

	/*0xC0*/ TranslateMissingOperands, TranslateMissingOperands, TranslateMissingOperands, TranslateMissingOperands,
	/*0xC4*/ TranslateMissingOperands, TranslateMissingOperands, TranslateMissingOperands, TranslateMissingOperands,
	/*0xC8*/ TranslateMissingOperands, TranslateMissingOperands, TranslateMissingOperands, TranslateMissingOperands,
	/*0xCC*/ TranslateMissingOperands, TranslateMissingOperands, TranslateMissingOperands, TranslateMissingOperands,

	/*0xD0*/ TranslateMissingOperands, TranslateMissingOperands, TranslateMissingOperands, TranslateMissingOperands,
	/*0xD4*/ TranslateMissingOperands, TranslateMissingOperands, TranslateMissingOperands, TranslateMissingOperands,
	/*0xD8*/ TranslateMissingOperands, TranslateMissingOperands, TranslateMissingOperands, TranslateMissingOperands,
	/*0xDC*/ TranslateMissingOperands, TranslateMissingOperands, TranslateMissingOperands, TranslateMissingOperands,

	/*0xE0*/ TranslateMissingOperands, TranslateMissingOperands, TranslateMissingOperands, TranslateMissingOperands,
	/*0xE4*/ TranslateMissingOperands, TranslateMissingOperands, TranslateMissingOperands, TranslateMissingOperands,
	/*0xE8*/ TranslateMissingOperands, TranslateMissingOperands, TranslateMissingOperands, TranslateMissingOperands,
	/*0xEC*/ TranslateMissingOperands, TranslateMissingOperands, TranslateMissingOperands, TranslateMissingOperands,

	/*0xF0*/ TranslateMissingOperands, TranslateMissingOperands, TranslateMissingOperands, TranslateMissingOperands,
	/*0xF4*/ TranslateMissingOperands, TranslateMissingOperands, TranslateMissingOperands, TranslateMissingOperands,
	/*0xF8*/ TranslateMissingOperands, TranslateMissingOperands, TranslateMissingOperands, TranslateMissingOperands,
	/*0xFC*/ TranslateMissingOperands, TranslateMissingOperands, TranslateMissingOperands, TranslateMissingOperands
};



DWORD Translate(struct _exec_env *pEnv, BYTE *px86, struct _river_instruction *pRiver) {
	//skip prefixes for now

	x86TranslationFunction *translateOpcode = TranslateOpcode0x00;
	x86TranslationFunction *translateExt = TranslateExt0x00;
	x86TranslationFunction *translateOperands = TranslateOperands0x00;

	if (0x0F == *px86) {
		*translateOpcode = TranslateOpcode0x0F;
		*translateExt = TranslateExt0x0F;
		*translateOperands = TranslateOperands0x0F;
		px86++;
	}

	translateOpcode[*px86](pEnv, px86, pRiver);
	translateExt[*px86](pEnv, px86, pRiver);
	translateOperands[*px86](pEnv, px86, pRiver);
}
