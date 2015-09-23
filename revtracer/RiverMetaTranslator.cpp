#include "RiverMetaTranslator.h"
#include "CodeGen.h"

void RiverMetaTranslator::CopyInstruction(RiverInstruction &rOut, const RiverInstruction &rIn) {
	memcpy(&rOut, &rIn, sizeof(rOut));

	for (int i = 0; i < 4; ++i) {
		if (RIVER_OPTYPE_MEM == RIVER_OPTYPE(rIn.opTypes[i])) {
			rOut.operands[i].asAddress = codegen->CloneAddress(*rIn.operands[i].asAddress, rIn.modifiers);
		}
	}
}

void RiverMetaTranslator::MakeAddNoFlagsRegImm8(RiverInstruction *rOut, const RiverRegister &reg, unsigned char offset, BYTE family) {
	unsigned char rg = (reg.name & 0x07) | RIVER_OPSIZE_32;

	rOut->opCode = 0x83;
	rOut->subOpCode = 0;
	rOut->modifiers = 0;
	rOut->family = family;
	rOut->specifiers = RIVER_SPEC_IGNORES_FLG | RIVER_SPEC_MODIFIES_OP1;

	rOut->modFlags = rOut->testFlags = 0;

	rOut->opTypes[0] = RIVER_OPTYPE_REG | RIVER_OPSIZE_32;
	rOut->operands[0].asRegister.versioned = codegen->GetCurrentReg(rg);

	rOut->opTypes[1] = RIVER_OPTYPE_IMM | RIVER_OPSIZE_8;
	rOut->operands[1].asImm8 = offset;

	rOut->opTypes[2] = rOut->opTypes[3] = RIVER_OPTYPE_NONE;

	rOut->TrackUnusedRegisters();
}

void RiverMetaTranslator::MakeSubNoFlagsRegImm8(RiverInstruction *rOut, const RiverRegister &reg, unsigned char offset, BYTE family) {
	unsigned char rg = (reg.name & 0x07) | RIVER_OPSIZE_32;

	rOut->opCode = 0x83;
	rOut->subOpCode = 5;
	rOut->modifiers = 0;
	rOut->family = family;
	rOut->specifiers = RIVER_SPEC_IGNORES_FLG | RIVER_SPEC_MODIFIES_OP1;

	rOut->modFlags = rOut->testFlags = 0;

	rOut->opTypes[0] = RIVER_OPTYPE_REG | RIVER_OPSIZE_32;
	rOut->operands[0].asRegister.versioned = codegen->GetCurrentReg(rg);

	rOut->opTypes[1] = RIVER_OPTYPE_IMM | RIVER_OPSIZE_8;
	rOut->operands[1].asImm8 = offset;

	rOut->opTypes[2] = rOut->opTypes[3] = RIVER_OPTYPE_NONE;

	rOut->TrackUnusedRegisters();
}

void RiverMetaTranslator::MakeMovRegMem32(RiverInstruction *rOut, const RiverRegister &reg, const RiverAddress &mem, BYTE family) {
	rOut->opCode = 0x8B;
	rOut->subOpCode = 0;
	rOut->modifiers = 0;
	rOut->family = family;
	rOut->specifiers = RIVER_SPEC_IGNORES_FLG | RIVER_SPEC_IGNORES_OP1 | RIVER_SPEC_MODIFIES_OP1;

	rOut->modFlags = rOut->testFlags = 0;

	rOut->opTypes[0] = RIVER_OPTYPE_REG | RIVER_OPSIZE_32;
	rOut->operands[0].asRegister.versioned = reg.versioned;

	rOut->opTypes[1] = RIVER_OPTYPE_MEM | RIVER_OPSIZE_32;
	rOut->operands[1].asAddress = codegen->CloneAddress(mem, 0);

	rOut->opTypes[2] = rOut->opTypes[3] = RIVER_OPTYPE_NONE;
}

void RiverMetaTranslator::MakeMovMemReg32(RiverInstruction *rOut, const RiverAddress &mem, const RiverRegister &reg, BYTE family) {
	rOut->opCode = 0x89;
	rOut->subOpCode = 0;
	rOut->modifiers = 0;
	rOut->family = family;
	rOut->specifiers = RIVER_SPEC_IGNORES_FLG | RIVER_SPEC_IGNORES_OP1 | RIVER_SPEC_MODIFIES_OP1;

	rOut->modFlags = rOut->testFlags = 0;

	rOut->opTypes[0] = RIVER_OPTYPE_MEM | RIVER_OPSIZE_32;
	rOut->operands[0].asAddress = codegen->CloneAddress(mem, 0);

	rOut->opTypes[1] = RIVER_OPTYPE_REG | RIVER_OPSIZE_32;
	rOut->operands[1].asRegister.versioned = reg.versioned;

	rOut->opTypes[2] = rOut->opTypes[3] = RIVER_OPTYPE_NONE;
}

void RiverMetaTranslator::MakeMovMemImm32(RiverInstruction *rOut, const RiverAddress &mem, DWORD imm, BYTE family) {
	rOut->opCode = 0xC7;
	rOut->subOpCode = 0;
	rOut->modifiers = 0;
	rOut->family = family;
	rOut->specifiers = RIVER_SPEC_IGNORES_FLG | RIVER_SPEC_IGNORES_OP1 | RIVER_SPEC_MODIFIES_OP1;

	rOut->modFlags = rOut->testFlags = 0;

	rOut->opTypes[0] = RIVER_OPTYPE_MEM | RIVER_OPSIZE_32;
	rOut->operands[0].asAddress = codegen->CloneAddress(mem, 0);

	rOut->opTypes[1] = RIVER_OPTYPE_IMM | RIVER_OPSIZE_32;
	rOut->operands[1].asImm32 = imm;

	rOut->opTypes[2] = rOut->opTypes[3] = RIVER_OPTYPE_NONE;
}

void RiverMetaTranslator::MakeMovMemMem32(RiverInstruction *rOut, const RiverAddress &memd, const RiverAddress &mems, BYTE family) {
	rOut->opCode = 0xA5;
	rOut->subOpCode = 0;
	rOut->modifiers = 0;
	rOut->family = family;
	rOut->specifiers = RIVER_SPEC_IGNORES_FLG | RIVER_SPEC_IGNORES_OP1 | RIVER_SPEC_MODIFIES_OP1;

	rOut->modFlags = rOut->testFlags = 0;

	rOut->opTypes[0] = RIVER_OPTYPE_MEM | RIVER_OPSIZE_32;
	rOut->operands[0].asAddress = codegen->CloneAddress(memd, 0);

	rOut->opTypes[1] = RIVER_OPTYPE_IMM | RIVER_OPSIZE_32;
	rOut->operands[1].asAddress = codegen->CloneAddress(mems, 0);

	rOut->opTypes[2] = rOut->opTypes[3] = RIVER_OPTYPE_NONE;
}

bool RiverMetaTranslator::Init(RiverCodeGen *cg) {
	codegen = cg;
	return true;
}

void RiverMetaTranslator::Translate(const RiverInstruction &rIn, RiverInstruction *rOut, DWORD &instrCount) {
	DWORD dwTable = (RIVER_MODIFIER_EXT & rIn.modifiers) ? 1 : 0;

	if (rIn.family == RIVER_FAMILY_NATIVE) {
		(this->*translateOpcodes[dwTable][rIn.opCode])(rOut, rIn, instrCount);
	}
	else {
		CopyInstruction(rOut[0], rIn);
		instrCount++;
	}
}

void RiverMetaTranslator::TranslateUnk(RiverInstruction *rOut, const RiverInstruction &rIn, DWORD &instrCount) {
	__asm int 3;
}

void RiverMetaTranslator::TranslateDefault(RiverInstruction *rOut, const RiverInstruction &rIn, DWORD &instrCount) {
	CopyInstruction(rOut[0], rIn);
	instrCount += 1;
}

void RiverMetaTranslator::TranslatePushReg(RiverInstruction *rOut, const RiverInstruction &rIn, DWORD &instrCount) {
	MakeMovMemReg32(&rOut[0], *(rIn.operands[2].asAddress), rIn.operands[0].asRegister, RIVER_FAMILY_PREMETAOP);
	MakeSubNoFlagsRegImm8(&rOut[1], rIn.operands[1].asRegister, 0x04, RIVER_FAMILY_PREMETAOP);
	CopyInstruction(rOut[2], rIn);
	rOut[2].family |= RIVER_FAMILY_FLAG_METAPROCESSED;
	instrCount += 3;
}

void RiverMetaTranslator::TranslatePusha(RiverInstruction *rOut, const RiverInstruction &rIn, DWORD &instrCount) {
	RiverAddress32 rStack;
	RiverRegister rReg;

	rStack.type = RIVER_ADDR_BASE | RIVER_ADDR_DISP8 | RIVER_ADDR_DIRTY;
	rStack.base.versioned = codegen->GetPrevReg(RIVER_REG_xSP);
	
	rStack.disp.d8 = 0xFC;
	rReg.versioned = codegen->GetCurrentReg(RIVER_REG_xAX);
	MakeMovMemReg32(&rOut[0], rStack, rReg, RIVER_FAMILY_PREMETAOP);

	rStack.disp.d8 = 0xF8;
	rReg.versioned = codegen->GetCurrentReg(RIVER_REG_xCX);
	MakeMovMemReg32(&rOut[1], rStack, rReg, RIVER_FAMILY_PREMETAOP);

	rStack.disp.d8 = 0xF4;
	rReg.versioned = codegen->GetCurrentReg(RIVER_REG_xDX);
	MakeMovMemReg32(&rOut[2], rStack, rReg, RIVER_FAMILY_PREMETAOP);

	rStack.disp.d8 = 0xF0;
	rReg.versioned = codegen->GetCurrentReg(RIVER_REG_xBX);
	MakeMovMemReg32(&rOut[3], rStack, rReg, RIVER_FAMILY_PREMETAOP);

	rStack.disp.d8 = 0xEC;
	rReg.versioned = codegen->GetPrevReg(RIVER_REG_xSP);
	MakeMovMemReg32(&rOut[4], rStack, rReg, RIVER_FAMILY_PREMETAOP);

	rStack.disp.d8 = 0xE8;
	rReg.versioned = codegen->GetCurrentReg(RIVER_REG_xBP);
	MakeMovMemReg32(&rOut[5], rStack, rReg, RIVER_FAMILY_PREMETAOP);

	rStack.disp.d8 = 0xE4;
	rReg.versioned = codegen->GetCurrentReg(RIVER_REG_xSI);
	MakeMovMemReg32(&rOut[6], rStack, rReg, RIVER_FAMILY_PREMETAOP);

	rStack.disp.d8 = 0xE0;
	rReg.versioned = codegen->GetCurrentReg(RIVER_REG_xDI);
	MakeMovMemReg32(&rOut[7], rStack, rReg, RIVER_FAMILY_PREMETAOP);

	rReg.versioned = codegen->GetCurrentReg(RIVER_REG_xSP);
	MakeSubNoFlagsRegImm8(&rOut[8], rReg, 0x20, RIVER_FAMILY_PREMETAOP);

	CopyInstruction(rOut[9], rIn);
	rOut[9].family |= RIVER_FAMILY_FLAG_METAPROCESSED;
	instrCount += 10;
}

void RiverMetaTranslator::TranslatePopReg(RiverInstruction *rOut, const RiverInstruction &rIn, DWORD &instrCount) {
	MakeMovRegMem32(&rOut[0], rIn.operands[0].asRegister, *rIn.operands[2].asAddress, RIVER_FAMILY_PREMETAOP);
	MakeAddNoFlagsRegImm8(&rOut[1], rIn.operands[1].asRegister, 0x04, RIVER_FAMILY_PREMETAOP);
	
	CopyInstruction(rOut[2], rIn);
	rOut[2].family |= RIVER_FAMILY_FLAG_METAPROCESSED;
	instrCount += 3;
}

void RiverMetaTranslator::TranslatePopMem(RiverInstruction *rOut, const RiverInstruction &rIn, DWORD &instrCount) {
	MakeMovMemMem32(&rOut[0], *rIn.operands[0].asAddress, *rIn.operands[2].asAddress, RIVER_FAMILY_PREMETAOP);
	MakeAddNoFlagsRegImm8(&rOut[1], rIn.operands[1].asRegister, 0x04, RIVER_FAMILY_PREMETAOP);

	CopyInstruction(rOut[2], rIn);
	rOut[2].family |= RIVER_FAMILY_FLAG_METAPROCESSED;
	instrCount += 3;
}

void RiverMetaTranslator::TranslateCall(RiverInstruction *rOut, const RiverInstruction &rIn, DWORD &instrCount) {
	MakeMovMemImm32(&rOut[0], *(rIn.operands[3].asAddress), rIn.operands[1].asImm32, RIVER_FAMILY_PREMETAOP);
	MakeSubNoFlagsRegImm8(&rOut[1], rIn.operands[2].asRegister, 0x04, RIVER_FAMILY_PREMETAOP);
	CopyInstruction(rOut[2], rIn);
	rOut[2].family |= RIVER_FAMILY_FLAG_METAPROCESSED;
	instrCount += 3;
}

void RiverMetaTranslator::TranslateRet(RiverInstruction *rOut, const RiverInstruction &rIn, DWORD &instrCount) {
	MakeAddNoFlagsRegImm8(&rOut[0], rIn.operands[0].asRegister, 0x04, RIVER_FAMILY_PREMETAOP);
	CopyInstruction(rOut[1], rIn);
	rOut[1].family |= RIVER_FAMILY_FLAG_METAPROCESSED;
	instrCount += 2;
}

void RiverMetaTranslator::TranslateRetn(RiverInstruction *rOut, const RiverInstruction &rIn, DWORD &instrCount) {
	MakeAddNoFlagsRegImm8(&rOut[0], rIn.operands[1].asRegister, rIn.operands[0].asImm16 + 0x04, RIVER_FAMILY_PREMETAOP);
	CopyInstruction(rOut[1], rIn);
	rOut[1].family |= RIVER_FAMILY_FLAG_METAPROCESSED;
	instrCount += 2;
}

RiverMetaTranslator::TranslateOpcodeFunc RiverMetaTranslator::translate0xFFOp[8] = {
	&RiverMetaTranslator::TranslateDefault,
	&RiverMetaTranslator::TranslateDefault,
	&RiverMetaTranslator::TranslateCall,
	&RiverMetaTranslator::TranslateDefault,
	&RiverMetaTranslator::TranslateDefault,
	&RiverMetaTranslator::TranslateDefault,
	&RiverMetaTranslator::TranslateDefault,
	&RiverMetaTranslator::TranslateDefault,
};


RiverMetaTranslator::TranslateOpcodeFunc RiverMetaTranslator::translateOpcodes[2][0x100] = {
	{
		/* 0x00 */ &RiverMetaTranslator::TranslateDefault,
		/* 0x01 */ &RiverMetaTranslator::TranslateDefault,
		/* 0x02 */ &RiverMetaTranslator::TranslateDefault,
		/* 0x03 */ &RiverMetaTranslator::TranslateDefault,
		/* 0x04 */ &RiverMetaTranslator::TranslateDefault,
		/* 0x05 */ &RiverMetaTranslator::TranslateDefault,
		/* 0x06 */ &RiverMetaTranslator::TranslateDefault,
		/* 0x07 */ &RiverMetaTranslator::TranslateDefault,
		/* 0x08 */ &RiverMetaTranslator::TranslateDefault,
		/* 0x09 */ &RiverMetaTranslator::TranslateDefault,
		/* 0x0A */ &RiverMetaTranslator::TranslateDefault,
		/* 0x0B */ &RiverMetaTranslator::TranslateDefault,
		/* 0x0C */ &RiverMetaTranslator::TranslateDefault,
		/* 0x0D */ &RiverMetaTranslator::TranslateDefault,
		/* 0x0E */ &RiverMetaTranslator::TranslateDefault,
		/* 0x0F */ &RiverMetaTranslator::TranslateDefault,
		/* 0x10 */ &RiverMetaTranslator::TranslateDefault, 
		/* 0x11 */ &RiverMetaTranslator::TranslateDefault, 
		/* 0x12 */ &RiverMetaTranslator::TranslateDefault, 
		/* 0x13 */ &RiverMetaTranslator::TranslateDefault, 
		/* 0x14 */ &RiverMetaTranslator::TranslateDefault, 
		/* 0x15 */ &RiverMetaTranslator::TranslateDefault, 
		/* 0x16 */ &RiverMetaTranslator::TranslateDefault, 
		/* 0x17 */ &RiverMetaTranslator::TranslateDefault, 
		/* 0x18 */ &RiverMetaTranslator::TranslateDefault, 
		/* 0x19 */ &RiverMetaTranslator::TranslateDefault, 
		/* 0x1A */ &RiverMetaTranslator::TranslateDefault, 
		/* 0x1B */ &RiverMetaTranslator::TranslateDefault, 
		/* 0x1C */ &RiverMetaTranslator::TranslateDefault, 
		/* 0x1D */ &RiverMetaTranslator::TranslateDefault, 
		/* 0x1E */ &RiverMetaTranslator::TranslateDefault, 
		/* 0x1F */ &RiverMetaTranslator::TranslateDefault, 
		/* 0x20 */ &RiverMetaTranslator::TranslateDefault, 
		/* 0x21 */ &RiverMetaTranslator::TranslateDefault, 
		/* 0x22 */ &RiverMetaTranslator::TranslateDefault, 
		/* 0x23 */ &RiverMetaTranslator::TranslateDefault, 
		/* 0x24 */ &RiverMetaTranslator::TranslateDefault, 
		/* 0x25 */ &RiverMetaTranslator::TranslateDefault, 
		/* 0x26 */ &RiverMetaTranslator::TranslateDefault, 
		/* 0x27 */ &RiverMetaTranslator::TranslateDefault, 
		/* 0x28 */ &RiverMetaTranslator::TranslateDefault, 
		/* 0x29 */ &RiverMetaTranslator::TranslateDefault, 
		/* 0x2A */ &RiverMetaTranslator::TranslateDefault, 
		/* 0x2B */ &RiverMetaTranslator::TranslateDefault, 
		/* 0x2C */ &RiverMetaTranslator::TranslateDefault, 
		/* 0x2D */ &RiverMetaTranslator::TranslateDefault, 
		/* 0x2E */ &RiverMetaTranslator::TranslateDefault, 
		/* 0x2F */ &RiverMetaTranslator::TranslateDefault, 
		/* 0x30 */ &RiverMetaTranslator::TranslateDefault, 
		/* 0x31 */ &RiverMetaTranslator::TranslateDefault, 
		/* 0x32 */ &RiverMetaTranslator::TranslateDefault, 
		/* 0x33 */ &RiverMetaTranslator::TranslateDefault, 
		/* 0x34 */ &RiverMetaTranslator::TranslateDefault, 
		/* 0x35 */ &RiverMetaTranslator::TranslateDefault, 
		/* 0x36 */ &RiverMetaTranslator::TranslateDefault, 
		/* 0x37 */ &RiverMetaTranslator::TranslateDefault, 
		/* 0x38 */ &RiverMetaTranslator::TranslateDefault, 
		/* 0x39 */ &RiverMetaTranslator::TranslateDefault, 
		/* 0x3A */ &RiverMetaTranslator::TranslateDefault, 
		/* 0x3B */ &RiverMetaTranslator::TranslateDefault, 
		/* 0x3C */ &RiverMetaTranslator::TranslateDefault, 
		/* 0x3D */ &RiverMetaTranslator::TranslateDefault, 
		/* 0x3E */ &RiverMetaTranslator::TranslateDefault, 
		/* 0x4F */ &RiverMetaTranslator::TranslateDefault, 
		/* 0x40 */ &RiverMetaTranslator::TranslateDefault, 
		/* 0x41 */ &RiverMetaTranslator::TranslateDefault, 
		/* 0x42 */ &RiverMetaTranslator::TranslateDefault, 
		/* 0x43 */ &RiverMetaTranslator::TranslateDefault, 
		/* 0x44 */ &RiverMetaTranslator::TranslateDefault, 
		/* 0x45 */ &RiverMetaTranslator::TranslateDefault, 
		/* 0x46 */ &RiverMetaTranslator::TranslateDefault, 
		/* 0x47 */ &RiverMetaTranslator::TranslateDefault, 
		/* 0x48 */ &RiverMetaTranslator::TranslateDefault, 
		/* 0x49 */ &RiverMetaTranslator::TranslateDefault, 
		/* 0x4A */ &RiverMetaTranslator::TranslateDefault, 
		/* 0x4B */ &RiverMetaTranslator::TranslateDefault, 
		/* 0x4C */ &RiverMetaTranslator::TranslateDefault, 
		/* 0x4D */ &RiverMetaTranslator::TranslateDefault, 
		/* 0x4E */ &RiverMetaTranslator::TranslateDefault, 
		/* 0x4F */ &RiverMetaTranslator::TranslateDefault, 
		/* 0x50 */ &RiverMetaTranslator::TranslatePushReg,
		/* 0x51 */ &RiverMetaTranslator::TranslatePushReg,
		/* 0x52 */ &RiverMetaTranslator::TranslatePushReg,
		/* 0x53 */ &RiverMetaTranslator::TranslatePushReg,
		/* 0x54 */ &RiverMetaTranslator::TranslatePushReg,
		/* 0x55 */ &RiverMetaTranslator::TranslatePushReg,
		/* 0x56 */ &RiverMetaTranslator::TranslatePushReg,
		/* 0x57 */ &RiverMetaTranslator::TranslatePushReg,
		/* 0x58 */ &RiverMetaTranslator::TranslatePopReg,
		/* 0x59 */ &RiverMetaTranslator::TranslatePopReg,
		/* 0x5A */ &RiverMetaTranslator::TranslatePopReg,
		/* 0x5B */ &RiverMetaTranslator::TranslatePopReg,
		/* 0x5C */ &RiverMetaTranslator::TranslatePopReg,
		/* 0x5D */ &RiverMetaTranslator::TranslatePopReg,
		/* 0x5E */ &RiverMetaTranslator::TranslatePopReg,
		/* 0x5F */ &RiverMetaTranslator::TranslatePopReg,
		/* 0x60 */ &RiverMetaTranslator::TranslatePusha,
		/* 0x61 */ &RiverMetaTranslator::TranslateUnk,
		/* 0x62 */ &RiverMetaTranslator::TranslateUnk,
		/* 0x63 */ &RiverMetaTranslator::TranslateUnk,
		/* 0x64 */ &RiverMetaTranslator::TranslateDefault,
		/* 0x65 */ &RiverMetaTranslator::TranslateDefault,
		/* 0x66 */ &RiverMetaTranslator::TranslateDefault,
		/* 0x67 */ &RiverMetaTranslator::TranslateDefault,
		/* 0x68 */ &RiverMetaTranslator::TranslateDefault,
		/* 0x69 */ &RiverMetaTranslator::TranslateDefault,
		/* 0x6A */ &RiverMetaTranslator::TranslateDefault,
		/* 0x6B */ &RiverMetaTranslator::TranslateDefault,
		/* 0x6C */ &RiverMetaTranslator::TranslateUnk,
		/* 0x6D */ &RiverMetaTranslator::TranslateUnk,
		/* 0x6E */ &RiverMetaTranslator::TranslateUnk,
		/* 0x6F */ &RiverMetaTranslator::TranslateUnk,
		/* 0x70 */ &RiverMetaTranslator::TranslateDefault, 
		/* 0x71 */ &RiverMetaTranslator::TranslateDefault, 
		/* 0x72 */ &RiverMetaTranslator::TranslateDefault, 
		/* 0x73 */ &RiverMetaTranslator::TranslateDefault, 
		/* 0x74 */ &RiverMetaTranslator::TranslateDefault, 
		/* 0x75 */ &RiverMetaTranslator::TranslateDefault, 
		/* 0x76 */ &RiverMetaTranslator::TranslateDefault, 
		/* 0x77 */ &RiverMetaTranslator::TranslateDefault, 
		/* 0x78 */ &RiverMetaTranslator::TranslateDefault, 
		/* 0x79 */ &RiverMetaTranslator::TranslateDefault, 
		/* 0x7A */ &RiverMetaTranslator::TranslateDefault, 
		/* 0x7B */ &RiverMetaTranslator::TranslateDefault, 
		/* 0x7C */ &RiverMetaTranslator::TranslateDefault, 
		/* 0x7D */ &RiverMetaTranslator::TranslateDefault, 
		/* 0x7E */ &RiverMetaTranslator::TranslateDefault, 
		/* 0x7F */ &RiverMetaTranslator::TranslateDefault, 
		/* 0x80 */ &RiverMetaTranslator::TranslateDefault, 
		/* 0x01 */ &RiverMetaTranslator::TranslateDefault, 
		/* 0x82 */ &RiverMetaTranslator::TranslateDefault, 
		/* 0x83 */ &RiverMetaTranslator::TranslateDefault, 
		/* 0x84 */ &RiverMetaTranslator::TranslateDefault, 
		/* 0x85 */ &RiverMetaTranslator::TranslateDefault, 
		/* 0x86 */ &RiverMetaTranslator::TranslateDefault, 
		/* 0x87 */ &RiverMetaTranslator::TranslateDefault, 
		/* 0x88 */ &RiverMetaTranslator::TranslateDefault, 
		/* 0x89 */ &RiverMetaTranslator::TranslateDefault, 
		/* 0x8A */ &RiverMetaTranslator::TranslateDefault, 
		/* 0x8B */ &RiverMetaTranslator::TranslateDefault,
		/* 0x8C */ &RiverMetaTranslator::TranslateUnk,
		/* 0x8D */ &RiverMetaTranslator::TranslateDefault,
		/* 0x8E */ &RiverMetaTranslator::TranslateUnk,
		/* 0x8F */ &RiverMetaTranslator::TranslatePopMem,
		/* 0x90 */ &RiverMetaTranslator::TranslateDefault, 
		/* 0x91 */ &RiverMetaTranslator::TranslateDefault, 
		/* 0x92 */ &RiverMetaTranslator::TranslateDefault, 
		/* 0x93 */ &RiverMetaTranslator::TranslateDefault, 
		/* 0x94 */ &RiverMetaTranslator::TranslateDefault, 
		/* 0x95 */ &RiverMetaTranslator::TranslateDefault, 
		/* 0x96 */ &RiverMetaTranslator::TranslateDefault, 
		/* 0x97 */ &RiverMetaTranslator::TranslateDefault, 
		/* 0x98 */ &RiverMetaTranslator::TranslateUnk,
		/* 0x99 */ &RiverMetaTranslator::TranslateDefault,
		/* 0x9A */ &RiverMetaTranslator::TranslateUnk,
		/* 0x9B */ &RiverMetaTranslator::TranslateUnk,
		/* 0x9C */ &RiverMetaTranslator::TranslateUnk,
		/* 0x9D */ &RiverMetaTranslator::TranslateUnk,
		/* 0x9E */ &RiverMetaTranslator::TranslateUnk,
		/* 0x9F */ &RiverMetaTranslator::TranslateUnk,
		/* 0xA0 */ &RiverMetaTranslator::TranslateDefault, 
		/* 0xA1 */ &RiverMetaTranslator::TranslateDefault, 
		/* 0xA2 */ &RiverMetaTranslator::TranslateDefault, 
		/* 0xA3 */ &RiverMetaTranslator::TranslateDefault, 
		/* 0xA4 */ &RiverMetaTranslator::TranslateDefault, 
		/* 0xA5 */ &RiverMetaTranslator::TranslateDefault, 
		/* 0xA6 */ &RiverMetaTranslator::TranslateDefault, 
		/* 0xA7 */ &RiverMetaTranslator::TranslateDefault, 
		/* 0xA8 */ &RiverMetaTranslator::TranslateDefault, 
		/* 0xA9 */ &RiverMetaTranslator::TranslateDefault, 
		/* 0xAA */ &RiverMetaTranslator::TranslateDefault, 
		/* 0xAB */ &RiverMetaTranslator::TranslateDefault, 
		/* 0xAC */ &RiverMetaTranslator::TranslateUnk, 
		/* 0xAD */ &RiverMetaTranslator::TranslateUnk, 
		/* 0xAE */ &RiverMetaTranslator::TranslateDefault, 
		/* 0xAF */ &RiverMetaTranslator::TranslateDefault, 
		/* 0xB0 */ &RiverMetaTranslator::TranslateDefault, 
		/* 0xB1 */ &RiverMetaTranslator::TranslateDefault, 
		/* 0xB2 */ &RiverMetaTranslator::TranslateDefault, 
		/* 0xB3 */ &RiverMetaTranslator::TranslateDefault, 
		/* 0xB4 */ &RiverMetaTranslator::TranslateDefault, 
		/* 0xB5 */ &RiverMetaTranslator::TranslateDefault, 
		/* 0xB6 */ &RiverMetaTranslator::TranslateDefault, 
		/* 0xB7 */ &RiverMetaTranslator::TranslateDefault, 
		/* 0xB8 */ &RiverMetaTranslator::TranslateDefault, 
		/* 0xB9 */ &RiverMetaTranslator::TranslateDefault, 
		/* 0xBA */ &RiverMetaTranslator::TranslateDefault, 
		/* 0xBB */ &RiverMetaTranslator::TranslateDefault, 
		/* 0xBC */ &RiverMetaTranslator::TranslateDefault, 
		/* 0xBD */ &RiverMetaTranslator::TranslateDefault, 
		/* 0xBE */ &RiverMetaTranslator::TranslateDefault, 
		/* 0xBF */ &RiverMetaTranslator::TranslateDefault, 
		/* 0xC0 */ &RiverMetaTranslator::TranslateDefault,
		/* 0xC1 */ &RiverMetaTranslator::TranslateDefault,
		/* 0xC2 */ &RiverMetaTranslator::TranslateRetn,
		/* 0xC3 */ &RiverMetaTranslator::TranslateRet,
		/* 0xC4 */ &RiverMetaTranslator::TranslateUnk,
		/* 0xC5 */ &RiverMetaTranslator::TranslateUnk,
		/* 0xC6 */ &RiverMetaTranslator::TranslateDefault,
		/* 0xC7 */ &RiverMetaTranslator::TranslateDefault,
		/* 0xC8 */ &RiverMetaTranslator::TranslateDefault,
		/* 0xC9 */ &RiverMetaTranslator::TranslateDefault,
		/* 0xCA */ &RiverMetaTranslator::TranslateDefault,
		/* 0xCB */ &RiverMetaTranslator::TranslateDefault,
		/* 0xCC */ &RiverMetaTranslator::TranslateDefault,
		/* 0xCD */ &RiverMetaTranslator::TranslateUnk,
		/* 0xCE */ &RiverMetaTranslator::TranslateDefault,
		/* 0xCF */ &RiverMetaTranslator::TranslateDefault,
		/* 0xD0 */ &RiverMetaTranslator::TranslateDefault,
		/* 0xD1 */ &RiverMetaTranslator::TranslateDefault,
		/* 0xD2 */ &RiverMetaTranslator::TranslateDefault,
		/* 0xD3 */ &RiverMetaTranslator::TranslateDefault,
		/* 0xD4 */ &RiverMetaTranslator::TranslateUnk,
		/* 0xD5 */ &RiverMetaTranslator::TranslateUnk,
		/* 0xD6 */ &RiverMetaTranslator::TranslateUnk,
		/* 0xD7 */ &RiverMetaTranslator::TranslateUnk,
		/* 0xD8 */ &RiverMetaTranslator::TranslateUnk,
		/* 0xD9 */ &RiverMetaTranslator::TranslateUnk,
		/* 0xDA */ &RiverMetaTranslator::TranslateUnk,
		/* 0xDB */ &RiverMetaTranslator::TranslateUnk,
		/* 0xDC */ &RiverMetaTranslator::TranslateUnk,
		/* 0xDD */ &RiverMetaTranslator::TranslateUnk,
		/* 0xDE */ &RiverMetaTranslator::TranslateUnk,
		/* 0xDF */ &RiverMetaTranslator::TranslateUnk,
		/* 0xE0 */ &RiverMetaTranslator::TranslateUnk,
		/* 0xE1 */ &RiverMetaTranslator::TranslateUnk,
		/* 0xE2 */ &RiverMetaTranslator::TranslateUnk,
		/* 0xE3 */ &RiverMetaTranslator::TranslateUnk,
		/* 0xE4 */ &RiverMetaTranslator::TranslateUnk,
		/* 0xE5 */ &RiverMetaTranslator::TranslateUnk,
		/* 0xE6 */ &RiverMetaTranslator::TranslateUnk,
		/* 0xE7 */ &RiverMetaTranslator::TranslateUnk,
		/* 0xE8 */ &RiverMetaTranslator::TranslateCall,
		/* 0xE9 */ &RiverMetaTranslator::TranslateDefault,
		/* 0xEA */ &RiverMetaTranslator::TranslateDefault,
		/* 0xEB */ &RiverMetaTranslator::TranslateDefault,
		/* 0xEC */ &RiverMetaTranslator::TranslateUnk,
		/* 0xED */ &RiverMetaTranslator::TranslateUnk,
		/* 0xEE */ &RiverMetaTranslator::TranslateUnk,
		/* 0xEF */ &RiverMetaTranslator::TranslateUnk,
		/* 0xF0 */ &RiverMetaTranslator::TranslateDefault,
		/* 0xF1 */ &RiverMetaTranslator::TranslateUnk,
		/* 0xF2 */ &RiverMetaTranslator::TranslateDefault,
		/* 0xF3 */ &RiverMetaTranslator::TranslateDefault,
		/* 0xF4 */ &RiverMetaTranslator::TranslateUnk,
		/* 0xF5 */ &RiverMetaTranslator::TranslateUnk,
		/* 0xF6 */ &RiverMetaTranslator::TranslateDefault,
		/* 0xF7 */ &RiverMetaTranslator::TranslateDefault,
		/* 0xF8 */ &RiverMetaTranslator::TranslateUnk,
		/* 0xF9 */ &RiverMetaTranslator::TranslateUnk,
		/* 0xFA */ &RiverMetaTranslator::TranslateUnk,
		/* 0xFB */ &RiverMetaTranslator::TranslateUnk,
		/* 0xFC */ &RiverMetaTranslator::TranslateDefault,
		/* 0xFD */ &RiverMetaTranslator::TranslateDefault,
		/* 0xFE */ &RiverMetaTranslator::TranslateDefault,
		/* 0xFF */ &RiverMetaTranslator::TranslateSubOp<translate0xFFOp>
	}, {
		/* 0x00 */ &RiverMetaTranslator::TranslateUnk,
		/* 0x01 */ &RiverMetaTranslator::TranslateUnk,
		/* 0x02 */ &RiverMetaTranslator::TranslateDefault,
		/* 0x03 */ &RiverMetaTranslator::TranslateDefault,
		/* 0x04 */ &RiverMetaTranslator::TranslateUnk,
		/* 0x05 */ &RiverMetaTranslator::TranslateUnk,
		/* 0x06 */ &RiverMetaTranslator::TranslateUnk,
		/* 0x07 */ &RiverMetaTranslator::TranslateUnk,
		/* 0x08 */ &RiverMetaTranslator::TranslateUnk,
		/* 0x09 */ &RiverMetaTranslator::TranslateUnk,
		/* 0x0A */ &RiverMetaTranslator::TranslateUnk,
		/* 0x0B */ &RiverMetaTranslator::TranslateUnk,
		/* 0x0C */ &RiverMetaTranslator::TranslateUnk,
		/* 0x0D */ &RiverMetaTranslator::TranslateUnk,
		/* 0x0E */ &RiverMetaTranslator::TranslateUnk,
		/* 0x0F */ &RiverMetaTranslator::TranslateUnk,
		/* 0x10 */ &RiverMetaTranslator::TranslateUnk,
		/* 0x11 */ &RiverMetaTranslator::TranslateUnk,
		/* 0x12 */ &RiverMetaTranslator::TranslateUnk,
		/* 0x13 */ &RiverMetaTranslator::TranslateUnk,
		/* 0x14 */ &RiverMetaTranslator::TranslateUnk,
		/* 0x15 */ &RiverMetaTranslator::TranslateUnk,
		/* 0x16 */ &RiverMetaTranslator::TranslateUnk,
		/* 0x17 */ &RiverMetaTranslator::TranslateUnk,
		/* 0x18 */ &RiverMetaTranslator::TranslateUnk,
		/* 0x19 */ &RiverMetaTranslator::TranslateUnk,
		/* 0x1A */ &RiverMetaTranslator::TranslateUnk,
		/* 0x1B */ &RiverMetaTranslator::TranslateUnk,
		/* 0x1C */ &RiverMetaTranslator::TranslateUnk,
		/* 0x1D */ &RiverMetaTranslator::TranslateUnk,
		/* 0x1E */ &RiverMetaTranslator::TranslateUnk,
		/* 0x1F */ &RiverMetaTranslator::TranslateUnk,
		/* 0x20 */ &RiverMetaTranslator::TranslateUnk,
		/* 0x21 */ &RiverMetaTranslator::TranslateUnk,
		/* 0x22 */ &RiverMetaTranslator::TranslateUnk,
		/* 0x23 */ &RiverMetaTranslator::TranslateUnk,
		/* 0x24 */ &RiverMetaTranslator::TranslateUnk,
		/* 0x25 */ &RiverMetaTranslator::TranslateUnk,
		/* 0x26 */ &RiverMetaTranslator::TranslateUnk,
		/* 0x27 */ &RiverMetaTranslator::TranslateUnk,
		/* 0x28 */ &RiverMetaTranslator::TranslateUnk,
		/* 0x29 */ &RiverMetaTranslator::TranslateUnk,
		/* 0x2A */ &RiverMetaTranslator::TranslateUnk,
		/* 0x2B */ &RiverMetaTranslator::TranslateUnk,
		/* 0x2C */ &RiverMetaTranslator::TranslateUnk,
		/* 0x2D */ &RiverMetaTranslator::TranslateUnk,
		/* 0x2E */ &RiverMetaTranslator::TranslateUnk,
		/* 0x2F */ &RiverMetaTranslator::TranslateUnk,
		/* 0x30 */ &RiverMetaTranslator::TranslateUnk,
		/* 0x31 */ &RiverMetaTranslator::TranslateDefault,
		/* 0x32 */ &RiverMetaTranslator::TranslateUnk,
		/* 0x33 */ &RiverMetaTranslator::TranslateUnk,
		/* 0x34 */ &RiverMetaTranslator::TranslateDefault,
		/* 0x35 */ &RiverMetaTranslator::TranslateUnk,
		/* 0x36 */ &RiverMetaTranslator::TranslateUnk,
		/* 0x37 */ &RiverMetaTranslator::TranslateUnk,
		/* 0x38 */ &RiverMetaTranslator::TranslateUnk,
		/* 0x39 */ &RiverMetaTranslator::TranslateUnk,
		/* 0x3A */ &RiverMetaTranslator::TranslateUnk,
		/* 0x3B */ &RiverMetaTranslator::TranslateUnk,
		/* 0x3C */ &RiverMetaTranslator::TranslateUnk,
		/* 0x3D */ &RiverMetaTranslator::TranslateUnk,
		/* 0x3E */ &RiverMetaTranslator::TranslateUnk,
		/* 0x4F */ &RiverMetaTranslator::TranslateUnk,
		/* 0x40 */ &RiverMetaTranslator::TranslateDefault,
		/* 0x41 */ &RiverMetaTranslator::TranslateDefault,
		/* 0x42 */ &RiverMetaTranslator::TranslateDefault,
		/* 0x43 */ &RiverMetaTranslator::TranslateDefault,
		/* 0x44 */ &RiverMetaTranslator::TranslateDefault,
		/* 0x45 */ &RiverMetaTranslator::TranslateDefault,
		/* 0x46 */ &RiverMetaTranslator::TranslateDefault,
		/* 0x47 */ &RiverMetaTranslator::TranslateDefault,
		/* 0x48 */ &RiverMetaTranslator::TranslateDefault,
		/* 0x49 */ &RiverMetaTranslator::TranslateDefault,
		/* 0x4A */ &RiverMetaTranslator::TranslateDefault,
		/* 0x4B */ &RiverMetaTranslator::TranslateDefault,
		/* 0x4C */ &RiverMetaTranslator::TranslateDefault,
		/* 0x4D */ &RiverMetaTranslator::TranslateDefault,
		/* 0x4E */ &RiverMetaTranslator::TranslateDefault,
		/* 0x4F */ &RiverMetaTranslator::TranslateDefault,
		/* 0x50 */ &RiverMetaTranslator::TranslateUnk,
		/* 0x51 */ &RiverMetaTranslator::TranslateUnk,
		/* 0x52 */ &RiverMetaTranslator::TranslateUnk,
		/* 0x53 */ &RiverMetaTranslator::TranslateUnk,
		/* 0x54 */ &RiverMetaTranslator::TranslateUnk,
		/* 0x55 */ &RiverMetaTranslator::TranslateUnk,
		/* 0x56 */ &RiverMetaTranslator::TranslateUnk,
		/* 0x57 */ &RiverMetaTranslator::TranslateUnk,
		/* 0x58 */ &RiverMetaTranslator::TranslateUnk,
		/* 0x59 */ &RiverMetaTranslator::TranslateUnk,
		/* 0x5A */ &RiverMetaTranslator::TranslateUnk,
		/* 0x5B */ &RiverMetaTranslator::TranslateUnk,
		/* 0x5C */ &RiverMetaTranslator::TranslateUnk,
		/* 0x5D */ &RiverMetaTranslator::TranslateUnk,
		/* 0x5E */ &RiverMetaTranslator::TranslateUnk,
		/* 0x5F */ &RiverMetaTranslator::TranslateUnk,
		/* 0x60 */ &RiverMetaTranslator::TranslateUnk,
		/* 0x61 */ &RiverMetaTranslator::TranslateUnk,
		/* 0x62 */ &RiverMetaTranslator::TranslateUnk,
		/* 0x63 */ &RiverMetaTranslator::TranslateUnk,
		/* 0x64 */ &RiverMetaTranslator::TranslateUnk,
		/* 0x65 */ &RiverMetaTranslator::TranslateUnk,
		/* 0x66 */ &RiverMetaTranslator::TranslateUnk,
		/* 0x67 */ &RiverMetaTranslator::TranslateUnk,
		/* 0x68 */ &RiverMetaTranslator::TranslateUnk,
		/* 0x69 */ &RiverMetaTranslator::TranslateUnk,
		/* 0x6A */ &RiverMetaTranslator::TranslateUnk,
		/* 0x6B */ &RiverMetaTranslator::TranslateUnk,
		/* 0x6C */ &RiverMetaTranslator::TranslateUnk,
		/* 0x6D */ &RiverMetaTranslator::TranslateUnk,
		/* 0x6E */ &RiverMetaTranslator::TranslateUnk,
		/* 0x6F */ &RiverMetaTranslator::TranslateUnk,
		/* 0x70 */ &RiverMetaTranslator::TranslateUnk,
		/* 0x71 */ &RiverMetaTranslator::TranslateUnk,
		/* 0x72 */ &RiverMetaTranslator::TranslateUnk,
		/* 0x73 */ &RiverMetaTranslator::TranslateUnk,
		/* 0x74 */ &RiverMetaTranslator::TranslateUnk,
		/* 0x75 */ &RiverMetaTranslator::TranslateUnk,
		/* 0x76 */ &RiverMetaTranslator::TranslateUnk,
		/* 0x77 */ &RiverMetaTranslator::TranslateUnk,
		/* 0x78 */ &RiverMetaTranslator::TranslateUnk,
		/* 0x79 */ &RiverMetaTranslator::TranslateUnk,
		/* 0x7A */ &RiverMetaTranslator::TranslateUnk,
		/* 0x7B */ &RiverMetaTranslator::TranslateUnk,
		/* 0x7C */ &RiverMetaTranslator::TranslateUnk,
		/* 0x7D */ &RiverMetaTranslator::TranslateUnk,
		/* 0x7E */ &RiverMetaTranslator::TranslateUnk,
		/* 0x7F */ &RiverMetaTranslator::TranslateUnk,
		/* 0x80 */ &RiverMetaTranslator::TranslateDefault, 
		/* 0x01 */ &RiverMetaTranslator::TranslateDefault, 
		/* 0x82 */ &RiverMetaTranslator::TranslateDefault, 
		/* 0x83 */ &RiverMetaTranslator::TranslateDefault, 
		/* 0x84 */ &RiverMetaTranslator::TranslateDefault, 
		/* 0x85 */ &RiverMetaTranslator::TranslateDefault, 
		/* 0x86 */ &RiverMetaTranslator::TranslateDefault, 
		/* 0x87 */ &RiverMetaTranslator::TranslateDefault, 
		/* 0x88 */ &RiverMetaTranslator::TranslateDefault, 
		/* 0x89 */ &RiverMetaTranslator::TranslateDefault, 
		/* 0x8A */ &RiverMetaTranslator::TranslateDefault, 
		/* 0x8B */ &RiverMetaTranslator::TranslateDefault, 
		/* 0x8C */ &RiverMetaTranslator::TranslateDefault, 
		/* 0x8D */ &RiverMetaTranslator::TranslateDefault, 
		/* 0x8E */ &RiverMetaTranslator::TranslateDefault, 
		/* 0x8F */ &RiverMetaTranslator::TranslateDefault, 
		/* 0x90 */ &RiverMetaTranslator::TranslateDefault, 
		/* 0x91 */ &RiverMetaTranslator::TranslateDefault, 
		/* 0x92 */ &RiverMetaTranslator::TranslateDefault, 
		/* 0x93 */ &RiverMetaTranslator::TranslateDefault, 
		/* 0x94 */ &RiverMetaTranslator::TranslateDefault, 
		/* 0x95 */ &RiverMetaTranslator::TranslateDefault, 
		/* 0x96 */ &RiverMetaTranslator::TranslateDefault, 
		/* 0x97 */ &RiverMetaTranslator::TranslateDefault, 
		/* 0x98 */ &RiverMetaTranslator::TranslateDefault, 
		/* 0x99 */ &RiverMetaTranslator::TranslateDefault, 
		/* 0x9A */ &RiverMetaTranslator::TranslateDefault, 
		/* 0x9B */ &RiverMetaTranslator::TranslateDefault, 
		/* 0x9C */ &RiverMetaTranslator::TranslateDefault, 
		/* 0x9D */ &RiverMetaTranslator::TranslateDefault, 
		/* 0x9E */ &RiverMetaTranslator::TranslateDefault, 
		/* 0x9F */ &RiverMetaTranslator::TranslateDefault, 
		/* 0xA0 */ &RiverMetaTranslator::TranslateUnk,
		/* 0xA1 */ &RiverMetaTranslator::TranslateUnk,
		/* 0xA2 */ &RiverMetaTranslator::TranslateDefault,
		/* 0xA3 */ &RiverMetaTranslator::TranslateUnk,
		/* 0xA4 */ &RiverMetaTranslator::TranslateDefault,
		/* 0xA5 */ &RiverMetaTranslator::TranslateDefault,
		/* 0xA6 */ &RiverMetaTranslator::TranslateUnk,
		/* 0xA7 */ &RiverMetaTranslator::TranslateUnk,
		/* 0xA8 */ &RiverMetaTranslator::TranslateUnk,
		/* 0xA9 */ &RiverMetaTranslator::TranslateUnk,
		/* 0xAA */ &RiverMetaTranslator::TranslateUnk,
		/* 0xAB */ &RiverMetaTranslator::TranslateUnk,
		/* 0xAC */ &RiverMetaTranslator::TranslateDefault,
		/* 0xAD */ &RiverMetaTranslator::TranslateDefault,
		/* 0xAE */ &RiverMetaTranslator::TranslateUnk,
		/* 0xAF */ &RiverMetaTranslator::TranslateDefault,
		/* 0xB0 */ &RiverMetaTranslator::TranslateDefault,
		/* 0xB1 */ &RiverMetaTranslator::TranslateDefault,
		/* 0xB2 */ &RiverMetaTranslator::TranslateUnk,
		/* 0xB3 */ &RiverMetaTranslator::TranslateUnk,
		/* 0xB4 */ &RiverMetaTranslator::TranslateUnk,
		/* 0xB5 */ &RiverMetaTranslator::TranslateUnk,
		/* 0xB6 */ &RiverMetaTranslator::TranslateDefault,
		/* 0xB7 */ &RiverMetaTranslator::TranslateDefault,
		/* 0xB8 */ &RiverMetaTranslator::TranslateUnk,
		/* 0xB9 */ &RiverMetaTranslator::TranslateUnk,
		/* 0xBA */ &RiverMetaTranslator::TranslateDefault,
		/* 0xBB */ &RiverMetaTranslator::TranslateUnk,
		/* 0xBC */ &RiverMetaTranslator::TranslateDefault,
		/* 0xBD */ &RiverMetaTranslator::TranslateDefault,
		/* 0xBE */ &RiverMetaTranslator::TranslateDefault,
		/* 0xBF */ &RiverMetaTranslator::TranslateDefault,
		/* 0xC0 */ &RiverMetaTranslator::TranslateDefault,
		/* 0xC1 */ &RiverMetaTranslator::TranslateDefault,
		/* 0xC2 */ &RiverMetaTranslator::TranslateUnk,
		/* 0xC3 */ &RiverMetaTranslator::TranslateUnk,
		/* 0xC4 */ &RiverMetaTranslator::TranslateUnk,
		/* 0xC5 */ &RiverMetaTranslator::TranslateUnk,
		/* 0xC6 */ &RiverMetaTranslator::TranslateUnk,
		/* 0xC7 */ &RiverMetaTranslator::TranslateDefault,
		/* 0xC8 */ &RiverMetaTranslator::TranslateUnk,
		/* 0xC9 */ &RiverMetaTranslator::TranslateUnk,
		/* 0xCA */ &RiverMetaTranslator::TranslateUnk,
		/* 0xCB */ &RiverMetaTranslator::TranslateUnk,
		/* 0xCC */ &RiverMetaTranslator::TranslateUnk,
		/* 0xCD */ &RiverMetaTranslator::TranslateUnk,
		/* 0xCE */ &RiverMetaTranslator::TranslateUnk,
		/* 0xCF */ &RiverMetaTranslator::TranslateUnk,
		/* 0xD0 */ &RiverMetaTranslator::TranslateUnk,
		/* 0xD1 */ &RiverMetaTranslator::TranslateUnk,
		/* 0xD2 */ &RiverMetaTranslator::TranslateUnk,
		/* 0xD3 */ &RiverMetaTranslator::TranslateUnk,
		/* 0xD4 */ &RiverMetaTranslator::TranslateUnk,
		/* 0xD5 */ &RiverMetaTranslator::TranslateUnk,
		/* 0xD6 */ &RiverMetaTranslator::TranslateUnk,
		/* 0xD7 */ &RiverMetaTranslator::TranslateUnk,
		/* 0xD8 */ &RiverMetaTranslator::TranslateUnk,
		/* 0xD9 */ &RiverMetaTranslator::TranslateUnk,
		/* 0xDA */ &RiverMetaTranslator::TranslateUnk,
		/* 0xDB */ &RiverMetaTranslator::TranslateUnk,
		/* 0xDC */ &RiverMetaTranslator::TranslateUnk,
		/* 0xDD */ &RiverMetaTranslator::TranslateUnk,
		/* 0xDE */ &RiverMetaTranslator::TranslateUnk,
		/* 0xDF */ &RiverMetaTranslator::TranslateUnk,
		/* 0xE0 */ &RiverMetaTranslator::TranslateUnk,
		/* 0xE1 */ &RiverMetaTranslator::TranslateUnk,
		/* 0xE2 */ &RiverMetaTranslator::TranslateUnk,
		/* 0xE3 */ &RiverMetaTranslator::TranslateUnk,
		/* 0xE4 */ &RiverMetaTranslator::TranslateUnk,
		/* 0xE5 */ &RiverMetaTranslator::TranslateUnk,
		/* 0xE6 */ &RiverMetaTranslator::TranslateUnk,
		/* 0xE7 */ &RiverMetaTranslator::TranslateUnk,
		/* 0xE8 */ &RiverMetaTranslator::TranslateUnk,
		/* 0xE9 */ &RiverMetaTranslator::TranslateUnk,
		/* 0xEA */ &RiverMetaTranslator::TranslateUnk,
		/* 0xEB */ &RiverMetaTranslator::TranslateUnk,
		/* 0xEC */ &RiverMetaTranslator::TranslateUnk,
		/* 0xED */ &RiverMetaTranslator::TranslateUnk,
		/* 0xEE */ &RiverMetaTranslator::TranslateUnk,
		/* 0xEF */ &RiverMetaTranslator::TranslateUnk,
		/* 0xF0 */ &RiverMetaTranslator::TranslateUnk,
		/* 0xF1 */ &RiverMetaTranslator::TranslateUnk,
		/* 0xF2 */ &RiverMetaTranslator::TranslateUnk,
		/* 0xF3 */ &RiverMetaTranslator::TranslateUnk,
		/* 0xF4 */ &RiverMetaTranslator::TranslateUnk,
		/* 0xF5 */ &RiverMetaTranslator::TranslateUnk,
		/* 0xF6 */ &RiverMetaTranslator::TranslateUnk,
		/* 0xF7 */ &RiverMetaTranslator::TranslateUnk,
		/* 0xF8 */ &RiverMetaTranslator::TranslateUnk,
		/* 0xF9 */ &RiverMetaTranslator::TranslateUnk,
		/* 0xFA */ &RiverMetaTranslator::TranslateUnk,
		/* 0xFB */ &RiverMetaTranslator::TranslateUnk,
		/* 0xFC */ &RiverMetaTranslator::TranslateUnk,
		/* 0xFD */ &RiverMetaTranslator::TranslateUnk,
		/* 0xFE */ &RiverMetaTranslator::TranslateUnk,
		/* 0xFF */ &RiverMetaTranslator::TranslateUnk
	}
};

