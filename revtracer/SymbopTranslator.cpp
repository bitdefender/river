#include "SymbopTranslator.h"

#include "CodeGen.h"

void SymbopTranslator::CopyInstruction(RiverInstruction &rOut, const RiverInstruction &rIn) {
	memcpy(&rOut, &rIn, sizeof(rOut));

	if (RIVER_OPTYPE_MEM == RIVER_OPTYPE(rIn.opTypes[0])) {
		rOut.operands[0].asAddress = codegen->CloneAddress(*rIn.operands[0].asAddress, rIn.modifiers);
	}

	if (RIVER_OPTYPE_MEM == RIVER_OPTYPE(rIn.opTypes[1])) {
		rOut.operands[1].asAddress = codegen->CloneAddress(*rIn.operands[1].asAddress, rIn.modifiers);
	}
}

DWORD SymbopTranslator::GetMemRepr(const RiverAddress &mem) {
	return 0;
}

bool SymbopTranslator::Init(RiverCodeGen *cg) {
	codegen = cg;
	return true;
}

bool SymbopTranslator::Translate(const RiverInstruction &rIn, RiverInstruction *rMainOut, DWORD &instrCount, RiverInstruction *rTrackOut, DWORD &trackCount) {
	if (rIn.modifiers & RIVER_FAMILY_RIVEROP) {
		/* do not track river operations */
		CopyInstruction(rMainOut[instrCount], rIn);
		instrCount++;
		return true;
	}

	DWORD dwTable = (RIVER_MODIFIER_EXT & rIn.modifiers) ? 1 : 0;
	(this->*translateOpcodes[dwTable][rIn.opCode])(rIn, rMainOut, instrCount, rTrackOut, trackCount);

	return true;
}

void SymbopTranslator::MakeTrackFlg(RiverInstruction *rMainOut, DWORD &instrCount, RiverInstruction *rTrackOut, DWORD &trackCount) {
	rTrackOut->opCode = 0x9C; // pushf
	rTrackOut->opTypes[0] = rTrackOut->opTypes[1] = RIVER_OPTYPE_NONE;
	rTrackOut->subOpCode = 0;
	rTrackOut->modifiers = 0;
	rTrackOut->family = RIVER_FAMILY_SYMBOP;

	trackCount++;
}

void SymbopTranslator::MakeMarkFlg(RiverInstruction *rMainOut, DWORD &instrCount, RiverInstruction *rTrackOut, DWORD &trackCount) {
	rTrackOut->opCode = 0x9D; // popf
	rTrackOut->opTypes[0] = rTrackOut->opTypes[1] = RIVER_OPTYPE_NONE;
	rTrackOut->subOpCode = 0;
	rTrackOut->modifiers = 0;
	rTrackOut->family = RIVER_FAMILY_SYMBOP;

	trackCount++;
}

void SymbopTranslator::MakeTrackReg(const RiverRegister &reg, RiverInstruction *rMainOut, DWORD &instrCount, RiverInstruction *rTrackOut, DWORD &trackCount) {
	rTrackOut->opCode = 0x50; // push + r

	rTrackOut->opTypes[0] = RIVER_OPTYPE_REG;
	rTrackOut->operands[0].asRegister.versioned = reg.versioned;

	rTrackOut->opTypes[1] = RIVER_OPTYPE_NONE;

	rTrackOut->modifiers = RIVER_FAMILY_SYMBOP | ((RIVER_REG_xSP == (reg.name & 0x07)) ? RIVER_FAMILY_ORIG_xSP : 0);
	trackCount++;
}

void SymbopTranslator::MakeMarkReg(const RiverRegister &reg, RiverInstruction *rMainOut, DWORD &instrCount, RiverInstruction *rTrackOut, DWORD &trackCount) {
	rTrackOut->opCode = 0x58; // pop + r

	rTrackOut->opTypes[0] = RIVER_OPTYPE_REG;
	rTrackOut->operands[0].asRegister.versioned = reg.versioned;

	rTrackOut->opTypes[1] = RIVER_OPTYPE_NONE;

	rTrackOut->modifiers = RIVER_FAMILY_SYMBOP | ((RIVER_REG_xSP == (reg.name & 0x07)) ? RIVER_FAMILY_ORIG_xSP : 0);
	trackCount++;
}


void SymbopTranslator::MakeTrackMem(const RiverAddress &mem, RiverInstruction *rMainOut, DWORD &instrCount, RiverInstruction *rTrackOut, DWORD &trackCount) {
	if (0 == mem.type) {
		MakeTrackReg(mem.base, rMainOut, instrCount, rTrackOut, trackCount);
		trackCount++;
		return;
	}

	rMainOut[instrCount].opCode = 0x8D; // lea eax, [mem]
	rMainOut[instrCount].modifiers = 0;
	rMainOut[instrCount].family = RIVER_FAMILY_SYMBOP;
	rMainOut[instrCount].opTypes[0] = RIVER_OPTYPE_REG;
	rMainOut[instrCount].operands[0].asRegister.versioned = RIVER_REG_xAX;
	rMainOut[instrCount].opTypes[1] = RIVER_OPTYPE_MEM;
	rMainOut[instrCount].operands[1].asAddress = codegen->CloneAddress(mem, 0);
	instrCount++;

	rMainOut[instrCount].opCode = 0x50;
	rMainOut[instrCount].modifiers = 0;
	rMainOut[instrCount].family = RIVER_FAMILY_SYMBOP;
	rMainOut[instrCount].opTypes[0] = RIVER_OPTYPE_REG;
	rMainOut[instrCount].operands[0].asRegister.versioned = RIVER_REG_xAX; //reg.name & 0xFFFFFF07;
	rMainOut[instrCount].opTypes[1] = RIVER_OPTYPE_NONE;
	instrCount++;

	rMainOut[instrCount].opCode = 0x68;
	rMainOut[instrCount].modifiers = 0;
	rMainOut[instrCount].family = RIVER_FAMILY_SYMBOP;
	rMainOut[instrCount].opTypes[0] = RIVER_OPTYPE_IMM;
	rMainOut[instrCount].operands[0].asImm32 = GetMemRepr(mem);
	rMainOut[instrCount].opTypes[1] = RIVER_OPTYPE_NONE;
	instrCount++;
	

	rTrackOut[trackCount].opCode = 0xFF;
	rTrackOut[trackCount].subOpCode = 0x06;
	rTrackOut[trackCount].modifiers = 0;
	rTrackOut[trackCount].family = RIVER_FAMILY_NATIVE;
	rTrackOut[trackCount].opTypes[0] = RIVER_OPTYPE_MEM;
	rTrackOut[trackCount].operands[0].asAddress = codegen->CloneAddress(mem, 0);
	trackCount++;
}

void SymbopTranslator::MakeMarkMem(const RiverAddress &mem, RiverInstruction *rMainOut, DWORD &instrCount, RiverInstruction *rTrackOut, DWORD &trackCount) {
	rTrackOut->opCode = 0x8F;
	rTrackOut->subOpCode = 0x00;
	rTrackOut->opTypes[0] = RIVER_OPTYPE_MEM;
	rTrackOut->operands[0].asAddress = codegen->CloneAddress(mem, 0);
}

void SymbopTranslator::MakeSkipMem(const RiverAddress &mem, RiverInstruction *rMainOut, DWORD &instrCount, RiverInstruction *rTrackOut, DWORD &trackCount) {
	rTrackOut->opCode = 0x8F;
	rTrackOut->subOpCode = 0x07;
	rTrackOut->opTypes[0] = RIVER_OPTYPE_MEM;
	rTrackOut->operands[0].asAddress = codegen->CloneAddress(mem, 0);
}

void SymbopTranslator::MakeTrackOp(const BYTE type, const RiverOperand &op, RiverInstruction *rMainOut, DWORD &instrCount, RiverInstruction *rTrackOut, DWORD &trackCount) {
	switch (RIVER_OPTYPE(type)) {
	case RIVER_OPTYPE_IMM :
		break;
	case RIVER_OPTYPE_REG :
		MakeTrackReg(op.asRegister, rMainOut, instrCount, rTrackOut, trackCount);
		break;
	case RIVER_OPTYPE_MEM:
		MakeTrackMem(*op.asAddress, rMainOut, instrCount, rTrackOut, trackCount);
		break;
	}
}

void SymbopTranslator::MakeMarkOp(const BYTE type, const RiverOperand &op, RiverInstruction *rMainOut, DWORD &instrCount, RiverInstruction *rTrackOut, DWORD &trackCount) {
	switch (RIVER_OPTYPE(type)) {
	case RIVER_OPTYPE_IMM:
		break;
	case RIVER_OPTYPE_REG:
		MakeMarkReg(op.asRegister, rMainOut, instrCount, rTrackOut, trackCount);
		break;
	case RIVER_OPTYPE_MEM:
		MakeMarkMem(*op.asAddress, rMainOut, instrCount, rTrackOut, trackCount);
		break;
	}
}

void SymbopTranslator::TranslateUnk(const RiverInstruction &rIn, RiverInstruction *rMainOut, DWORD &instrCount, RiverInstruction *rTrackOut, DWORD &trackCount) {
	__asm int 3;
}

void SymbopTranslator::TranslateDefault(const RiverInstruction &rIn, RiverInstruction *rMainOut, DWORD &instrCount, RiverInstruction *rTrackOut, DWORD &trackCount) {
	if (0 == (RIVER_SPEC_IGNORES_FLG & rIn.specifiers)) {
		MakeTrackFlg(rMainOut, instrCount, rTrackOut, trackCount);
	}

	if (RIVER_OPTYPE_NONE != rIn.opTypes[1]) {
		MakeTrackOp(rIn.opTypes[1], rIn.operands[1], rMainOut, instrCount, rTrackOut, trackCount);
	}

	if (RIVER_OPTYPE_NONE != rIn.opTypes[0]) {
		MakeTrackOp(rIn.opTypes[0], rIn.operands[0], rMainOut, instrCount, rTrackOut, trackCount);
	}

	// make opcode
	CopyInstruction(rMainOut[instrCount], rIn);
	instrCount++;
	

	if (RIVER_SPEC_MODIFIES_FLG & rIn.specifiers) {
		MakeMarkFlg(rMainOut, instrCount, rTrackOut, trackCount);
	}

	if ((RIVER_SPEC_MODIFIES_OP2 & rIn.specifiers) && (RIVER_OPTYPE_NONE != rIn.opTypes[1])) {
		MakeMarkOp(rIn.opTypes[1], rIn.operands[1], rMainOut, instrCount, rTrackOut, trackCount);
	}

	if ((RIVER_SPEC_MODIFIES_OP1 & rIn.specifiers) && (RIVER_OPTYPE_NONE != rIn.opTypes[0])) {
		MakeMarkOp(rIn.opTypes[1], rIn.operands[1], rMainOut, instrCount, rTrackOut, trackCount);
	}
}






SymbopTranslator::TranslateOpcodeFunc SymbopTranslator::translateOpcodes[2][0x100] = {
	{
		/* 0x00 */ &SymbopTranslator::TranslateUnk, &SymbopTranslator::TranslateUnk, &SymbopTranslator::TranslateUnk, &SymbopTranslator::TranslateUnk,
		/* 0x04 */ &SymbopTranslator::TranslateUnk, &SymbopTranslator::TranslateUnk, &SymbopTranslator::TranslateUnk, &SymbopTranslator::TranslateUnk,
		/* 0x08 */ &SymbopTranslator::TranslateUnk, &SymbopTranslator::TranslateUnk, &SymbopTranslator::TranslateUnk, &SymbopTranslator::TranslateUnk,
		/* 0x0C */ &SymbopTranslator::TranslateUnk, &SymbopTranslator::TranslateUnk, &SymbopTranslator::TranslateUnk, &SymbopTranslator::TranslateUnk,

		/* 0x10 */ &SymbopTranslator::TranslateUnk, &SymbopTranslator::TranslateUnk, &SymbopTranslator::TranslateUnk, &SymbopTranslator::TranslateUnk,
		/* 0x14 */ &SymbopTranslator::TranslateUnk, &SymbopTranslator::TranslateUnk, &SymbopTranslator::TranslateUnk, &SymbopTranslator::TranslateUnk,
		/* 0x18 */ &SymbopTranslator::TranslateUnk, &SymbopTranslator::TranslateUnk, &SymbopTranslator::TranslateUnk, &SymbopTranslator::TranslateUnk,
		/* 0x1C */ &SymbopTranslator::TranslateUnk, &SymbopTranslator::TranslateUnk, &SymbopTranslator::TranslateUnk, &SymbopTranslator::TranslateUnk,

		/* 0x20 */ &SymbopTranslator::TranslateUnk, &SymbopTranslator::TranslateUnk, &SymbopTranslator::TranslateUnk, &SymbopTranslator::TranslateUnk,
		/* 0x24 */ &SymbopTranslator::TranslateUnk, &SymbopTranslator::TranslateUnk, &SymbopTranslator::TranslateUnk, &SymbopTranslator::TranslateUnk,
		/* 0x28 */ &SymbopTranslator::TranslateUnk, &SymbopTranslator::TranslateUnk, &SymbopTranslator::TranslateUnk, &SymbopTranslator::TranslateUnk,
		/* 0x2C */ &SymbopTranslator::TranslateUnk, &SymbopTranslator::TranslateUnk, &SymbopTranslator::TranslateUnk, &SymbopTranslator::TranslateUnk,

		/* 0x30 */ &SymbopTranslator::TranslateUnk, &SymbopTranslator::TranslateUnk, &SymbopTranslator::TranslateUnk, &SymbopTranslator::TranslateDefault,
		/* 0x34 */ &SymbopTranslator::TranslateUnk, &SymbopTranslator::TranslateUnk, &SymbopTranslator::TranslateUnk, &SymbopTranslator::TranslateUnk,
		/* 0x38 */ &SymbopTranslator::TranslateUnk, &SymbopTranslator::TranslateUnk, &SymbopTranslator::TranslateUnk, &SymbopTranslator::TranslateDefault,
		/* 0x3C */ &SymbopTranslator::TranslateUnk, &SymbopTranslator::TranslateUnk, &SymbopTranslator::TranslateUnk, &SymbopTranslator::TranslateUnk,

		/* 0x40 */ &SymbopTranslator::TranslateUnk, &SymbopTranslator::TranslateUnk, &SymbopTranslator::TranslateUnk, &SymbopTranslator::TranslateUnk,
		/* 0x44 */ &SymbopTranslator::TranslateUnk, &SymbopTranslator::TranslateUnk, &SymbopTranslator::TranslateUnk, &SymbopTranslator::TranslateUnk,
		/* 0x48 */ &SymbopTranslator::TranslateUnk, &SymbopTranslator::TranslateUnk, &SymbopTranslator::TranslateUnk, &SymbopTranslator::TranslateUnk,
		/* 0x4C */ &SymbopTranslator::TranslateUnk, &SymbopTranslator::TranslateUnk, &SymbopTranslator::TranslateUnk, &SymbopTranslator::TranslateUnk,

		/* not really default */
		/* 0x50 */ &SymbopTranslator::TranslateDefault, &SymbopTranslator::TranslateDefault, &SymbopTranslator::TranslateDefault, &SymbopTranslator::TranslateDefault,
		/* 0x54 */ &SymbopTranslator::TranslateDefault, &SymbopTranslator::TranslateDefault, &SymbopTranslator::TranslateDefault, &SymbopTranslator::TranslateDefault,
		/* 0x58 */ &SymbopTranslator::TranslateDefault, &SymbopTranslator::TranslateDefault, &SymbopTranslator::TranslateDefault, &SymbopTranslator::TranslateDefault,
		/* 0x5C */ &SymbopTranslator::TranslateDefault, &SymbopTranslator::TranslateDefault, &SymbopTranslator::TranslateDefault, &SymbopTranslator::TranslateDefault,

		/* 0x60 */ &SymbopTranslator::TranslateUnk, &SymbopTranslator::TranslateUnk, &SymbopTranslator::TranslateUnk, &SymbopTranslator::TranslateUnk,
		/* 0x64 */ &SymbopTranslator::TranslateUnk, &SymbopTranslator::TranslateUnk, &SymbopTranslator::TranslateUnk, &SymbopTranslator::TranslateUnk,
		/* 0x68 */ &SymbopTranslator::TranslateUnk, &SymbopTranslator::TranslateUnk, &SymbopTranslator::TranslateUnk, &SymbopTranslator::TranslateUnk,
		/* 0x6C */ &SymbopTranslator::TranslateUnk, &SymbopTranslator::TranslateUnk, &SymbopTranslator::TranslateUnk, &SymbopTranslator::TranslateUnk,

		/* 0x70 */ &SymbopTranslator::TranslateUnk, &SymbopTranslator::TranslateUnk, &SymbopTranslator::TranslateDefault, &SymbopTranslator::TranslateUnk,
		/* 0x74 */ &SymbopTranslator::TranslateUnk, &SymbopTranslator::TranslateUnk, &SymbopTranslator::TranslateUnk, &SymbopTranslator::TranslateDefault,
		/* 0x78 */ &SymbopTranslator::TranslateUnk, &SymbopTranslator::TranslateUnk, &SymbopTranslator::TranslateUnk, &SymbopTranslator::TranslateUnk,
		/* 0x7C */ &SymbopTranslator::TranslateUnk, &SymbopTranslator::TranslateUnk, &SymbopTranslator::TranslateUnk, &SymbopTranslator::TranslateUnk,

		/* 0x80 */ &SymbopTranslator::TranslateUnk, &SymbopTranslator::TranslateUnk, &SymbopTranslator::TranslateUnk, &SymbopTranslator::TranslateDefault,
		/* 0x84 */ &SymbopTranslator::TranslateUnk, &SymbopTranslator::TranslateUnk, &SymbopTranslator::TranslateUnk, &SymbopTranslator::TranslateUnk,
		/* 0x88 */ &SymbopTranslator::TranslateUnk, &SymbopTranslator::TranslateDefault, &SymbopTranslator::TranslateUnk, &SymbopTranslator::TranslateDefault,
		/* 0x8C */ &SymbopTranslator::TranslateUnk, &SymbopTranslator::TranslateUnk, &SymbopTranslator::TranslateUnk, &SymbopTranslator::TranslateUnk,

		/* 0x90 */ &SymbopTranslator::TranslateUnk, &SymbopTranslator::TranslateUnk, &SymbopTranslator::TranslateUnk, &SymbopTranslator::TranslateUnk,
		/* 0x94 */ &SymbopTranslator::TranslateUnk, &SymbopTranslator::TranslateUnk, &SymbopTranslator::TranslateUnk, &SymbopTranslator::TranslateUnk,
		/* 0x98 */ &SymbopTranslator::TranslateUnk, &SymbopTranslator::TranslateUnk, &SymbopTranslator::TranslateUnk, &SymbopTranslator::TranslateUnk,
		/* 0x9C */ &SymbopTranslator::TranslateUnk, &SymbopTranslator::TranslateUnk, &SymbopTranslator::TranslateUnk, &SymbopTranslator::TranslateUnk,

		/* 0xA0 */ &SymbopTranslator::TranslateUnk, &SymbopTranslator::TranslateUnk, &SymbopTranslator::TranslateUnk, &SymbopTranslator::TranslateUnk,
		/* 0xA4 */ &SymbopTranslator::TranslateUnk, &SymbopTranslator::TranslateUnk, &SymbopTranslator::TranslateUnk, &SymbopTranslator::TranslateUnk,
		/* 0xA8 */ &SymbopTranslator::TranslateUnk, &SymbopTranslator::TranslateUnk, &SymbopTranslator::TranslateUnk, &SymbopTranslator::TranslateUnk,
		/* 0xAC */ &SymbopTranslator::TranslateUnk, &SymbopTranslator::TranslateUnk, &SymbopTranslator::TranslateUnk, &SymbopTranslator::TranslateUnk,

		/* 0xB0 */ &SymbopTranslator::TranslateUnk, &SymbopTranslator::TranslateUnk, &SymbopTranslator::TranslateUnk, &SymbopTranslator::TranslateUnk,
		/* 0xB4 */ &SymbopTranslator::TranslateUnk, &SymbopTranslator::TranslateUnk, &SymbopTranslator::TranslateUnk, &SymbopTranslator::TranslateUnk,
		/* 0xB8 */ &SymbopTranslator::TranslateUnk, &SymbopTranslator::TranslateUnk, &SymbopTranslator::TranslateUnk, &SymbopTranslator::TranslateUnk,
		/* 0xBC */ &SymbopTranslator::TranslateUnk, &SymbopTranslator::TranslateUnk, &SymbopTranslator::TranslateUnk, &SymbopTranslator::TranslateUnk,

		/* 0xC0 */ &SymbopTranslator::TranslateUnk, &SymbopTranslator::TranslateUnk, &SymbopTranslator::TranslateUnk, &SymbopTranslator::TranslateDefault,
		/* 0xC4 */ &SymbopTranslator::TranslateUnk, &SymbopTranslator::TranslateUnk, &SymbopTranslator::TranslateUnk, &SymbopTranslator::TranslateUnk,
		/* 0xC8 */ &SymbopTranslator::TranslateUnk, &SymbopTranslator::TranslateUnk, &SymbopTranslator::TranslateUnk, &SymbopTranslator::TranslateUnk,
		/* 0xCC */ &SymbopTranslator::TranslateUnk, &SymbopTranslator::TranslateUnk, &SymbopTranslator::TranslateUnk, &SymbopTranslator::TranslateUnk,

		/* 0xD0 */ &SymbopTranslator::TranslateUnk, &SymbopTranslator::TranslateUnk, &SymbopTranslator::TranslateUnk, &SymbopTranslator::TranslateUnk,
		/* 0xD4 */ &SymbopTranslator::TranslateUnk, &SymbopTranslator::TranslateUnk, &SymbopTranslator::TranslateUnk, &SymbopTranslator::TranslateUnk,
		/* 0xD8 */ &SymbopTranslator::TranslateUnk, &SymbopTranslator::TranslateUnk, &SymbopTranslator::TranslateUnk, &SymbopTranslator::TranslateUnk,
		/* 0xDC */ &SymbopTranslator::TranslateUnk, &SymbopTranslator::TranslateUnk, &SymbopTranslator::TranslateUnk, &SymbopTranslator::TranslateUnk,

		/* 0xE0 */ &SymbopTranslator::TranslateUnk, &SymbopTranslator::TranslateUnk, &SymbopTranslator::TranslateUnk, &SymbopTranslator::TranslateUnk,
		/* 0xE4 */ &SymbopTranslator::TranslateUnk, &SymbopTranslator::TranslateUnk, &SymbopTranslator::TranslateUnk, &SymbopTranslator::TranslateUnk,
		/* 0xE8 */ &SymbopTranslator::TranslateUnk, &SymbopTranslator::TranslateDefault, &SymbopTranslator::TranslateUnk, &SymbopTranslator::TranslateUnk,
		/* 0xEC */ &SymbopTranslator::TranslateUnk, &SymbopTranslator::TranslateUnk, &SymbopTranslator::TranslateUnk, &SymbopTranslator::TranslateUnk,

		/* 0xF0 */ &SymbopTranslator::TranslateUnk, &SymbopTranslator::TranslateUnk, &SymbopTranslator::TranslateUnk, &SymbopTranslator::TranslateUnk,
		/* 0xF4 */ &SymbopTranslator::TranslateUnk, &SymbopTranslator::TranslateUnk, &SymbopTranslator::TranslateUnk, &SymbopTranslator::TranslateUnk,
		/* 0xF8 */ &SymbopTranslator::TranslateUnk, &SymbopTranslator::TranslateUnk, &SymbopTranslator::TranslateUnk, &SymbopTranslator::TranslateUnk,
		/* 0xFC */ &SymbopTranslator::TranslateUnk, &SymbopTranslator::TranslateUnk, &SymbopTranslator::TranslateUnk, &SymbopTranslator::TranslateUnk
	}, {
		/* 0x00 */ &SymbopTranslator::TranslateUnk, &SymbopTranslator::TranslateUnk, &SymbopTranslator::TranslateUnk, &SymbopTranslator::TranslateUnk,
		/* 0x04 */ &SymbopTranslator::TranslateUnk, &SymbopTranslator::TranslateUnk, &SymbopTranslator::TranslateUnk, &SymbopTranslator::TranslateUnk,
		/* 0x08 */ &SymbopTranslator::TranslateUnk, &SymbopTranslator::TranslateUnk, &SymbopTranslator::TranslateUnk, &SymbopTranslator::TranslateUnk,
		/* 0x0C */ &SymbopTranslator::TranslateUnk, &SymbopTranslator::TranslateUnk, &SymbopTranslator::TranslateUnk, &SymbopTranslator::TranslateUnk,

		/* 0x10 */ &SymbopTranslator::TranslateUnk, &SymbopTranslator::TranslateUnk, &SymbopTranslator::TranslateUnk, &SymbopTranslator::TranslateUnk,
		/* 0x14 */ &SymbopTranslator::TranslateUnk, &SymbopTranslator::TranslateUnk, &SymbopTranslator::TranslateUnk, &SymbopTranslator::TranslateUnk,
		/* 0x18 */ &SymbopTranslator::TranslateUnk, &SymbopTranslator::TranslateUnk, &SymbopTranslator::TranslateUnk, &SymbopTranslator::TranslateUnk,
		/* 0x1C */ &SymbopTranslator::TranslateUnk, &SymbopTranslator::TranslateUnk, &SymbopTranslator::TranslateUnk, &SymbopTranslator::TranslateUnk,

		/* 0x20 */ &SymbopTranslator::TranslateUnk, &SymbopTranslator::TranslateUnk, &SymbopTranslator::TranslateUnk, &SymbopTranslator::TranslateUnk,
		/* 0x24 */ &SymbopTranslator::TranslateUnk, &SymbopTranslator::TranslateUnk, &SymbopTranslator::TranslateUnk, &SymbopTranslator::TranslateUnk,
		/* 0x28 */ &SymbopTranslator::TranslateUnk, &SymbopTranslator::TranslateUnk, &SymbopTranslator::TranslateUnk, &SymbopTranslator::TranslateUnk,
		/* 0x2C */ &SymbopTranslator::TranslateUnk, &SymbopTranslator::TranslateUnk, &SymbopTranslator::TranslateUnk, &SymbopTranslator::TranslateUnk,

		/* 0x30 */ &SymbopTranslator::TranslateUnk, &SymbopTranslator::TranslateUnk, &SymbopTranslator::TranslateUnk, &SymbopTranslator::TranslateUnk,
		/* 0x34 */ &SymbopTranslator::TranslateUnk, &SymbopTranslator::TranslateUnk, &SymbopTranslator::TranslateUnk, &SymbopTranslator::TranslateUnk,
		/* 0x38 */ &SymbopTranslator::TranslateUnk, &SymbopTranslator::TranslateUnk, &SymbopTranslator::TranslateUnk, &SymbopTranslator::TranslateUnk,
		/* 0x3C */ &SymbopTranslator::TranslateUnk, &SymbopTranslator::TranslateUnk, &SymbopTranslator::TranslateUnk, &SymbopTranslator::TranslateUnk,

		/* 0x40 */ &SymbopTranslator::TranslateUnk, &SymbopTranslator::TranslateUnk, &SymbopTranslator::TranslateUnk, &SymbopTranslator::TranslateUnk,
		/* 0x44 */ &SymbopTranslator::TranslateUnk, &SymbopTranslator::TranslateUnk, &SymbopTranslator::TranslateUnk, &SymbopTranslator::TranslateUnk,
		/* 0x48 */ &SymbopTranslator::TranslateUnk, &SymbopTranslator::TranslateUnk, &SymbopTranslator::TranslateUnk, &SymbopTranslator::TranslateUnk,
		/* 0x4C */ &SymbopTranslator::TranslateUnk, &SymbopTranslator::TranslateUnk, &SymbopTranslator::TranslateUnk, &SymbopTranslator::TranslateUnk,

		/* 0x50 */ &SymbopTranslator::TranslateUnk, &SymbopTranslator::TranslateUnk, &SymbopTranslator::TranslateUnk, &SymbopTranslator::TranslateUnk,
		/* 0x54 */ &SymbopTranslator::TranslateUnk, &SymbopTranslator::TranslateUnk, &SymbopTranslator::TranslateUnk, &SymbopTranslator::TranslateUnk,
		/* 0x58 */ &SymbopTranslator::TranslateUnk, &SymbopTranslator::TranslateUnk, &SymbopTranslator::TranslateUnk, &SymbopTranslator::TranslateUnk,
		/* 0x5C */ &SymbopTranslator::TranslateUnk, &SymbopTranslator::TranslateUnk, &SymbopTranslator::TranslateUnk, &SymbopTranslator::TranslateUnk,

		/* 0x60 */ &SymbopTranslator::TranslateUnk, &SymbopTranslator::TranslateUnk, &SymbopTranslator::TranslateUnk, &SymbopTranslator::TranslateUnk,
		/* 0x64 */ &SymbopTranslator::TranslateUnk, &SymbopTranslator::TranslateUnk, &SymbopTranslator::TranslateUnk, &SymbopTranslator::TranslateUnk,
		/* 0x68 */ &SymbopTranslator::TranslateUnk, &SymbopTranslator::TranslateUnk, &SymbopTranslator::TranslateUnk, &SymbopTranslator::TranslateUnk,
		/* 0x6C */ &SymbopTranslator::TranslateUnk, &SymbopTranslator::TranslateUnk, &SymbopTranslator::TranslateUnk, &SymbopTranslator::TranslateUnk,

		/* 0x70 */ &SymbopTranslator::TranslateUnk, &SymbopTranslator::TranslateUnk, &SymbopTranslator::TranslateUnk, &SymbopTranslator::TranslateUnk,
		/* 0x74 */ &SymbopTranslator::TranslateUnk, &SymbopTranslator::TranslateUnk, &SymbopTranslator::TranslateUnk, &SymbopTranslator::TranslateUnk,
		/* 0x78 */ &SymbopTranslator::TranslateUnk, &SymbopTranslator::TranslateUnk, &SymbopTranslator::TranslateUnk, &SymbopTranslator::TranslateUnk,
		/* 0x7C */ &SymbopTranslator::TranslateUnk, &SymbopTranslator::TranslateUnk, &SymbopTranslator::TranslateUnk, &SymbopTranslator::TranslateUnk,

		/* 0x80 */ &SymbopTranslator::TranslateUnk, &SymbopTranslator::TranslateUnk, &SymbopTranslator::TranslateUnk, &SymbopTranslator::TranslateUnk,
		/* 0x84 */ &SymbopTranslator::TranslateUnk, &SymbopTranslator::TranslateUnk, &SymbopTranslator::TranslateUnk, &SymbopTranslator::TranslateUnk,
		/* 0x88 */ &SymbopTranslator::TranslateUnk, &SymbopTranslator::TranslateUnk, &SymbopTranslator::TranslateUnk, &SymbopTranslator::TranslateUnk,
		/* 0x8C */ &SymbopTranslator::TranslateUnk, &SymbopTranslator::TranslateUnk, &SymbopTranslator::TranslateUnk, &SymbopTranslator::TranslateUnk,

		/* 0x90 */ &SymbopTranslator::TranslateUnk, &SymbopTranslator::TranslateUnk, &SymbopTranslator::TranslateUnk, &SymbopTranslator::TranslateUnk,
		/* 0x94 */ &SymbopTranslator::TranslateUnk, &SymbopTranslator::TranslateUnk, &SymbopTranslator::TranslateUnk, &SymbopTranslator::TranslateUnk,
		/* 0x98 */ &SymbopTranslator::TranslateUnk, &SymbopTranslator::TranslateUnk, &SymbopTranslator::TranslateUnk, &SymbopTranslator::TranslateUnk,
		/* 0x9C */ &SymbopTranslator::TranslateUnk, &SymbopTranslator::TranslateUnk, &SymbopTranslator::TranslateUnk, &SymbopTranslator::TranslateUnk,

		/* 0xA0 */ &SymbopTranslator::TranslateUnk, &SymbopTranslator::TranslateUnk, &SymbopTranslator::TranslateUnk, &SymbopTranslator::TranslateUnk,
		/* 0xA4 */ &SymbopTranslator::TranslateUnk, &SymbopTranslator::TranslateUnk, &SymbopTranslator::TranslateUnk, &SymbopTranslator::TranslateUnk,
		/* 0xA8 */ &SymbopTranslator::TranslateUnk, &SymbopTranslator::TranslateUnk, &SymbopTranslator::TranslateUnk, &SymbopTranslator::TranslateUnk,
		/* 0xAC */ &SymbopTranslator::TranslateUnk, &SymbopTranslator::TranslateUnk, &SymbopTranslator::TranslateUnk, &SymbopTranslator::TranslateUnk,

		/* 0xB0 */ &SymbopTranslator::TranslateUnk, &SymbopTranslator::TranslateUnk, &SymbopTranslator::TranslateUnk, &SymbopTranslator::TranslateUnk,
		/* 0xB4 */ &SymbopTranslator::TranslateUnk, &SymbopTranslator::TranslateUnk, &SymbopTranslator::TranslateUnk, &SymbopTranslator::TranslateUnk,
		/* 0xB8 */ &SymbopTranslator::TranslateUnk, &SymbopTranslator::TranslateUnk, &SymbopTranslator::TranslateUnk, &SymbopTranslator::TranslateUnk,
		/* 0xBC */ &SymbopTranslator::TranslateUnk, &SymbopTranslator::TranslateUnk, &SymbopTranslator::TranslateUnk, &SymbopTranslator::TranslateUnk,

		/* 0xC0 */ &SymbopTranslator::TranslateUnk, &SymbopTranslator::TranslateUnk, &SymbopTranslator::TranslateUnk, &SymbopTranslator::TranslateUnk,
		/* 0xC4 */ &SymbopTranslator::TranslateUnk, &SymbopTranslator::TranslateUnk, &SymbopTranslator::TranslateUnk, &SymbopTranslator::TranslateUnk,
		/* 0xC8 */ &SymbopTranslator::TranslateUnk, &SymbopTranslator::TranslateUnk, &SymbopTranslator::TranslateUnk, &SymbopTranslator::TranslateUnk,
		/* 0xCC */ &SymbopTranslator::TranslateUnk, &SymbopTranslator::TranslateUnk, &SymbopTranslator::TranslateUnk, &SymbopTranslator::TranslateUnk,

		/* 0xD0 */ &SymbopTranslator::TranslateUnk, &SymbopTranslator::TranslateUnk, &SymbopTranslator::TranslateUnk, &SymbopTranslator::TranslateUnk,
		/* 0xD4 */ &SymbopTranslator::TranslateUnk, &SymbopTranslator::TranslateUnk, &SymbopTranslator::TranslateUnk, &SymbopTranslator::TranslateUnk,
		/* 0xD8 */ &SymbopTranslator::TranslateUnk, &SymbopTranslator::TranslateUnk, &SymbopTranslator::TranslateUnk, &SymbopTranslator::TranslateUnk,
		/* 0xDC */ &SymbopTranslator::TranslateUnk, &SymbopTranslator::TranslateUnk, &SymbopTranslator::TranslateUnk, &SymbopTranslator::TranslateUnk,

		/* 0xE0 */ &SymbopTranslator::TranslateUnk, &SymbopTranslator::TranslateUnk, &SymbopTranslator::TranslateUnk, &SymbopTranslator::TranslateUnk,
		/* 0xE4 */ &SymbopTranslator::TranslateUnk, &SymbopTranslator::TranslateUnk, &SymbopTranslator::TranslateUnk, &SymbopTranslator::TranslateUnk,
		/* 0xE8 */ &SymbopTranslator::TranslateUnk, &SymbopTranslator::TranslateUnk, &SymbopTranslator::TranslateUnk, &SymbopTranslator::TranslateUnk,
		/* 0xEC */ &SymbopTranslator::TranslateUnk, &SymbopTranslator::TranslateUnk, &SymbopTranslator::TranslateUnk, &SymbopTranslator::TranslateUnk,

		/* 0xF0 */ &SymbopTranslator::TranslateUnk, &SymbopTranslator::TranslateUnk, &SymbopTranslator::TranslateUnk, &SymbopTranslator::TranslateUnk,
		/* 0xF4 */ &SymbopTranslator::TranslateUnk, &SymbopTranslator::TranslateUnk, &SymbopTranslator::TranslateUnk, &SymbopTranslator::TranslateUnk,
		/* 0xF8 */ &SymbopTranslator::TranslateUnk, &SymbopTranslator::TranslateUnk, &SymbopTranslator::TranslateUnk, &SymbopTranslator::TranslateUnk,
		/* 0xFC */ &SymbopTranslator::TranslateUnk, &SymbopTranslator::TranslateUnk, &SymbopTranslator::TranslateUnk, &SymbopTranslator::TranslateUnk
	}
};
