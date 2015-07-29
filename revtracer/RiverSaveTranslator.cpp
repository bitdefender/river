#include "RiverSaveTranslator.h"
#include "CodeGen.h"

void RiverSaveTranslator::CopyInstruction(RiverInstruction &rOut, const RiverInstruction &rIn) {
	memcpy(&rOut, &rIn, sizeof(rOut));

	if (RIVER_OPTYPE_MEM == RIVER_OPTYPE(rIn.opTypes[0])) {
		rOut.operands[0].asAddress = codegen->CloneAddress(*rIn.operands[0].asAddress, rIn.modifiers);
	}

	if (RIVER_OPTYPE_MEM == RIVER_OPTYPE(rIn.opTypes[1])) {
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
	rOut->modifiers = 0;
	rOut->family = RIVER_FAMILY_RIVEROP;
	rOut->specifiers = 0; // maybe
	rOut->unusedRegisters = RIVER_UNUSED_ALL;

	rOut->opTypes[0] = rOut->opTypes[1] = rOut->opTypes[2] = rOut->opTypes[3] = RIVER_OPTYPE_NONE;
}

void RiverSaveTranslator::MakeSaveReg(RiverInstruction *rOut, const RiverRegister &reg, unsigned short familyFlag) {
	//unsigned char rg = (reg.name & 0x07) | RIVER_OPSIZE_32;

	rOut->opCode = 0x50; // | (reg.name & 0x07); //PUSH
	rOut->modifiers = 0;
	rOut->family = RIVER_FAMILY_RIVEROP | familyFlag;
	rOut->specifiers = 0;

	rOut->opTypes[0] = RIVER_OPTYPE_REG | RIVER_OPSIZE_32;
	rOut->operands[0].asRegister.versioned = codegen->GetPrevReg(reg.name);

	if (RIVER_REG_xSP == GetFundamentalRegister(reg.name)) {
		rOut->family |= RIVER_FAMILY_ORIG_xSP;
	}

	rOut->opTypes[1] = rOut->opTypes[2] = rOut->opTypes[3] = RIVER_OPTYPE_NONE;
	rOut->TrackUnusedRegisters();
}

void RiverSaveTranslator::MakeSaveMem(RiverInstruction *rOut, const RiverAddress &mem, unsigned short familyFlag, const RiverInstruction &rIn) {
	if (mem.type == 0) {
		MakeSaveReg(rOut, mem.base, familyFlag);
	} else {
		rOut->opCode = 0xFF; // PUSH (ext + 6)
		rOut->modifiers = 0;
		rOut->family = RIVER_FAMILY_RIVEROP | familyFlag | ((rIn.specifiers & RIVER_SPEC_MODIFIES_xSP) ? RIVER_FAMILY_ORIG_xSP : 0);
		rOut->subOpCode = 0x06;

		rOut->opTypes[0] = RIVER_OPTYPE_MEM | RIVER_OPSIZE_32;
		rOut->operands[0].asAddress = codegen->CloneAddress(mem, 0); // FIXME: needs te kind of address to be cloned
		rOut->operands[0].asAddress->modRM &= 0xC7;
		rOut->operands[0].asAddress->modRM |= rOut->subOpCode << 3;
		rOut->opTypes[1] = rOut->opTypes[2] = rOut->opTypes[3] = RIVER_OPTYPE_NONE;

		rOut->PromoteModifiers();
		rOut->TrackUnusedRegisters();
	}
}

void RiverSaveTranslator::MakeSaveMemOffset(RiverInstruction *rOut, const RiverAddress &mem, int offset, unsigned short auxFlags, const RiverInstruction &rIn) {
	if (mem.type == 0) {
		__asm int 3;
	}
	else {
		RiverAddress32 tMem;
		memcpy(&tMem, &mem, sizeof(tMem));

		if (tMem.type & RIVER_ADDR_DISP8) {
			tMem.disp.d32 = tMem.disp.d8;
			tMem.type &= ~RIVER_ADDR_DISP8;
			tMem.type |= RIVER_ADDR_DISP | RIVER_ADDR_DIRTY;
		}

		if (0 == (tMem.type & RIVER_ADDR_DISP)) {
			tMem.disp.d32 = 0;
			tMem.type |= RIVER_ADDR_DISP | RIVER_ADDR_DIRTY;
		}

		tMem.disp.d32 += offset;
		MakeSaveMem(rOut, mem, auxFlags, rIn);
	}
}

void RiverSaveTranslator::MakeAddNoFlagsRegImm8(RiverInstruction *rOut, const RiverRegister &reg, unsigned char offset, unsigned short familyFlag) {
	unsigned char rg = (reg.name & 0x07) | RIVER_OPSIZE_32;

	rOut->opCode = 0x83;
	rOut->subOpCode = 0;
	rOut->modifiers = 0;
	rOut->family = RIVER_FAMILY_RIVEROP | familyFlag;
	rOut->specifiers = 0;

	rOut->opTypes[0] = RIVER_OPTYPE_REG | RIVER_OPSIZE_32;
	rOut->operands[0].asRegister.versioned = codegen->GetCurrentReg(rg);

	rOut->opTypes[1] = RIVER_OPTYPE_IMM | RIVER_OPSIZE_8;
	rOut->operands[1].asImm8 = offset;

	rOut->opTypes[2] = rOut->opTypes[3] = RIVER_OPTYPE_NONE;

	rOut->TrackUnusedRegisters();
}

void RiverSaveTranslator::MakeSubNoFlagsRegImm8(RiverInstruction *rOut, const RiverRegister &reg, unsigned char offset, unsigned short familyFlag) {
	unsigned char rg = (reg.name & 0x07) | RIVER_OPSIZE_32;

	rOut->opCode = 0x83;
	rOut->subOpCode = 5;
	rOut->modifiers = 0;
	rOut->family = RIVER_FAMILY_RIVEROP | familyFlag;
	rOut->specifiers = 0;

	rOut->opTypes[0] = RIVER_OPTYPE_REG | RIVER_OPSIZE_32;
	rOut->operands[0].asRegister.versioned = codegen->GetCurrentReg(rg);

	rOut->opTypes[1] = RIVER_OPTYPE_IMM | RIVER_OPSIZE_8;
	rOut->operands[1].asImm8 = offset;

	rOut->opTypes[2] = rOut->opTypes[3] = RIVER_OPTYPE_NONE;

	rOut->TrackUnusedRegisters();
}


void RiverSaveTranslator::MakeSaveOp(RiverInstruction *rOut, unsigned char opType, const RiverOperand &op, const RiverInstruction &rIn) {
	switch (RIVER_OPTYPE(opType)) {
	case RIVER_OPTYPE_NONE:
	case RIVER_OPTYPE_IMM:
		__asm int 3;
		break;
	case RIVER_OPTYPE_REG:
		MakeSaveReg(rOut, op.asRegister, 0);
		break;
	case RIVER_OPTYPE_MEM:
		MakeSaveMem(rOut, *op.asAddress, 0, rIn);
		break;
	}
}

void RiverSaveTranslator::MakeSaveAtxSP(RiverInstruction *rOut, const RiverInstruction &rIn) {
	RiverAddress32 rTmp;

	rTmp.scaleAndSegment = 0;
	rTmp.type = RIVER_ADDR_BASE | RIVER_ADDR_DISP8;
	rTmp.base.versioned = codegen->GetPrevReg(RIVER_REG_xSP); // Here lies the original xSP
	rTmp.index.versioned = RIVER_REG_NONE;
	rTmp.disp.d8 = 0xFC;

	rTmp.modRM = 0x70; // actually save xAX (because xSP is used for other stuff)

	MakeSaveMem(rOut, rTmp, RIVER_FAMILY_ORIG_xSP, rIn);
}

void RiverSaveTranslator::SaveOperands(RiverInstruction *rOut, const RiverInstruction &rIn, DWORD &instrCount) {
	if (RIVER_SPEC_MODIFIES_FLG & rIn.specifiers) {
		MakeSaveFlags(rOut);
		instrCount++;
		rOut++;
	}

	for (int i = 3; i >= 0; --i) {
		if (RIVER_SPEC_MODIFIES_OP(i) & rIn.specifiers) {
			MakeSaveOp(rOut, rIn.opTypes[i], rIn.operands[i], rIn);
			instrCount++;
			rOut++;
		}
	}

	CopyInstruction(*rOut, rIn);
	instrCount++;
}

/* Opcode translators */

void RiverSaveTranslator::TranslateUnk(RiverInstruction *rOut, const RiverInstruction &rIn, DWORD &instrCount) {
	__asm int 3;
}

void RiverSaveTranslator::TranslateDefault(RiverInstruction *rOut, const RiverInstruction &rIn, DWORD &instrCount) {
	/*if (RIVER_SPEC_MODIFIES_xSP & rIn.specifiers) {
		__asm int 3;
	}*/

	SaveOperands(rOut, rIn, instrCount);
}

void RiverSaveTranslator::TranslateSaveCPUID(RiverInstruction *rOut, const RiverInstruction &rIn, DWORD &instrCount) {
	RiverRegister tmpReg;

	tmpReg.name = RIVER_REG_xAX;
	MakeSaveReg(rOut, tmpReg, 0);
	instrCount++;
	rOut++;

	tmpReg.name = RIVER_REG_xCX;
	MakeSaveReg(rOut, tmpReg, 0);
	instrCount++;
	rOut++;

	tmpReg.name = RIVER_REG_xDX;
	MakeSaveReg(rOut, tmpReg, 0);
	instrCount++;
	rOut++;

	tmpReg.name = RIVER_REG_xBX;
	MakeSaveReg(rOut, tmpReg, 0);
	instrCount++;
	rOut++;
}

void RiverSaveTranslator::TranslateSaveCMPXCHG8B(RiverInstruction *rOut, const RiverInstruction &rIn, DWORD &instrCount) {
	RiverRegister tmpReg; 

	MakeSaveFlags(rOut);
	instrCount++;
	rOut++;

	tmpReg.name = RIVER_REG_xAX;
	MakeSaveReg(rOut, tmpReg, 0);
	instrCount++;
	rOut++;

	tmpReg.name = RIVER_REG_xDX;
	MakeSaveReg(rOut, tmpReg, 0);
	instrCount++;
	rOut++;

	MakeSaveMem(rOut, *(rIn.operands[0].asAddress), 0, rIn);
	instrCount++;
	rOut++;

	MakeSaveMemOffset(rOut, *(rIn.operands[0].asAddress), 0x04, 0, rIn);
	instrCount++;
	rOut++;
}

/* =========================================== */
/* Translation table                           */
/* =========================================== */

RiverSaveTranslator::TranslateOpcodeFunc RiverSaveTranslator::translateOpcodes[2][0x100] = {
	{
		/*0x00*/&RiverSaveTranslator::TranslateDefault, &RiverSaveTranslator::TranslateDefault, &RiverSaveTranslator::TranslateDefault, &RiverSaveTranslator::TranslateDefault,
		/*0x04*/&RiverSaveTranslator::TranslateDefault, &RiverSaveTranslator::TranslateDefault, &RiverSaveTranslator::TranslateDefault, &RiverSaveTranslator::TranslateUnk,
		/*0x08*/&RiverSaveTranslator::TranslateDefault, &RiverSaveTranslator::TranslateDefault, &RiverSaveTranslator::TranslateDefault, &RiverSaveTranslator::TranslateDefault,
		/*0x0C*/&RiverSaveTranslator::TranslateDefault, &RiverSaveTranslator::TranslateDefault, &RiverSaveTranslator::TranslateUnk, &RiverSaveTranslator::TranslateUnk,

		/*0x10*/&RiverSaveTranslator::TranslateDefault, &RiverSaveTranslator::TranslateDefault, &RiverSaveTranslator::TranslateDefault, &RiverSaveTranslator::TranslateDefault,
		/*0x14*/&RiverSaveTranslator::TranslateDefault, &RiverSaveTranslator::TranslateDefault, &RiverSaveTranslator::TranslateDefault, &RiverSaveTranslator::TranslateUnk,
		/*0x18*/&RiverSaveTranslator::TranslateDefault, &RiverSaveTranslator::TranslateDefault, &RiverSaveTranslator::TranslateDefault, &RiverSaveTranslator::TranslateDefault,
		/*0x1C*/&RiverSaveTranslator::TranslateDefault, &RiverSaveTranslator::TranslateDefault, &RiverSaveTranslator::TranslateUnk, &RiverSaveTranslator::TranslateUnk,

		/*0x20*/&RiverSaveTranslator::TranslateDefault, &RiverSaveTranslator::TranslateDefault, &RiverSaveTranslator::TranslateDefault, &RiverSaveTranslator::TranslateDefault,
		/*0x24*/&RiverSaveTranslator::TranslateDefault, &RiverSaveTranslator::TranslateDefault, &RiverSaveTranslator::TranslateUnk, &RiverSaveTranslator::TranslateUnk,
		/*0x28*/&RiverSaveTranslator::TranslateDefault, &RiverSaveTranslator::TranslateDefault, &RiverSaveTranslator::TranslateDefault, &RiverSaveTranslator::TranslateDefault,
		/*0x2C*/&RiverSaveTranslator::TranslateDefault, &RiverSaveTranslator::TranslateDefault, &RiverSaveTranslator::TranslateUnk, &RiverSaveTranslator::TranslateUnk,

		/*0x30*/&RiverSaveTranslator::TranslateDefault, &RiverSaveTranslator::TranslateDefault, &RiverSaveTranslator::TranslateDefault, &RiverSaveTranslator::TranslateDefault,
		/*0x34*/&RiverSaveTranslator::TranslateDefault, &RiverSaveTranslator::TranslateDefault, &RiverSaveTranslator::TranslateUnk, &RiverSaveTranslator::TranslateUnk,
		/*0x38*/&RiverSaveTranslator::TranslateDefault, &RiverSaveTranslator::TranslateDefault, &RiverSaveTranslator::TranslateDefault, &RiverSaveTranslator::TranslateDefault,
		/*0x3C*/&RiverSaveTranslator::TranslateDefault, &RiverSaveTranslator::TranslateDefault, &RiverSaveTranslator::TranslateUnk, &RiverSaveTranslator::TranslateUnk,

		/*0x40*/&RiverSaveTranslator::TranslateDefault, &RiverSaveTranslator::TranslateDefault, &RiverSaveTranslator::TranslateDefault, &RiverSaveTranslator::TranslateDefault,
		/*0x44*/&RiverSaveTranslator::TranslateDefault, &RiverSaveTranslator::TranslateDefault, &RiverSaveTranslator::TranslateDefault, &RiverSaveTranslator::TranslateDefault,
		/*0x48*/&RiverSaveTranslator::TranslateDefault, &RiverSaveTranslator::TranslateDefault, &RiverSaveTranslator::TranslateDefault, &RiverSaveTranslator::TranslateDefault,
		/*0x4C*/&RiverSaveTranslator::TranslateDefault, &RiverSaveTranslator::TranslateDefault, &RiverSaveTranslator::TranslateDefault, &RiverSaveTranslator::TranslateDefault,

		/*0x50*/&RiverSaveTranslator::TranslateDefault, &RiverSaveTranslator::TranslateDefault, &RiverSaveTranslator::TranslateDefault, &RiverSaveTranslator::TranslateDefault,
		/*0x54*/&RiverSaveTranslator::TranslateDefault, &RiverSaveTranslator::TranslateDefault, &RiverSaveTranslator::TranslateDefault, &RiverSaveTranslator::TranslateDefault,
		/*0x58*/&RiverSaveTranslator::TranslateDefault, &RiverSaveTranslator::TranslateDefault, &RiverSaveTranslator::TranslateDefault, &RiverSaveTranslator::TranslateDefault,
		/*0x5C*/&RiverSaveTranslator::TranslateDefault, &RiverSaveTranslator::TranslateDefault, &RiverSaveTranslator::TranslateDefault, &RiverSaveTranslator::TranslateDefault,

		/*0x60*/&RiverSaveTranslator::TranslateUnk, &RiverSaveTranslator::TranslateUnk, &RiverSaveTranslator::TranslateUnk, &RiverSaveTranslator::TranslateUnk,
		/*0x64*/&RiverSaveTranslator::TranslateUnk, &RiverSaveTranslator::TranslateUnk, &RiverSaveTranslator::TranslateUnk, &RiverSaveTranslator::TranslateUnk,
		/*0x68*/&RiverSaveTranslator::TranslateDefault, &RiverSaveTranslator::TranslateDefault, &RiverSaveTranslator::TranslateDefault, &RiverSaveTranslator::TranslateDefault,
		/*0x6C*/&RiverSaveTranslator::TranslateUnk, &RiverSaveTranslator::TranslateUnk, &RiverSaveTranslator::TranslateUnk, &RiverSaveTranslator::TranslateUnk,

		/*0x70*/&RiverSaveTranslator::TranslateDefault, &RiverSaveTranslator::TranslateDefault, &RiverSaveTranslator::TranslateDefault, &RiverSaveTranslator::TranslateDefault,
		/*0x74*/&RiverSaveTranslator::TranslateDefault, &RiverSaveTranslator::TranslateDefault, &RiverSaveTranslator::TranslateDefault, &RiverSaveTranslator::TranslateDefault,
		/*0x78*/&RiverSaveTranslator::TranslateDefault, &RiverSaveTranslator::TranslateDefault, &RiverSaveTranslator::TranslateDefault, &RiverSaveTranslator::TranslateDefault,
		/*0x7C*/&RiverSaveTranslator::TranslateDefault, &RiverSaveTranslator::TranslateDefault, &RiverSaveTranslator::TranslateDefault, &RiverSaveTranslator::TranslateDefault,

		/*0x80*/&RiverSaveTranslator::TranslateDefault, &RiverSaveTranslator::TranslateDefault, &RiverSaveTranslator::TranslateUnk, &RiverSaveTranslator::TranslateDefault,
		/*0x84*/&RiverSaveTranslator::TranslateDefault, &RiverSaveTranslator::TranslateDefault, &RiverSaveTranslator::TranslateDefault, &RiverSaveTranslator::TranslateDefault,
		/*0x88*/&RiverSaveTranslator::TranslateDefault, &RiverSaveTranslator::TranslateDefault, &RiverSaveTranslator::TranslateDefault, &RiverSaveTranslator::TranslateDefault,
		/*0x8C*/&RiverSaveTranslator::TranslateUnk, &RiverSaveTranslator::TranslateDefault, &RiverSaveTranslator::TranslateUnk, &RiverSaveTranslator::TranslateUnk,

		/*0x90*/&RiverSaveTranslator::TranslateDefault, &RiverSaveTranslator::TranslateDefault, &RiverSaveTranslator::TranslateDefault, &RiverSaveTranslator::TranslateDefault,
		/*0x94*/&RiverSaveTranslator::TranslateDefault, &RiverSaveTranslator::TranslateDefault, &RiverSaveTranslator::TranslateDefault, &RiverSaveTranslator::TranslateDefault,
		/*0x98*/&RiverSaveTranslator::TranslateDefault, &RiverSaveTranslator::TranslateDefault, &RiverSaveTranslator::TranslateDefault, &RiverSaveTranslator::TranslateDefault,
		/*0x9C*/&RiverSaveTranslator::TranslateDefault, &RiverSaveTranslator::TranslateDefault, &RiverSaveTranslator::TranslateDefault, &RiverSaveTranslator::TranslateDefault,

		/*0xA0*/&RiverSaveTranslator::TranslateDefault, &RiverSaveTranslator::TranslateDefault, &RiverSaveTranslator::TranslateDefault, &RiverSaveTranslator::TranslateDefault,
		/*0xA4*/&RiverSaveTranslator::TranslateRep<&RiverSaveTranslator::TranslateDefault>, &RiverSaveTranslator::TranslateRep<&RiverSaveTranslator::TranslateDefault>, &RiverSaveTranslator::TranslateRep<&RiverSaveTranslator::TranslateDefault>, &RiverSaveTranslator::TranslateRep<&RiverSaveTranslator::TranslateDefault>,
		/*0xA8*/&RiverSaveTranslator::TranslateDefault, &RiverSaveTranslator::TranslateDefault, &RiverSaveTranslator::TranslateRep<&RiverSaveTranslator::TranslateDefault>, &RiverSaveTranslator::TranslateRep<&RiverSaveTranslator::TranslateDefault>,
		/*0xAC*/&RiverSaveTranslator::TranslateRep<&RiverSaveTranslator::TranslateUnk>, &RiverSaveTranslator::TranslateRep<&RiverSaveTranslator::TranslateUnk>, &RiverSaveTranslator::TranslateRep<&RiverSaveTranslator::TranslateDefault>, &RiverSaveTranslator::TranslateRep<&RiverSaveTranslator::TranslateDefault>,

		/*0xB0*/&RiverSaveTranslator::TranslateDefault, &RiverSaveTranslator::TranslateDefault, &RiverSaveTranslator::TranslateDefault, &RiverSaveTranslator::TranslateDefault,
		/*0xB4*/&RiverSaveTranslator::TranslateDefault, &RiverSaveTranslator::TranslateDefault, &RiverSaveTranslator::TranslateDefault, &RiverSaveTranslator::TranslateDefault,
		/*0xB8*/&RiverSaveTranslator::TranslateDefault, &RiverSaveTranslator::TranslateDefault, &RiverSaveTranslator::TranslateDefault, &RiverSaveTranslator::TranslateDefault,
		/*0xBC*/&RiverSaveTranslator::TranslateDefault, &RiverSaveTranslator::TranslateDefault, &RiverSaveTranslator::TranslateDefault, &RiverSaveTranslator::TranslateDefault,

		/*0xC0*/&RiverSaveTranslator::TranslateDefault, &RiverSaveTranslator::TranslateDefault, &RiverSaveTranslator::TranslateDefault, &RiverSaveTranslator::TranslateDefault,
		/*0xC4*/&RiverSaveTranslator::TranslateUnk, &RiverSaveTranslator::TranslateUnk, &RiverSaveTranslator::TranslateDefault, &RiverSaveTranslator::TranslateDefault,
		/*0xC8*/&RiverSaveTranslator::TranslateUnk, &RiverSaveTranslator::TranslateDefault, &RiverSaveTranslator::TranslateUnk, &RiverSaveTranslator::TranslateUnk,
		/*0xCC*/&RiverSaveTranslator::TranslateUnk, &RiverSaveTranslator::TranslateUnk, &RiverSaveTranslator::TranslateUnk, &RiverSaveTranslator::TranslateUnk,

		/*0xD0*/&RiverSaveTranslator::TranslateDefault, &RiverSaveTranslator::TranslateDefault, &RiverSaveTranslator::TranslateDefault, &RiverSaveTranslator::TranslateDefault,
		/*0xD4*/&RiverSaveTranslator::TranslateUnk, &RiverSaveTranslator::TranslateUnk, &RiverSaveTranslator::TranslateUnk, &RiverSaveTranslator::TranslateUnk,
		/*0xD8*/&RiverSaveTranslator::TranslateUnk, &RiverSaveTranslator::TranslateUnk, &RiverSaveTranslator::TranslateUnk, &RiverSaveTranslator::TranslateUnk,
		/*0xDC*/&RiverSaveTranslator::TranslateUnk, &RiverSaveTranslator::TranslateUnk, &RiverSaveTranslator::TranslateUnk, &RiverSaveTranslator::TranslateUnk,

		/*0xE0*/&RiverSaveTranslator::TranslateUnk, &RiverSaveTranslator::TranslateUnk, &RiverSaveTranslator::TranslateUnk, &RiverSaveTranslator::TranslateUnk,
		/*0xE4*/&RiverSaveTranslator::TranslateUnk, &RiverSaveTranslator::TranslateUnk, &RiverSaveTranslator::TranslateUnk, &RiverSaveTranslator::TranslateUnk,
		/*0xE8*/&RiverSaveTranslator::TranslateDefault, &RiverSaveTranslator::SaveOperands, &RiverSaveTranslator::SaveOperands, &RiverSaveTranslator::SaveOperands,
		/*0xEC*/&RiverSaveTranslator::TranslateUnk, &RiverSaveTranslator::TranslateUnk, &RiverSaveTranslator::TranslateUnk, &RiverSaveTranslator::TranslateUnk,

		/*0xF0*/&RiverSaveTranslator::TranslateUnk, &RiverSaveTranslator::TranslateUnk, &RiverSaveTranslator::TranslateUnk, &RiverSaveTranslator::TranslateUnk,
		/*0xF4*/&RiverSaveTranslator::TranslateUnk, &RiverSaveTranslator::TranslateUnk, &RiverSaveTranslator::TranslateDefault, &RiverSaveTranslator::TranslateSubOp<RiverSaveTranslator::translate0xF7>,
		/*0xF8*/&RiverSaveTranslator::TranslateUnk, &RiverSaveTranslator::TranslateUnk, &RiverSaveTranslator::TranslateUnk, &RiverSaveTranslator::TranslateUnk,
		/*0xFC*/&RiverSaveTranslator::TranslateDefault, &RiverSaveTranslator::TranslateDefault, &RiverSaveTranslator::TranslateDefault, &RiverSaveTranslator::TranslateSubOp<RiverSaveTranslator::translate0xFF>
	}, {
		/*0x00*/&RiverSaveTranslator::TranslateUnk, &RiverSaveTranslator::TranslateUnk, &RiverSaveTranslator::TranslateDefault, &RiverSaveTranslator::TranslateDefault,
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

		/*0x30*/&RiverSaveTranslator::TranslateUnk, &RiverSaveTranslator::TranslateDefault, &RiverSaveTranslator::TranslateUnk, &RiverSaveTranslator::TranslateUnk,
		/*0x34*/&RiverSaveTranslator::TranslateDefault, &RiverSaveTranslator::TranslateUnk, &RiverSaveTranslator::TranslateUnk, &RiverSaveTranslator::TranslateUnk,
		/*0x38*/&RiverSaveTranslator::TranslateUnk, &RiverSaveTranslator::TranslateUnk, &RiverSaveTranslator::TranslateUnk, &RiverSaveTranslator::TranslateUnk,
		/*0x3C*/&RiverSaveTranslator::TranslateUnk, &RiverSaveTranslator::TranslateUnk, &RiverSaveTranslator::TranslateUnk, &RiverSaveTranslator::TranslateUnk,

		/*0x40*/&RiverSaveTranslator::TranslateDefault, &RiverSaveTranslator::TranslateDefault, &RiverSaveTranslator::TranslateDefault, &RiverSaveTranslator::TranslateDefault,
		/*0x44*/&RiverSaveTranslator::TranslateDefault, &RiverSaveTranslator::TranslateDefault, &RiverSaveTranslator::TranslateDefault, &RiverSaveTranslator::TranslateDefault,
		/*0x48*/&RiverSaveTranslator::TranslateDefault, &RiverSaveTranslator::TranslateDefault, &RiverSaveTranslator::TranslateDefault, &RiverSaveTranslator::TranslateDefault,
		/*0x4C*/&RiverSaveTranslator::TranslateDefault, &RiverSaveTranslator::TranslateDefault, &RiverSaveTranslator::TranslateDefault, &RiverSaveTranslator::TranslateDefault,

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

		/*0x80*/&RiverSaveTranslator::TranslateDefault, &RiverSaveTranslator::TranslateDefault, &RiverSaveTranslator::TranslateDefault, &RiverSaveTranslator::TranslateDefault,
		/*0x84*/&RiverSaveTranslator::TranslateDefault, &RiverSaveTranslator::TranslateDefault, &RiverSaveTranslator::TranslateDefault, &RiverSaveTranslator::TranslateDefault,
		/*0x88*/&RiverSaveTranslator::TranslateDefault, &RiverSaveTranslator::TranslateDefault, &RiverSaveTranslator::TranslateDefault, &RiverSaveTranslator::TranslateDefault,
		/*0x8C*/&RiverSaveTranslator::TranslateDefault, &RiverSaveTranslator::TranslateDefault, &RiverSaveTranslator::TranslateDefault, &RiverSaveTranslator::TranslateDefault,

		/*0x90*/&RiverSaveTranslator::TranslateDefault, &RiverSaveTranslator::TranslateDefault, &RiverSaveTranslator::TranslateDefault, &RiverSaveTranslator::TranslateDefault,
		/*0x94*/&RiverSaveTranslator::TranslateDefault, &RiverSaveTranslator::TranslateDefault, &RiverSaveTranslator::TranslateDefault, &RiverSaveTranslator::TranslateDefault,
		/*0x98*/&RiverSaveTranslator::TranslateDefault, &RiverSaveTranslator::TranslateDefault, &RiverSaveTranslator::TranslateDefault, &RiverSaveTranslator::TranslateDefault,
		/*0x9C*/&RiverSaveTranslator::TranslateDefault, &RiverSaveTranslator::TranslateDefault, &RiverSaveTranslator::TranslateDefault, &RiverSaveTranslator::TranslateDefault,

		/*0xA0*/&RiverSaveTranslator::TranslateUnk, &RiverSaveTranslator::TranslateUnk, &RiverSaveTranslator::TranslateSaveCPUID, &RiverSaveTranslator::TranslateUnk,
		/*0xA4*/&RiverSaveTranslator::TranslateDefault, &RiverSaveTranslator::TranslateDefault, &RiverSaveTranslator::TranslateUnk, &RiverSaveTranslator::TranslateUnk,
		/*0xA8*/&RiverSaveTranslator::TranslateUnk, &RiverSaveTranslator::TranslateUnk, &RiverSaveTranslator::TranslateUnk, &RiverSaveTranslator::TranslateUnk,
		/*0xAC*/&RiverSaveTranslator::TranslateDefault, &RiverSaveTranslator::TranslateDefault, &RiverSaveTranslator::TranslateUnk, &RiverSaveTranslator::TranslateDefault,

		/*0xB0*/&RiverSaveTranslator::TranslateDefault, &RiverSaveTranslator::TranslateDefault, &RiverSaveTranslator::TranslateUnk, &RiverSaveTranslator::TranslateUnk,
		/*0xB4*/&RiverSaveTranslator::TranslateUnk, &RiverSaveTranslator::TranslateUnk, &RiverSaveTranslator::TranslateDefault, &RiverSaveTranslator::TranslateDefault,
		/*0xB8*/&RiverSaveTranslator::TranslateUnk, &RiverSaveTranslator::TranslateUnk, &RiverSaveTranslator::TranslateDefault, &RiverSaveTranslator::TranslateUnk,
		/*0xBC*/&RiverSaveTranslator::TranslateDefault, &RiverSaveTranslator::TranslateDefault, &RiverSaveTranslator::TranslateDefault, &RiverSaveTranslator::TranslateDefault,

		/*0xC0*/&RiverSaveTranslator::TranslateDefault, &RiverSaveTranslator::TranslateDefault, &RiverSaveTranslator::TranslateUnk, &RiverSaveTranslator::TranslateUnk,
		/*0xC4*/&RiverSaveTranslator::TranslateUnk, &RiverSaveTranslator::TranslateUnk, &RiverSaveTranslator::TranslateUnk, &RiverSaveTranslator::TranslateSubOp<RiverSaveTranslator::translate0x0FC7>,
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

RiverSaveTranslator::TranslateOpcodeFunc RiverSaveTranslator::translate0xF7[8] = {
	&RiverSaveTranslator::TranslateDefault, 
	&RiverSaveTranslator::TranslateDefault, 
	&RiverSaveTranslator::TranslateDefault, 
	&RiverSaveTranslator::TranslateDefault,
	&RiverSaveTranslator::TranslateDefault,
	&RiverSaveTranslator::TranslateDefault,
	&RiverSaveTranslator::TranslateDefault,
	&RiverSaveTranslator::TranslateDefault
};


RiverSaveTranslator::TranslateOpcodeFunc RiverSaveTranslator::translate0xFF[8] = {
	&RiverSaveTranslator::TranslateDefault, 
	&RiverSaveTranslator::TranslateDefault, 
	&RiverSaveTranslator::TranslateDefault,
	&RiverSaveTranslator::TranslateDefault,
	&RiverSaveTranslator::TranslateDefault, 
	&RiverSaveTranslator::TranslateDefault, 
	&RiverSaveTranslator::TranslateDefault,
	&RiverSaveTranslator::TranslateUnk
};

RiverSaveTranslator::TranslateOpcodeFunc RiverSaveTranslator::translate0x0FC7[8] = {
	&RiverSaveTranslator::TranslateUnk, 
	&RiverSaveTranslator::TranslateSaveCMPXCHG8B, 
	&RiverSaveTranslator::TranslateUnk, 
	&RiverSaveTranslator::TranslateUnk,
	&RiverSaveTranslator::TranslateUnk, 
	&RiverSaveTranslator::TranslateUnk, 
	&RiverSaveTranslator::TranslateUnk, 
	&RiverSaveTranslator::TranslateUnk
};
