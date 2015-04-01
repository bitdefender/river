#include "RiverSaveTranslator.h"
#include "CodeGen.h"

void RiverSaveTranslator::CopyInstruction(RiverInstruction &rOut, const RiverInstruction &rIn) {
	memcpy(&rOut, &rIn, sizeof(rOut));

	if ((RIVER_OPTYPE_NONE != rIn.opTypes[0]) && (RIVER_OPTYPE_MEM & rIn.opTypes[0])) {
		rOut.operands[0].asAddress = codegen->CloneAddress(*rIn.operands[0].asAddress, rIn.modifiers);
	}

	if ((RIVER_OPTYPE_NONE != rIn.opTypes[1]) && (RIVER_OPTYPE_MEM & rIn.opTypes[1])) {
		rOut.operands[1].asAddress = codegen->CloneAddress(*rIn.operands[1].asAddress, rIn.modifiers);
	}
}

bool RiverSaveTranslator::Init(RiverCodeGen *cg) {
	codegen = cg;
	return true;
}

void RiverSaveTranslator::Translate(const RiverInstruction &rIn, RiverInstruction *rOut, DWORD &instrCount) {
	DWORD dwTable = (RIVER_MODIFIER_EXT & rIn.modifiers) ? 1 : 0;

	(this->*translateOpcodes[dwTable][rIn.opCode])(rOut, rIn, instrCount);
}

/* =========================================== */
/* Translation helpers                         */
/* =========================================== */

void RiverSaveTranslator::MakeSaveFlags(RiverInstruction *rOut) {
	rOut->opCode = 0x9C; // PUSHF
	rOut->modifiers = RIVER_MODIFIER_RIVEROP;
	rOut->specifiers = 0; // maybe

	rOut->opTypes[0] = rOut->opTypes[1] = RIVER_OPTYPE_NONE;
}

void RiverSaveTranslator::MakeSaveReg(RiverInstruction *rOut, const RiverRegister &reg, unsigned short auxFlags) {
	//unsigned char rg = (reg.name & 0x07) | RIVER_OPSIZE_32;

	rOut->opCode = 0x50; //PUSH
	rOut->modifiers = RIVER_MODIFIER_RIVEROP | auxFlags;
	rOut->specifiers = 0;

	rOut->opTypes[0] = RIVER_OPTYPE_REG | RIVER_OPSIZE_32;
	rOut->operands[0].asRegister.versioned = codegen->GetPrevReg(reg.name);

	rOut->opTypes[1] = RIVER_OPTYPE_NONE;
}

void RiverSaveTranslator::MakeSaveMem(RiverInstruction *rOut, const RiverAddress &mem, unsigned short auxFlags) {
	if (mem.type == 0) {
		MakeSaveReg(rOut, mem.base, auxFlags);
	}
	else {
		rOut->opCode = 0xFF; // PUSH (ext + 6)
		rOut->modifiers = RIVER_MODIFIER_RIVEROP | auxFlags;
		rOut->subOpCode = 0x06;

		rOut->opTypes[0] = RIVER_OPTYPE_MEM | RIVER_OPSIZE_32;
		rOut->operands[0].asAddress = codegen->CloneAddress(mem, 0); // FIXME: needs te kind of address to be cloned
		rOut->operands[0].asAddress->modRM &= 0xC7;
		rOut->operands[0].asAddress->modRM |= rOut->subOpCode << 3;
		rOut->opTypes[1] = RIVER_OPTYPE_NONE;
	}
}

void RiverSaveTranslator::MakeAddNoFlagsRegImm8(RiverInstruction *rOut, const RiverRegister &reg, unsigned char offset, unsigned short auxFlags) {
	unsigned char rg = (reg.name & 0x07) | RIVER_OPSIZE_32;

	rOut->opCode = 0x83;
	rOut->subOpCode = 0;
	rOut->modifiers = RIVER_MODIFIER_RIVEROP | auxFlags;
	rOut->specifiers = 0;

	rOut->opTypes[0] = RIVER_OPTYPE_REG | RIVER_OPSIZE_32;
	rOut->operands[0].asRegister.versioned = codegen->GetCurrentReg(rg);

	rOut->opTypes[1] = RIVER_OPTYPE_IMM | RIVER_OPSIZE_8;
	rOut->operands[1].asImm8 = offset;
}

void RiverSaveTranslator::MakeSubNoFlagsRegImm8(RiverInstruction *rOut, const RiverRegister &reg, unsigned char offset, unsigned short auxFlags) {
	unsigned char rg = (reg.name & 0x07) | RIVER_OPSIZE_32;

	rOut->opCode = 0x83;
	rOut->subOpCode = 5;
	rOut->modifiers = RIVER_MODIFIER_RIVEROP | auxFlags;
	rOut->specifiers = 0;

	rOut->opTypes[0] = RIVER_OPTYPE_REG | RIVER_OPSIZE_32;
	rOut->operands[0].asRegister.versioned = codegen->GetCurrentReg(rg);

	rOut->opTypes[1] = RIVER_OPTYPE_IMM | RIVER_OPSIZE_8;
	rOut->operands[1].asImm8 = offset;
}


void RiverSaveTranslator::MakeSaveOp(RiverInstruction *rOut, unsigned char opType, const RiverOperand &op) {
	switch (opType) {
	case RIVER_OPTYPE_NONE:
	case RIVER_OPTYPE_IMM:
		__asm int 3;
		break;
	case RIVER_OPTYPE_REG:
		MakeSaveReg(rOut, op.asRegister, 0);
		break;
	case RIVER_OPTYPE_MEM:
		MakeSaveMem(rOut, *op.asAddress, 0);
		break;
	}
}

void RiverSaveTranslator::MakeSaveAtxSP(RiverInstruction *rOut) {
	RiverAddress32 rTmp;

	rTmp.type = RIVER_ADDR_BASE | RIVER_ADDR_DISP8;
	rTmp.base.versioned = codegen->GetPrevReg(RIVER_REG_xSP); // Here lies the original xSP
	rTmp.index.versioned = RIVER_REG_NONE;
	rTmp.disp.d8 = 0xFC;

	rTmp.modRM = 0x70; // actually save xAX (because xSP is used for other stuff

	MakeSaveMem(rOut, rTmp, RIVER_MODIFIER_ORIG_xSP);
}

void RiverSaveTranslator::SaveOperands(RiverInstruction *rOut, const RiverInstruction &rIn, DWORD &instrCount) {
	if (RIVER_SPEC_MODIFIES_FLG & rIn.specifiers) {
		MakeSaveFlags(rOut);
		instrCount++;
		rOut++;
	}

	if (RIVER_SPEC_MODIFIES_OP2 & rIn.specifiers) {
		MakeSaveOp(rOut, rIn.opTypes[1], rIn.operands[1]);
		instrCount++; 
		rOut++;
	}

	if (RIVER_SPEC_MODIFIES_OP1 & rIn.specifiers) {
		MakeSaveOp(rOut, rIn.opTypes[0], rIn.operands[0]);
		instrCount++; 
		rOut++;
	}

	CopyInstruction(*rOut, rIn);
	instrCount++;
}

/* Opcode translators */

void RiverSaveTranslator::TranslateUnk(RiverInstruction *rOut, const RiverInstruction &rIn, DWORD &instrCount) {
	__asm int 3;
}

void RiverSaveTranslator::TranslateDefault(RiverInstruction *rOut, const RiverInstruction &rIn, DWORD &instrCount) {
	if (RIVER_SPEC_MODIFIES_xSP & rIn.specifiers) {
		__asm int 3;
	}

	SaveOperands(rOut, rIn, instrCount);
}

void RiverSaveTranslator::TranslatePush(RiverInstruction *rOut, const RiverInstruction &rIn, DWORD &instrCount) {
	RiverRegister tmpReg;

	MakeSaveAtxSP(rOut);
	instrCount++;
	rOut++;

	tmpReg.versioned = RIVER_REG_xSP;
	MakeSaveReg(rOut, tmpReg, RIVER_MODIFIER_ORIG_xSP);
	instrCount++;
	rOut++;

	tmpReg.versioned = RIVER_REG_xSP;
	MakeSubNoFlagsRegImm8(rOut, tmpReg, 0x04, RIVER_MODIFIER_ORIG_xSP | RIVER_MODIFIER_METAOP);
	instrCount++;
	rOut++;

	SaveOperands(rOut, rIn, instrCount);
}

void RiverSaveTranslator::TranslatePop(RiverInstruction *rOut, const RiverInstruction &rIn, DWORD &instrCount) {
	RiverRegister tmpReg;

	tmpReg.name = RIVER_REG_xSP;
	MakeSaveReg(rOut, tmpReg, RIVER_MODIFIER_ORIG_xSP);
	instrCount++;
	rOut++;

	tmpReg.versioned = RIVER_REG_xSP;
	MakeAddNoFlagsRegImm8(rOut, tmpReg, 0x04, RIVER_MODIFIER_ORIG_xSP | RIVER_MODIFIER_METAOP);
	instrCount++;
	rOut++;

	SaveOperands(rOut, rIn, instrCount);
}

/* =========================================== */
/* Translation table                           */
/* =========================================== */

RiverSaveTranslator::TranslateOpcodeFunc RiverSaveTranslator::translateOpcodes[2][0x100] = {
	{
		/*0x00*/&RiverSaveTranslator::TranslateUnk, &RiverSaveTranslator::TranslateUnk, &RiverSaveTranslator::TranslateUnk, &RiverSaveTranslator::TranslateUnk,
		/*0x04*/&RiverSaveTranslator::TranslateUnk, &RiverSaveTranslator::TranslateUnk, &RiverSaveTranslator::TranslateUnk, &RiverSaveTranslator::TranslateUnk,
		/*0x08*/&RiverSaveTranslator::TranslateUnk, &RiverSaveTranslator::TranslateUnk, &RiverSaveTranslator::TranslateUnk, &RiverSaveTranslator::TranslateUnk,
		/*0x0C*/&RiverSaveTranslator::TranslateUnk, &RiverSaveTranslator::TranslateUnk, &RiverSaveTranslator::TranslateUnk, &RiverSaveTranslator::TranslateUnk,

		/*0x10*/&RiverSaveTranslator::TranslateUnk, &RiverSaveTranslator::TranslateUnk, &RiverSaveTranslator::TranslateUnk, &RiverSaveTranslator::TranslateUnk,
		/*0x14*/&RiverSaveTranslator::TranslateUnk, &RiverSaveTranslator::TranslateUnk, &RiverSaveTranslator::TranslateUnk, &RiverSaveTranslator::TranslateUnk,
		/*0x18*/&RiverSaveTranslator::TranslateUnk, &RiverSaveTranslator::TranslateUnk, &RiverSaveTranslator::TranslateUnk, &RiverSaveTranslator::TranslateUnk,
		/*0x1C*/&RiverSaveTranslator::TranslateUnk, &RiverSaveTranslator::TranslateUnk, &RiverSaveTranslator::TranslateUnk, &RiverSaveTranslator::TranslateUnk,

		/*0x20*/&RiverSaveTranslator::TranslateUnk, &RiverSaveTranslator::TranslateUnk, &RiverSaveTranslator::TranslateUnk, &RiverSaveTranslator::TranslateUnk,
		/*0x24*/&RiverSaveTranslator::TranslateUnk, &RiverSaveTranslator::TranslateUnk, &RiverSaveTranslator::TranslateUnk, &RiverSaveTranslator::TranslateUnk,
		/*0x28*/&RiverSaveTranslator::TranslateUnk, &RiverSaveTranslator::TranslateUnk, &RiverSaveTranslator::TranslateUnk, &RiverSaveTranslator::TranslateUnk,
		/*0x2C*/&RiverSaveTranslator::TranslateUnk, &RiverSaveTranslator::TranslateUnk, &RiverSaveTranslator::TranslateUnk, &RiverSaveTranslator::TranslateUnk,

		/*0x30*/&RiverSaveTranslator::TranslateUnk, &RiverSaveTranslator::TranslateUnk, &RiverSaveTranslator::TranslateUnk, &RiverSaveTranslator::TranslateDefault,
		/*0x34*/&RiverSaveTranslator::TranslateUnk, &RiverSaveTranslator::TranslateUnk, &RiverSaveTranslator::TranslateUnk, &RiverSaveTranslator::TranslateUnk,
		/*0x38*/&RiverSaveTranslator::TranslateUnk, &RiverSaveTranslator::TranslateUnk, &RiverSaveTranslator::TranslateUnk, &RiverSaveTranslator::TranslateDefault,
		/*0x3C*/&RiverSaveTranslator::TranslateUnk, &RiverSaveTranslator::TranslateUnk, &RiverSaveTranslator::TranslateUnk, &RiverSaveTranslator::TranslateUnk,

		/*0x40*/&RiverSaveTranslator::TranslateDefault, &RiverSaveTranslator::TranslateDefault, &RiverSaveTranslator::TranslateDefault, &RiverSaveTranslator::TranslateDefault,
		/*0x44*/&RiverSaveTranslator::TranslateDefault, &RiverSaveTranslator::TranslateDefault, &RiverSaveTranslator::TranslateDefault, &RiverSaveTranslator::TranslateDefault,
		/*0x48*/&RiverSaveTranslator::TranslateDefault, &RiverSaveTranslator::TranslateDefault, &RiverSaveTranslator::TranslateDefault, &RiverSaveTranslator::TranslateDefault,
		/*0x4C*/&RiverSaveTranslator::TranslateDefault, &RiverSaveTranslator::TranslateDefault, &RiverSaveTranslator::TranslateDefault, &RiverSaveTranslator::TranslateDefault,

		/*0x50*/&RiverSaveTranslator::TranslatePush, &RiverSaveTranslator::TranslatePush, &RiverSaveTranslator::TranslatePush, &RiverSaveTranslator::TranslatePush,
		/*0x54*/&RiverSaveTranslator::TranslatePush, &RiverSaveTranslator::TranslatePush, &RiverSaveTranslator::TranslatePush, &RiverSaveTranslator::TranslatePush,
		/*0x58*/&RiverSaveTranslator::TranslatePop, &RiverSaveTranslator::TranslatePop, &RiverSaveTranslator::TranslatePop, &RiverSaveTranslator::TranslatePop,
		/*0x5C*/&RiverSaveTranslator::TranslatePop, &RiverSaveTranslator::TranslatePop, &RiverSaveTranslator::TranslatePop, &RiverSaveTranslator::TranslatePop,

		/*0x60*/&RiverSaveTranslator::TranslateUnk, &RiverSaveTranslator::TranslateUnk, &RiverSaveTranslator::TranslateUnk, &RiverSaveTranslator::TranslateUnk,
		/*0x64*/&RiverSaveTranslator::TranslateUnk, &RiverSaveTranslator::TranslateUnk, &RiverSaveTranslator::TranslateUnk, &RiverSaveTranslator::TranslateUnk,
		/*0x68*/&RiverSaveTranslator::TranslateUnk, &RiverSaveTranslator::TranslateUnk, &RiverSaveTranslator::TranslateUnk, &RiverSaveTranslator::TranslateUnk,
		/*0x6C*/&RiverSaveTranslator::TranslateUnk, &RiverSaveTranslator::TranslateUnk, &RiverSaveTranslator::TranslateUnk, &RiverSaveTranslator::TranslateUnk,

		/*0x70*/&RiverSaveTranslator::TranslateDefault, &RiverSaveTranslator::TranslateDefault, &RiverSaveTranslator::TranslateDefault, &RiverSaveTranslator::TranslateDefault,
		/*0x74*/&RiverSaveTranslator::TranslateDefault, &RiverSaveTranslator::TranslateDefault, &RiverSaveTranslator::TranslateDefault, &RiverSaveTranslator::TranslateDefault,
		/*0x78*/&RiverSaveTranslator::TranslateDefault, &RiverSaveTranslator::TranslateDefault, &RiverSaveTranslator::TranslateDefault, &RiverSaveTranslator::TranslateDefault,
		/*0x7C*/&RiverSaveTranslator::TranslateDefault, &RiverSaveTranslator::TranslateDefault, &RiverSaveTranslator::TranslateDefault, &RiverSaveTranslator::TranslateDefault,

		/*0x80*/&RiverSaveTranslator::TranslateUnk, &RiverSaveTranslator::TranslateUnk, &RiverSaveTranslator::TranslateUnk, &RiverSaveTranslator::TranslateDefault,
		/*0x84*/&RiverSaveTranslator::TranslateUnk, &RiverSaveTranslator::TranslateUnk, &RiverSaveTranslator::TranslateUnk, &RiverSaveTranslator::TranslateUnk,
		/*0x88*/&RiverSaveTranslator::TranslateUnk, &RiverSaveTranslator::TranslateDefault, &RiverSaveTranslator::TranslateUnk, &RiverSaveTranslator::TranslateDefault,
		/*0x8C*/&RiverSaveTranslator::TranslateUnk, &RiverSaveTranslator::TranslateUnk, &RiverSaveTranslator::TranslateUnk, &RiverSaveTranslator::TranslateUnk,

		/*0x90*/&RiverSaveTranslator::TranslateUnk, &RiverSaveTranslator::TranslateUnk, &RiverSaveTranslator::TranslateUnk, &RiverSaveTranslator::TranslateUnk,
		/*0x94*/&RiverSaveTranslator::TranslateUnk, &RiverSaveTranslator::TranslateUnk, &RiverSaveTranslator::TranslateUnk, &RiverSaveTranslator::TranslateUnk,
		/*0x98*/&RiverSaveTranslator::TranslateUnk, &RiverSaveTranslator::TranslateUnk, &RiverSaveTranslator::TranslateUnk, &RiverSaveTranslator::TranslateUnk,
		/*0x9C*/&RiverSaveTranslator::TranslateUnk, &RiverSaveTranslator::TranslateUnk, &RiverSaveTranslator::TranslateUnk, &RiverSaveTranslator::TranslateUnk,

		/*0xA0*/&RiverSaveTranslator::TranslateUnk, &RiverSaveTranslator::TranslateUnk, &RiverSaveTranslator::TranslateUnk, &RiverSaveTranslator::TranslateUnk,
		/*0xA4*/&RiverSaveTranslator::TranslateUnk, &RiverSaveTranslator::TranslateUnk, &RiverSaveTranslator::TranslateUnk, &RiverSaveTranslator::TranslateUnk,
		/*0xA8*/&RiverSaveTranslator::TranslateUnk, &RiverSaveTranslator::TranslateUnk, &RiverSaveTranslator::TranslateUnk, &RiverSaveTranslator::TranslateUnk,
		/*0xAC*/&RiverSaveTranslator::TranslateUnk, &RiverSaveTranslator::TranslateUnk, &RiverSaveTranslator::TranslateUnk, &RiverSaveTranslator::TranslateUnk,

		/*0xB0*/&RiverSaveTranslator::TranslateDefault, &RiverSaveTranslator::TranslateDefault, &RiverSaveTranslator::TranslateDefault, &RiverSaveTranslator::TranslateDefault,
		/*0xB4*/&RiverSaveTranslator::TranslateDefault, &RiverSaveTranslator::TranslateDefault, &RiverSaveTranslator::TranslateDefault, &RiverSaveTranslator::TranslateDefault,
		/*0xB8*/&RiverSaveTranslator::TranslateDefault, &RiverSaveTranslator::TranslateDefault, &RiverSaveTranslator::TranslateDefault, &RiverSaveTranslator::TranslateDefault,
		/*0xBC*/&RiverSaveTranslator::TranslateDefault, &RiverSaveTranslator::TranslateDefault, &RiverSaveTranslator::TranslateDefault, &RiverSaveTranslator::TranslateDefault,

		/*0xC0*/&RiverSaveTranslator::TranslateUnk, &RiverSaveTranslator::TranslateUnk, &RiverSaveTranslator::TranslateUnk, &RiverSaveTranslator::TranslatePop,
		/*0xC4*/&RiverSaveTranslator::TranslateUnk, &RiverSaveTranslator::TranslateUnk, &RiverSaveTranslator::TranslateUnk, &RiverSaveTranslator::TranslateUnk,
		/*0xC8*/&RiverSaveTranslator::TranslateUnk, &RiverSaveTranslator::TranslateUnk, &RiverSaveTranslator::TranslateUnk, &RiverSaveTranslator::TranslateUnk,
		/*0xCC*/&RiverSaveTranslator::TranslateUnk, &RiverSaveTranslator::TranslateUnk, &RiverSaveTranslator::TranslateUnk, &RiverSaveTranslator::TranslateUnk,

		/*0xD0*/&RiverSaveTranslator::TranslateUnk, &RiverSaveTranslator::TranslateUnk, &RiverSaveTranslator::TranslateUnk, &RiverSaveTranslator::TranslateUnk,
		/*0xD4*/&RiverSaveTranslator::TranslateUnk, &RiverSaveTranslator::TranslateUnk, &RiverSaveTranslator::TranslateUnk, &RiverSaveTranslator::TranslateUnk,
		/*0xD8*/&RiverSaveTranslator::TranslateUnk, &RiverSaveTranslator::TranslateUnk, &RiverSaveTranslator::TranslateUnk, &RiverSaveTranslator::TranslateUnk,
		/*0xDC*/&RiverSaveTranslator::TranslateUnk, &RiverSaveTranslator::TranslateUnk, &RiverSaveTranslator::TranslateUnk, &RiverSaveTranslator::TranslateUnk,

		/*0xE0*/&RiverSaveTranslator::TranslateUnk, &RiverSaveTranslator::TranslateUnk, &RiverSaveTranslator::TranslateUnk, &RiverSaveTranslator::TranslateUnk,
		/*0xE4*/&RiverSaveTranslator::TranslateUnk, &RiverSaveTranslator::TranslateUnk, &RiverSaveTranslator::TranslateUnk, &RiverSaveTranslator::TranslateUnk,
		/*0xE8*/&RiverSaveTranslator::TranslateUnk, &RiverSaveTranslator::SaveOperands, &RiverSaveTranslator::TranslateUnk, &RiverSaveTranslator::TranslateUnk,
		/*0xEC*/&RiverSaveTranslator::TranslateUnk, &RiverSaveTranslator::TranslateUnk, &RiverSaveTranslator::TranslateUnk, &RiverSaveTranslator::TranslateUnk,

		/*0xF0*/&RiverSaveTranslator::TranslateUnk, &RiverSaveTranslator::TranslateUnk, &RiverSaveTranslator::TranslateUnk, &RiverSaveTranslator::TranslateUnk,
		/*0xF4*/&RiverSaveTranslator::TranslateUnk, &RiverSaveTranslator::TranslateUnk, &RiverSaveTranslator::TranslateUnk, &RiverSaveTranslator::TranslateUnk,
		/*0xF8*/&RiverSaveTranslator::TranslateUnk, &RiverSaveTranslator::TranslateUnk, &RiverSaveTranslator::TranslateUnk, &RiverSaveTranslator::TranslateUnk,
		/*0xFC*/&RiverSaveTranslator::TranslateUnk, &RiverSaveTranslator::TranslateUnk, &RiverSaveTranslator::TranslateUnk, &RiverSaveTranslator::TranslateUnk
	}, {
		/*0x00*/&RiverSaveTranslator::TranslateUnk, &RiverSaveTranslator::TranslateUnk, &RiverSaveTranslator::TranslateUnk, &RiverSaveTranslator::TranslateUnk,
		/*0x04*/&RiverSaveTranslator::TranslateUnk, &RiverSaveTranslator::TranslateUnk, &RiverSaveTranslator::TranslateUnk, &RiverSaveTranslator::TranslateUnk,
		/*0x08*/&RiverSaveTranslator::TranslateUnk, &RiverSaveTranslator::TranslateUnk, &RiverSaveTranslator::TranslateUnk, &RiverSaveTranslator::TranslateUnk,
		/*0x0C*/&RiverSaveTranslator::TranslateUnk, &RiverSaveTranslator::TranslateUnk, &RiverSaveTranslator::TranslateUnk, &RiverSaveTranslator::TranslateUnk,

		/*0x10*/&RiverSaveTranslator::TranslateUnk, &RiverSaveTranslator::TranslateUnk, &RiverSaveTranslator::TranslateUnk, &RiverSaveTranslator::TranslateUnk,
		/*0x14*/&RiverSaveTranslator::TranslateUnk, &RiverSaveTranslator::TranslateUnk, &RiverSaveTranslator::TranslateUnk, &RiverSaveTranslator::TranslateUnk,
		/*0x18*/&RiverSaveTranslator::TranslateUnk, &RiverSaveTranslator::TranslateUnk, &RiverSaveTranslator::TranslateUnk, &RiverSaveTranslator::TranslateUnk,
		/*0x1C*/&RiverSaveTranslator::TranslateUnk, &RiverSaveTranslator::TranslateUnk, &RiverSaveTranslator::TranslateUnk, &RiverSaveTranslator::TranslateUnk,

		/*0x20*/&RiverSaveTranslator::TranslateUnk, &RiverSaveTranslator::TranslateUnk, &RiverSaveTranslator::TranslateUnk, &RiverSaveTranslator::TranslateUnk,
		/*0x24*/&RiverSaveTranslator::TranslateUnk, &RiverSaveTranslator::TranslateUnk, &RiverSaveTranslator::TranslateUnk, &RiverSaveTranslator::TranslateUnk,
		/*0x28*/&RiverSaveTranslator::TranslateUnk, &RiverSaveTranslator::TranslateUnk, &RiverSaveTranslator::TranslateUnk, &RiverSaveTranslator::TranslateUnk,
		/*0x2C*/&RiverSaveTranslator::TranslateUnk, &RiverSaveTranslator::TranslateUnk, &RiverSaveTranslator::TranslateUnk, &RiverSaveTranslator::TranslateUnk,

		/*0x30*/&RiverSaveTranslator::TranslateUnk, &RiverSaveTranslator::TranslateUnk, &RiverSaveTranslator::TranslateUnk, &RiverSaveTranslator::TranslateUnk,
		/*0x34*/&RiverSaveTranslator::TranslateUnk, &RiverSaveTranslator::TranslateUnk, &RiverSaveTranslator::TranslateUnk, &RiverSaveTranslator::TranslateUnk,
		/*0x38*/&RiverSaveTranslator::TranslateUnk, &RiverSaveTranslator::TranslateUnk, &RiverSaveTranslator::TranslateUnk, &RiverSaveTranslator::TranslateUnk,
		/*0x3C*/&RiverSaveTranslator::TranslateUnk, &RiverSaveTranslator::TranslateUnk, &RiverSaveTranslator::TranslateUnk, &RiverSaveTranslator::TranslateUnk,

		/*0x40*/&RiverSaveTranslator::TranslateUnk, &RiverSaveTranslator::TranslateUnk, &RiverSaveTranslator::TranslateUnk, &RiverSaveTranslator::TranslateUnk,
		/*0x44*/&RiverSaveTranslator::TranslateUnk, &RiverSaveTranslator::TranslateUnk, &RiverSaveTranslator::TranslateUnk, &RiverSaveTranslator::TranslateUnk,
		/*0x48*/&RiverSaveTranslator::TranslateUnk, &RiverSaveTranslator::TranslateUnk, &RiverSaveTranslator::TranslateUnk, &RiverSaveTranslator::TranslateUnk,
		/*0x4C*/&RiverSaveTranslator::TranslateUnk, &RiverSaveTranslator::TranslateUnk, &RiverSaveTranslator::TranslateUnk, &RiverSaveTranslator::TranslateUnk,

		/*0x50*/&RiverSaveTranslator::TranslateUnk, &RiverSaveTranslator::TranslateUnk, &RiverSaveTranslator::TranslateUnk, &RiverSaveTranslator::TranslateUnk,
		/*0x54*/&RiverSaveTranslator::TranslateUnk, &RiverSaveTranslator::TranslateUnk, &RiverSaveTranslator::TranslateUnk, &RiverSaveTranslator::TranslateUnk,
		/*0x58*/&RiverSaveTranslator::TranslateUnk, &RiverSaveTranslator::TranslateUnk, &RiverSaveTranslator::TranslateUnk, &RiverSaveTranslator::TranslateUnk,
		/*0x5C*/&RiverSaveTranslator::TranslateUnk, &RiverSaveTranslator::TranslateUnk, &RiverSaveTranslator::TranslateUnk, &RiverSaveTranslator::TranslateUnk,

		/*0x60*/&RiverSaveTranslator::TranslateUnk, &RiverSaveTranslator::TranslateUnk, &RiverSaveTranslator::TranslateUnk, &RiverSaveTranslator::TranslateUnk,
		/*0x64*/&RiverSaveTranslator::TranslateUnk, &RiverSaveTranslator::TranslateUnk, &RiverSaveTranslator::TranslateUnk, &RiverSaveTranslator::TranslateUnk,
		/*0x68*/&RiverSaveTranslator::TranslateUnk, &RiverSaveTranslator::TranslateUnk, &RiverSaveTranslator::TranslateUnk, &RiverSaveTranslator::TranslateUnk,
		/*0x6C*/&RiverSaveTranslator::TranslateUnk, &RiverSaveTranslator::TranslateUnk, &RiverSaveTranslator::TranslateUnk, &RiverSaveTranslator::TranslateUnk,

		/*0x70*/&RiverSaveTranslator::TranslateUnk, &RiverSaveTranslator::TranslateUnk, &RiverSaveTranslator::TranslateUnk, &RiverSaveTranslator::TranslateUnk,
		/*0x74*/&RiverSaveTranslator::TranslateUnk, &RiverSaveTranslator::TranslateUnk, &RiverSaveTranslator::TranslateUnk, &RiverSaveTranslator::TranslateUnk,
		/*0x78*/&RiverSaveTranslator::TranslateUnk, &RiverSaveTranslator::TranslateUnk, &RiverSaveTranslator::TranslateUnk, &RiverSaveTranslator::TranslateUnk,
		/*0x7C*/&RiverSaveTranslator::TranslateUnk, &RiverSaveTranslator::TranslateUnk, &RiverSaveTranslator::TranslateUnk, &RiverSaveTranslator::TranslateUnk,

		/*0x80*/&RiverSaveTranslator::TranslateUnk, &RiverSaveTranslator::TranslateUnk, &RiverSaveTranslator::TranslateUnk, &RiverSaveTranslator::TranslateUnk,
		/*0x84*/&RiverSaveTranslator::TranslateUnk, &RiverSaveTranslator::TranslateUnk, &RiverSaveTranslator::TranslateUnk, &RiverSaveTranslator::TranslateUnk,
		/*0x88*/&RiverSaveTranslator::TranslateUnk, &RiverSaveTranslator::TranslateUnk, &RiverSaveTranslator::TranslateUnk, &RiverSaveTranslator::TranslateUnk,
		/*0x8C*/&RiverSaveTranslator::TranslateUnk, &RiverSaveTranslator::TranslateUnk, &RiverSaveTranslator::TranslateUnk, &RiverSaveTranslator::TranslateUnk,

		/*0x90*/&RiverSaveTranslator::TranslateUnk, &RiverSaveTranslator::TranslateUnk, &RiverSaveTranslator::TranslateUnk, &RiverSaveTranslator::TranslateUnk,
		/*0x94*/&RiverSaveTranslator::TranslateUnk, &RiverSaveTranslator::TranslateUnk, &RiverSaveTranslator::TranslateUnk, &RiverSaveTranslator::TranslateUnk,
		/*0x98*/&RiverSaveTranslator::TranslateUnk, &RiverSaveTranslator::TranslateUnk, &RiverSaveTranslator::TranslateUnk, &RiverSaveTranslator::TranslateUnk,
		/*0x9C*/&RiverSaveTranslator::TranslateUnk, &RiverSaveTranslator::TranslateUnk, &RiverSaveTranslator::TranslateUnk, &RiverSaveTranslator::TranslateUnk,

		/*0xA0*/&RiverSaveTranslator::TranslateUnk, &RiverSaveTranslator::TranslateUnk, &RiverSaveTranslator::TranslateUnk, &RiverSaveTranslator::TranslateUnk,
		/*0xA4*/&RiverSaveTranslator::TranslateUnk, &RiverSaveTranslator::TranslateUnk, &RiverSaveTranslator::TranslateUnk, &RiverSaveTranslator::TranslateUnk,
		/*0xA8*/&RiverSaveTranslator::TranslateUnk, &RiverSaveTranslator::TranslateUnk, &RiverSaveTranslator::TranslateUnk, &RiverSaveTranslator::TranslateUnk,
		/*0xAC*/&RiverSaveTranslator::TranslateUnk, &RiverSaveTranslator::TranslateUnk, &RiverSaveTranslator::TranslateUnk, &RiverSaveTranslator::TranslateUnk,

		/*0xB0*/&RiverSaveTranslator::TranslateUnk, &RiverSaveTranslator::TranslateUnk, &RiverSaveTranslator::TranslateUnk, &RiverSaveTranslator::TranslateUnk,
		/*0xB4*/&RiverSaveTranslator::TranslateUnk, &RiverSaveTranslator::TranslateUnk, &RiverSaveTranslator::TranslateUnk, &RiverSaveTranslator::TranslateUnk,
		/*0xB8*/&RiverSaveTranslator::TranslateUnk, &RiverSaveTranslator::TranslateUnk, &RiverSaveTranslator::TranslateUnk, &RiverSaveTranslator::TranslateUnk,
		/*0xBC*/&RiverSaveTranslator::TranslateUnk, &RiverSaveTranslator::TranslateUnk, &RiverSaveTranslator::TranslateUnk, &RiverSaveTranslator::TranslateUnk,

		/*0xC0*/&RiverSaveTranslator::TranslateUnk, &RiverSaveTranslator::TranslateUnk, &RiverSaveTranslator::TranslateUnk, &RiverSaveTranslator::TranslateUnk,
		/*0xC4*/&RiverSaveTranslator::TranslateUnk, &RiverSaveTranslator::TranslateUnk, &RiverSaveTranslator::TranslateUnk, &RiverSaveTranslator::TranslateUnk,
		/*0xC8*/&RiverSaveTranslator::TranslateUnk, &RiverSaveTranslator::TranslateUnk, &RiverSaveTranslator::TranslateUnk, &RiverSaveTranslator::TranslateUnk,
		/*0xCC*/&RiverSaveTranslator::TranslateUnk, &RiverSaveTranslator::TranslateUnk, &RiverSaveTranslator::TranslateUnk, &RiverSaveTranslator::TranslateUnk,

		/*0xD0*/&RiverSaveTranslator::TranslateUnk, &RiverSaveTranslator::TranslateUnk, &RiverSaveTranslator::TranslateUnk, &RiverSaveTranslator::TranslateUnk,
		/*0xD4*/&RiverSaveTranslator::TranslateUnk, &RiverSaveTranslator::TranslateUnk, &RiverSaveTranslator::TranslateUnk, &RiverSaveTranslator::TranslateUnk,
		/*0xD8*/&RiverSaveTranslator::TranslateUnk, &RiverSaveTranslator::TranslateUnk, &RiverSaveTranslator::TranslateUnk, &RiverSaveTranslator::TranslateUnk,
		/*0xDC*/&RiverSaveTranslator::TranslateUnk, &RiverSaveTranslator::TranslateUnk, &RiverSaveTranslator::TranslateUnk, &RiverSaveTranslator::TranslateUnk,

		/*0xE0*/&RiverSaveTranslator::TranslateUnk, &RiverSaveTranslator::TranslateUnk, &RiverSaveTranslator::TranslateUnk, &RiverSaveTranslator::TranslateUnk,
		/*0xE4*/&RiverSaveTranslator::TranslateUnk, &RiverSaveTranslator::TranslateUnk, &RiverSaveTranslator::TranslateUnk, &RiverSaveTranslator::TranslateUnk,
		/*0xE8*/&RiverSaveTranslator::TranslateUnk, &RiverSaveTranslator::TranslateUnk, &RiverSaveTranslator::TranslateUnk, &RiverSaveTranslator::TranslateUnk,
		/*0xEC*/&RiverSaveTranslator::TranslateUnk, &RiverSaveTranslator::TranslateUnk, &RiverSaveTranslator::TranslateUnk, &RiverSaveTranslator::TranslateUnk,

		/*0xF0*/&RiverSaveTranslator::TranslateUnk, &RiverSaveTranslator::TranslateUnk, &RiverSaveTranslator::TranslateUnk, &RiverSaveTranslator::TranslateUnk,
		/*0xF4*/&RiverSaveTranslator::TranslateUnk, &RiverSaveTranslator::TranslateUnk, &RiverSaveTranslator::TranslateUnk, &RiverSaveTranslator::TranslateUnk,
		/*0xF8*/&RiverSaveTranslator::TranslateUnk, &RiverSaveTranslator::TranslateUnk, &RiverSaveTranslator::TranslateUnk, &RiverSaveTranslator::TranslateUnk,
		/*0xFC*/&RiverSaveTranslator::TranslateUnk, &RiverSaveTranslator::TranslateUnk, &RiverSaveTranslator::TranslateUnk, &RiverSaveTranslator::TranslateUnk
	}
};