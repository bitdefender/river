#include "SymbopTranslator.h"

#include "CodeGen.h"
#include "TranslatorUtil.h"

nodep::DWORD SymbopTranslator::GetMemRepr(const RiverAddress &mem) {
	return 0;
}

bool SymbopTranslator::Init(RiverCodeGen *cg) {
	codegen = cg;
	return true;
}

bool SymbopTranslator::Translate(const RiverInstruction &rIn, RiverInstruction *rMainOut, nodep::DWORD &instrCount, RiverInstruction *rTrackOut, nodep::DWORD &trackCount, nodep::DWORD dwTranslationFlags) {
	if ((RIVER_FAMILY(rIn.family) == RIVER_FAMILY_RIVER) || (RIVER_FAMILY_FLAG_METAPROCESSED & rIn.family)) {
		/* do not track river operations */
		CopyInstruction(codegen, *rMainOut, rIn);
		rMainOut++;
		instrCount++;
		return true;
	}

	nodep::DWORD dwTable = (RIVER_MODIFIER_EXT & rIn.modifiers) ? 1 : 0;
	(this->*translateOpcodes[dwTable][rIn.opCode])(rIn, rMainOut, instrCount, rTrackOut, trackCount, dwTranslationFlags);

	return true;
}

void SymbopTranslator::MakeInitTrack(const RiverInstruction &rIn, RiverInstruction *&rTrackOut, nodep::DWORD &trackCount) {
	trackedValues = 0;

	rTrackOut->opCode = 0xB8; // mov eax, 0
	rTrackOut->subOpCode = 0;
	rTrackOut->modifiers = 0;
	rTrackOut->family = RIVER_FAMILY_TRACK;

	rTrackOut->opTypes[0] = RIVER_OPTYPE_REG;
	rTrackOut->operands[0].asRegister.versioned = RIVER_REG_xDI;

	rTrackOut->opTypes[1] = RIVER_OPTYPE_IMM; 
	rTrackOut->operands[1].asImm32 = 0;

	rTrackOut->opTypes[2] = rTrackOut->opTypes[3] = RIVER_OPTYPE_NONE;
	rTrackOut->instructionAddress = rIn.instructionAddress;

	rTrackOut++;
	trackCount++;
}

void SymbopTranslator::MakeCleanTrack(RiverInstruction *&rTrackOut, nodep::DWORD &trackCount) {
	rTrackOut->opCode = 0xC3; // mov eax, 0
	rTrackOut->subOpCode = 0;
	rTrackOut->modifiers = 0;
	rTrackOut->family = RIVER_FAMILY_TRACK;

	rTrackOut->opTypes[0] = RIVER_OPTYPE_IMM | RIVER_OPSIZE_8;
	rTrackOut->operands[0].asImm8 = trackedValues;

	rTrackOut->opTypes[1] = rTrackOut->opTypes[2] = rTrackOut->opTypes[3] = RIVER_OPTYPE_NONE;

	rTrackOut++;
	trackCount++;

}

nodep::DWORD SymbopTranslator::MakeTrackFlg(nodep::BYTE flags, RiverInstruction *&rMainOut, nodep::DWORD &instrCount, RiverInstruction *&rTrackOut, nodep::DWORD &trackCount) {
	trackedValues += 1;

	rMainOut->opCode = 0x9C;
	rMainOut->opTypes[0] = rMainOut->opTypes[1] = rMainOut->opTypes[2] = rMainOut->opTypes[3] = RIVER_OPTYPE_NONE;
	rMainOut->modifiers = 0;
	rMainOut->specifiers = 0;
	rMainOut->family = RIVER_FAMILY_PRETRACK;
	rMainOut++;
	instrCount++;


	rTrackOut->opCode = 0x9C; // pushf
	rTrackOut->opTypes[0] = RIVER_OPTYPE_IMM | RIVER_OPSIZE_8;
	rTrackOut->operands[0].asImm8 = flags;
		
	rTrackOut->opTypes[1] = rTrackOut->opTypes[2] = rTrackOut->opTypes[3] = RIVER_OPTYPE_NONE;
	rTrackOut->subOpCode = 0;
	rTrackOut->modifiers = 0;
	rTrackOut->specifiers = 0;
	rTrackOut->family = RIVER_FAMILY_TRACK;

	rTrackOut++;
	trackCount++;

	return trackedValues - 1;
}

void SymbopTranslator::MakeMarkFlg(nodep::BYTE flags, nodep::DWORD offset, RiverInstruction *&rMainOut, nodep::DWORD &instrCount, RiverInstruction *&rTrackOut, nodep::DWORD &trackCount) {
	rTrackOut->opCode = 0x9D; // popf
	rTrackOut->opTypes[0] = RIVER_OPTYPE_IMM | RIVER_OPSIZE_8;
	rTrackOut->operands[0].asImm8 = flags;
		
	rTrackOut->opTypes[1] = rTrackOut->opTypes[2] = rTrackOut->opTypes[3] = RIVER_OPTYPE_NONE;
	rTrackOut->subOpCode = 0;
	rTrackOut->modifiers = 0;
	rTrackOut->family = RIVER_FAMILY_TRACK;

	rTrackOut++;
	trackCount++;
}

nodep::DWORD SymbopTranslator::MakePreTrackReg(const RiverRegister &reg, RiverInstruction *&rMainOut, nodep::DWORD &instrCount) {
	trackedValues += 1; 
	
	rMainOut->opCode = 0x50; // lea eax, [mem]
	rMainOut->modifiers = 0;
	rMainOut->family = RIVER_FAMILY_PRETRACK | ((RIVER_REG_xSP == (reg.name & 0x07)) ? RIVER_FAMILY_FLAG_ORIG_xSP : 0);
	rMainOut->opTypes[0] = RIVER_OPTYPE_REG;
	rMainOut->operands[0].asRegister.versioned = reg.versioned;
	rMainOut->opTypes[1] = rMainOut->opTypes[2] = rMainOut->opTypes[3] = RIVER_OPTYPE_NONE;
	rMainOut->TrackEspAsParameter();
	rMainOut->TrackUnusedRegisters();
	rMainOut++;
	instrCount++;

	return trackedValues - 1;
}

void SymbopTranslator::MakeTrackReg(const RiverRegister &reg, RiverInstruction *&rTrackOut, nodep::DWORD &trackCount) {
	rTrackOut->opCode = 0x50; // push + r
	rTrackOut->opTypes[0] = RIVER_OPTYPE_REG;
	rTrackOut->operands[0].asRegister.versioned = reg.versioned;
	rTrackOut->opTypes[1] = rTrackOut->opTypes[2] = rTrackOut->opTypes[3] = RIVER_OPTYPE_NONE;
	rTrackOut->modifiers = 0;
	rTrackOut->family = RIVER_FAMILY_TRACK | ((RIVER_REG_xSP == (reg.name & 0x07)) ? RIVER_FAMILY_FLAG_ORIG_xSP : 0);
	rTrackOut->TrackEspAsParameter();
	rTrackOut->TrackUnusedRegisters();
	rTrackOut++;
	trackCount++;
}

void SymbopTranslator::MakeMarkReg(const RiverRegister &reg, nodep::DWORD addrOffset, nodep::DWORD valueOffset, RiverInstruction *&rMainOut, nodep::DWORD &instrCount, RiverInstruction *&rTrackOut, nodep::DWORD &trackCount) {
	rTrackOut->opCode = 0x58; // pop + r

	rTrackOut->opTypes[0] = RIVER_OPTYPE_REG;
	rTrackOut->operands[0].asRegister.versioned = reg.versioned;

	rTrackOut->opTypes[1] = rTrackOut->opTypes[2] = rTrackOut->opTypes[3] = RIVER_OPTYPE_NONE;

	rTrackOut->modifiers = 0;
	rTrackOut->family = RIVER_FAMILY_TRACK | ((RIVER_REG_xSP == (reg.name & 0x07)) ? RIVER_FAMILY_FLAG_ORIG_xSP : 0);

	rTrackOut->TrackEspAsParameter();
	rTrackOut->TrackUnusedRegisters();
	
	rTrackOut++;
	trackCount++;
}

nodep::DWORD SymbopTranslator::MakeTrackAddress(nodep::WORD specifiers, const RiverOperand &op, nodep::BYTE optype, RiverInstruction *&rMainOut, nodep::DWORD &instrCount, RiverInstruction *&rTrackOut, nodep::DWORD &trackCount, nodep::DWORD &valuesOut) {
	if (RIVER_OPTYPE_MEM != RIVER_OPTYPE(optype)) {
		return 0xFFFFFFFF;
	}

	if (0 == op.asAddress->type) {
		return 0xFFFFFFFF;
	}

	trackedValues += 1;
	if (op.asAddress->HasSegment()) {
		trackedValues += 1;
	}

	nodep::DWORD ret = trackedValues - 1;

	rMainOut->opCode = 0x8D;
	rMainOut->specifiers = 0;
	rMainOut->modifiers = 0;
	rMainOut->family = RIVER_FAMILY_PRETRACK;
	rMainOut->opTypes[0] = RIVER_OPTYPE_MEM;
	rMainOut->operands[0].asAddress = codegen->CloneAddress(*op.asAddress, 0);

	rMainOut->opTypes[1] = RIVER_OPTYPE_IMM | RIVER_OPSIZE_8;
	rMainOut->operands[1].asImm8 = (specifiers & RIVER_SPEC_IGNORES_MEMORY) ? 0 : 1;

	rMainOut->opTypes[2] = rMainOut->opTypes[3] = RIVER_OPTYPE_NONE;

	if (op.asAddress->type & RIVER_ADDR_BASE) {
		trackedValues++;
	}

	if (op.asAddress->type & RIVER_ADDR_INDEX) {
		trackedValues++;
	}

	if (0 == (specifiers & RIVER_SPEC_IGNORES_MEMORY)) {
		trackedValues += 2; // we push two operands on the stack [addr], [addr + 4]
		rMainOut->opTypes[2] = RIVER_OPTYPE_IMM | RIVER_OPSIZE_8;
		rMainOut->operands[2].asImm8 = trackedValues - 2;
	}
	
	rMainOut->PromoteModifiers();
	rMainOut->TrackEspAsParameter();
	rMainOut->TrackUnusedRegisters();
	rMainOut++;
	instrCount++;

	rTrackOut->opCode = 0x8D;
	rTrackOut->specifiers = 0;
	rTrackOut->modifiers = 0;
	rTrackOut->family = RIVER_FAMILY_TRACK;
	rTrackOut->opTypes[0] = RIVER_OPTYPE_MEM;
	rTrackOut->operands[0].asAddress = codegen->CloneAddress(*op.asAddress, 0);
	rTrackOut->opTypes[1] = rTrackOut->opTypes[2] = rTrackOut->opTypes[3] = RIVER_OPTYPE_NONE;
	rTrackOut->TrackEspAsParameter();
	rTrackOut->TrackUnusedRegisters();
	rTrackOut++;
	trackCount++;

	return ret;
}

/*DWORD SymbopTranslator::MakePreTrackMem(const RiverAddress &mem, WORD specifiers, DWORD addrOffset, RiverInstruction *&rMainOut, DWORD &instrCount) {
	if (0 == mem.type) {
		return MakePreTrackReg(mem.base, rMainOut, instrCount);
	}

	if (0 == (specifiers & RIVER_SPEC_IGNORES_MEMORY)) {
		trackedValues += 1;

		rMainOut->opCode = 0xFF; // lea eax, [mem]
		rMainOut->specifiers = 0;
		rMainOut->subOpCode = 6;
		rMainOut->modifiers = 0;
		rMainOut->family = RIVER_FAMILY_PRETRACK;
		rMainOut->opTypes[0] = RIVER_OPTYPE_MEM;
		rMainOut->operands[0].asAddress = codegen->CloneAddress(mem, 0);
		rMainOut->opTypes[1] = RIVER_OPTYPE_IMM | RIVER_OPSIZE_8;
		rMainOut->operands[1].asImm8 = trackedValues - 1;
		rMainOut->opTypes[2] = rMainOut->opTypes[3] = RIVER_OPTYPE_NONE;
		rMainOut->PromoteModifiers();
		rMainOut->TrackEspAsParameter();
		rMainOut->TrackUnusedRegisters();
		rMainOut++;
		instrCount++;

		return trackedValues - 1;
	}

	return 0xFFFFFFFF;
}*/

void SymbopTranslator::MakeTrackMem(const RiverAddress &mem, nodep::WORD specifiers, nodep::DWORD addrOffset, RiverInstruction *&rTrackOut, nodep::DWORD &trackCount) {
	if (0 == mem.type) {
		return MakeTrackReg(mem.base, rTrackOut, trackCount);
	}

	if (0 == (specifiers & RIVER_SPEC_IGNORES_MEMORY)) {
		rTrackOut->opCode = 0xFF;
		rTrackOut->specifiers = 0;
		rTrackOut->subOpCode = 0x06;
		rTrackOut->modifiers = 0;
		rTrackOut->family = RIVER_FAMILY_TRACK;
		rTrackOut->opTypes[0] = RIVER_OPTYPE_MEM;
		rTrackOut->operands[0].asAddress = codegen->CloneAddress(mem, 0);
		rTrackOut->opTypes[1] = RIVER_OPTYPE_IMM | RIVER_OPSIZE_8;
		rTrackOut->operands[1].asImm8 = (nodep::BYTE)addrOffset;
		rTrackOut->opTypes[2] = rTrackOut->opTypes[3] = RIVER_OPTYPE_NONE;
		rTrackOut->TrackEspAsParameter();
		rTrackOut->TrackUnusedRegisters();
		rTrackOut++;
		trackCount++;
	}
}

void SymbopTranslator::MakeMarkMem(const RiverAddress &mem, nodep::WORD specifiers, nodep::DWORD addrOffset, nodep::DWORD valueOffset, RiverInstruction *&rMainOut, nodep::DWORD &instrCount, RiverInstruction *&rTrackOut, nodep::DWORD &trackCount) {
	if (0 == mem.type) {
		MakeMarkReg(mem.base, addrOffset, valueOffset, rMainOut, instrCount, rTrackOut, trackCount);
		return;
	}

	if (0 == (specifiers & RIVER_SPEC_IGNORES_MEMORY)) {
		rTrackOut->opCode = 0x8F;
		rTrackOut->subOpCode = 0x00;
		rTrackOut->modifiers = 0;
		rTrackOut->family = RIVER_FAMILY_TRACK;
		rTrackOut->opTypes[0] = RIVER_OPTYPE_MEM;
		rTrackOut->operands[0].asAddress = codegen->CloneAddress(mem, 0);
		rTrackOut->opTypes[1] = RIVER_OPTYPE_IMM | RIVER_OPSIZE_8;
		rTrackOut->operands[1].asImm8 = (nodep::BYTE)addrOffset;
		rTrackOut->opTypes[2] = rTrackOut->opTypes[3] = RIVER_OPTYPE_NONE;
		rTrackOut->TrackEspAsParameter();
		rTrackOut->TrackUnusedRegisters();

		rTrackOut++;
		trackCount++;
	}
}

/*void SymbopTranslator::MakeSkipMem(const RiverAddress &mem, RiverInstruction *&rMainOut, DWORD &instrCount, RiverInstruction *&rTrackOut, DWORD &trackCount) {
	rTrackOut->opCode = 0x8F;
	rTrackOut->subOpCode = 0x07;
	rTrackOut->modifiers = 0;
	rTrackOut->family = RIVER_FAMILY_TRACK;
	rTrackOut->opTypes[0] = RIVER_OPTYPE_MEM;
	rTrackOut->operands[0].asAddress = codegen->CloneAddress(mem, 0);
	rTrackOut->opTypes[1] = rTrackOut->opTypes[2] = rTrackOut->opTypes[3] = RIVER_OPTYPE_NONE;
	rTrackOut->TrackEspAsParameter();
	rTrackOut->TrackUnusedRegisters();

	rTrackOut++;
	trackCount++;
}*/

void SymbopTranslator::MakeTrackOp(nodep::DWORD opIdx, const nodep::BYTE type, const RiverOperand &op, nodep::WORD specifiers, nodep::DWORD addrOffset, RiverInstruction *&rMainOut, nodep::DWORD &instrCount, RiverInstruction *&rTrackOut, nodep::DWORD &trackCount, nodep::DWORD &valueOffset) {
	switch (RIVER_OPTYPE(type)) {
	case RIVER_OPTYPE_IMM :
		valueOffset = 0xFFFFFFFF;
		break;
	case RIVER_OPTYPE_REG :
		//if ((0 == (RIVER_SPEC_IGNORES_OP(opIdx) & specifiers))) {
			valueOffset = MakePreTrackReg(op.asRegister, rMainOut, instrCount);
		//}
		MakeTrackReg(op.asRegister, rTrackOut, trackCount);
		break;
	case RIVER_OPTYPE_MEM:
		if (0 == op.asAddress->type) {
			//if ((0 == (RIVER_SPEC_IGNORES_OP(opIdx) & specifiers))) {
				valueOffset = MakePreTrackReg(op.asAddress->base, rMainOut, instrCount);
			//}
			MakeTrackReg(op.asAddress->base, rTrackOut, trackCount);
		} else {
			MakeTrackMem(*op.asAddress, specifiers, addrOffset, rTrackOut, trackCount);
		}
		break;
	default : 
		valueOffset = 0xFFFFFFFF;
	}
}

void SymbopTranslator::MakeMarkOp(const nodep::BYTE type, nodep::WORD specifiers, nodep::DWORD addrOffset, nodep::DWORD valueOffset, const RiverOperand &op, RiverInstruction *&rMainOut, nodep::DWORD &instrCount, RiverInstruction *&rTrackOut, nodep::DWORD &trackCount) {
	switch (RIVER_OPTYPE(type)) {
	case RIVER_OPTYPE_IMM:
		break;
	case RIVER_OPTYPE_REG:
		MakeMarkReg(op.asRegister, addrOffset, valueOffset, rMainOut, instrCount, rTrackOut, trackCount);
		break;
	case RIVER_OPTYPE_MEM:
		MakeMarkMem(*op.asAddress, specifiers, addrOffset, valueOffset, rMainOut, instrCount, rTrackOut, trackCount);
		break;
	}
}

void SymbopTranslator::MakeCallSymbolic(const RiverInstruction &rIn, RiverInstruction *&rMainOut, nodep::DWORD &instrCount, RiverInstruction *&rTrackOut, nodep::DWORD &trackCount) {
	rTrackOut->opCode = 0xE8;
	rTrackOut->subOpCode = 0x00;
	rTrackOut->modifiers = 0;
	rTrackOut->family = RIVER_FAMILY_TRACK;
	rTrackOut->opTypes[0] = RIVER_OPTYPE_IMM | RIVER_OPSIZE_32;
	rTrackOut->operands[0].asImm32 = rIn.instructionAddress;
	rTrackOut->opTypes[1] = rTrackOut->opTypes[2] = rTrackOut->opTypes[3] = RIVER_OPTYPE_NONE;
	rTrackOut->TrackEspAsParameter();
	rTrackOut->TrackUnusedRegisters();

	rTrackOut++;
	trackCount++;
}

void SymbopTranslator::TranslateUnk(const RiverInstruction &rIn, RiverInstruction *&rMainOut, nodep::DWORD &instrCount, RiverInstruction *&rTrackOut, nodep::DWORD &trackCount, nodep::DWORD dwTranslationFlags) {
	static nodep::BYTE lastOpcode;
	static nodep::DWORD lastAddr;

	lastOpcode = rIn.opCode;
	lastAddr = rIn.instructionAddress;
	revtracerImports.dbgPrintFunc(PRINT_ERROR | PRINT_DISASSEMBLY, "Translating unknown instruction %02x %02x \n", rIn.modifiers & RIVER_MODIFIER_EXT ? 0x0F : 0x00, lastOpcode);

	DEBUG_BREAK;
}

void SymbopTranslator::TranslateDefault(const RiverInstruction &rIn, RiverInstruction *&rMainOut, nodep::DWORD &instrCount, RiverInstruction *&rTrackOut, nodep::DWORD &trackCount, nodep::DWORD dwTranslationFlags) {
	nodep::DWORD addressOffsets[4] = { 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF };
	nodep::DWORD valueOffsets[4] = { 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF };
	nodep::DWORD flagOffset = 0xFFFFFFFF;

	MakeInitTrack(rIn, rTrackOut, trackCount);

	if ((0 == (RIVER_SPEC_IGNORES_FLG & rIn.specifiers)) || (rIn.modFlags)) {
		// TODO: flag unset for ignored flags!
		flagOffset = MakeTrackFlg(rIn.testFlags | rIn.modFlags, rMainOut, instrCount, rTrackOut, trackCount);
	}

	for (int i = 3; i >= 0; --i) {
		addressOffsets[i] = MakeTrackAddress(rIn.specifiers, rIn.operands[i], rIn.opTypes[i], rMainOut, instrCount, rTrackOut, trackCount, valueOffsets[i]);
	}

	for (int i = 3; i >= 0; --i) {
		if (RIVER_OPTYPE_NONE != rIn.opTypes[i]) {
			MakeTrackOp(i, rIn.opTypes[i], rIn.operands[i], rIn.specifiers, addressOffsets[i], rMainOut, instrCount, rTrackOut, trackCount, valueOffsets[i]);
		}
	}

	// make opcode
	CopyInstruction(codegen, *rMainOut, rIn);
	rMainOut++;
	instrCount++;

	if (RIVER_SPEC_MODIFIES_FLG & rIn.specifiers) {
		MakeMarkFlg(rIn.modFlags, flagOffset, rMainOut, instrCount, rTrackOut, trackCount);
	}

	for (int i = 3; i >= 0; --i) {
		if ((RIVER_OPTYPE_NONE != rIn.opTypes[i]) && (RIVER_SPEC_MODIFIES_OP(i) & rIn.specifiers)) {
			MakeMarkOp(rIn.opTypes[i], rIn.specifiers, addressOffsets[i], valueOffsets[i], rIn.operands[i], rMainOut, instrCount, rTrackOut, trackCount);
		}
	}

	if (dwTranslationFlags & TRACER_FEATURE_ADVANCED_TRACKING) {
		MakeCallSymbolic(rIn, rMainOut, instrCount, rTrackOut, trackCount);
	}

	MakeCleanTrack(rTrackOut, trackCount);
}






SymbopTranslator::TranslateOpcodeFunc SymbopTranslator::translateOpcodes[2][0x100] = {
	{
		/* 0x00 */ &SymbopTranslator::TranslateDefault, &SymbopTranslator::TranslateDefault, &SymbopTranslator::TranslateDefault, &SymbopTranslator::TranslateDefault,
		/* 0x04 */ &SymbopTranslator::TranslateDefault, &SymbopTranslator::TranslateDefault, &SymbopTranslator::TranslateDefault, &SymbopTranslator::TranslateDefault,
		/* 0x08 */ &SymbopTranslator::TranslateDefault, &SymbopTranslator::TranslateDefault, &SymbopTranslator::TranslateDefault, &SymbopTranslator::TranslateDefault,
		/* 0x0C */ &SymbopTranslator::TranslateDefault, &SymbopTranslator::TranslateDefault, &SymbopTranslator::TranslateDefault, &SymbopTranslator::TranslateDefault,

		/* 0x10 */ &SymbopTranslator::TranslateDefault, &SymbopTranslator::TranslateDefault, &SymbopTranslator::TranslateDefault, &SymbopTranslator::TranslateDefault,
		/* 0x14 */ &SymbopTranslator::TranslateDefault, &SymbopTranslator::TranslateDefault, &SymbopTranslator::TranslateDefault, &SymbopTranslator::TranslateDefault,
		/* 0x18 */ &SymbopTranslator::TranslateDefault, &SymbopTranslator::TranslateDefault, &SymbopTranslator::TranslateDefault, &SymbopTranslator::TranslateDefault,
		/* 0x1C */ &SymbopTranslator::TranslateDefault, &SymbopTranslator::TranslateDefault, &SymbopTranslator::TranslateDefault, &SymbopTranslator::TranslateDefault,

		/* 0x20 */ &SymbopTranslator::TranslateDefault, &SymbopTranslator::TranslateDefault, &SymbopTranslator::TranslateDefault, &SymbopTranslator::TranslateDefault,
		/* 0x24 */ &SymbopTranslator::TranslateDefault, &SymbopTranslator::TranslateDefault, &SymbopTranslator::TranslateDefault, &SymbopTranslator::TranslateDefault,
		/* 0x28 */ &SymbopTranslator::TranslateDefault, &SymbopTranslator::TranslateDefault, &SymbopTranslator::TranslateDefault, &SymbopTranslator::TranslateDefault,
		/* 0x2C */ &SymbopTranslator::TranslateDefault, &SymbopTranslator::TranslateDefault, &SymbopTranslator::TranslateDefault, &SymbopTranslator::TranslateDefault,

		/* 0x30 */ &SymbopTranslator::TranslateDefault, &SymbopTranslator::TranslateDefault, &SymbopTranslator::TranslateDefault, &SymbopTranslator::TranslateDefault,
		/* 0x34 */ &SymbopTranslator::TranslateDefault, &SymbopTranslator::TranslateDefault, &SymbopTranslator::TranslateDefault, &SymbopTranslator::TranslateDefault,
		/* 0x38 */ &SymbopTranslator::TranslateDefault, &SymbopTranslator::TranslateDefault, &SymbopTranslator::TranslateDefault, &SymbopTranslator::TranslateDefault,
		/* 0x3C */ &SymbopTranslator::TranslateDefault, &SymbopTranslator::TranslateDefault, &SymbopTranslator::TranslateDefault, &SymbopTranslator::TranslateDefault,

		/* 0x40 */ &SymbopTranslator::TranslateDefault, &SymbopTranslator::TranslateDefault, &SymbopTranslator::TranslateDefault, &SymbopTranslator::TranslateDefault,
		/* 0x44 */ &SymbopTranslator::TranslateDefault, &SymbopTranslator::TranslateDefault, &SymbopTranslator::TranslateDefault, &SymbopTranslator::TranslateDefault,
		/* 0x48 */ &SymbopTranslator::TranslateDefault, &SymbopTranslator::TranslateDefault, &SymbopTranslator::TranslateDefault, &SymbopTranslator::TranslateDefault,
		/* 0x4C */ &SymbopTranslator::TranslateDefault, &SymbopTranslator::TranslateDefault, &SymbopTranslator::TranslateDefault, &SymbopTranslator::TranslateDefault,

		/* not really default */
		/* 0x50 */ &SymbopTranslator::TranslateDefault, &SymbopTranslator::TranslateDefault, &SymbopTranslator::TranslateDefault, &SymbopTranslator::TranslateDefault,
		/* 0x54 */ &SymbopTranslator::TranslateDefault, &SymbopTranslator::TranslateDefault, &SymbopTranslator::TranslateDefault, &SymbopTranslator::TranslateDefault,
		/* 0x58 */ &SymbopTranslator::TranslateDefault, &SymbopTranslator::TranslateDefault, &SymbopTranslator::TranslateDefault, &SymbopTranslator::TranslateDefault,
		/* 0x5C */ &SymbopTranslator::TranslateDefault, &SymbopTranslator::TranslateDefault, &SymbopTranslator::TranslateDefault, &SymbopTranslator::TranslateDefault,

		/* 0x60 */ &SymbopTranslator::TranslateUnk, &SymbopTranslator::TranslateUnk, &SymbopTranslator::TranslateUnk, &SymbopTranslator::TranslateUnk,
		/* 0x64 */ &SymbopTranslator::TranslateDefault, &SymbopTranslator::TranslateDefault, &SymbopTranslator::TranslateDefault, &SymbopTranslator::TranslateDefault,
		/* 0x68 */ &SymbopTranslator::TranslateDefault, &SymbopTranslator::TranslateDefault, &SymbopTranslator::TranslateDefault, &SymbopTranslator::TranslateDefault,
		/* 0x6C */ &SymbopTranslator::TranslateUnk, &SymbopTranslator::TranslateUnk, &SymbopTranslator::TranslateUnk, &SymbopTranslator::TranslateUnk,

		/* 0x70 */ &SymbopTranslator::TranslateDefault, &SymbopTranslator::TranslateDefault, &SymbopTranslator::TranslateDefault, &SymbopTranslator::TranslateDefault,
		/* 0x74 */ &SymbopTranslator::TranslateDefault, &SymbopTranslator::TranslateDefault, &SymbopTranslator::TranslateDefault, &SymbopTranslator::TranslateDefault,
		/* 0x78 */ &SymbopTranslator::TranslateDefault, &SymbopTranslator::TranslateDefault, &SymbopTranslator::TranslateDefault, &SymbopTranslator::TranslateDefault,
		/* 0x7C */ &SymbopTranslator::TranslateDefault, &SymbopTranslator::TranslateDefault, &SymbopTranslator::TranslateDefault, &SymbopTranslator::TranslateDefault,

		/* 0x80 */ &SymbopTranslator::TranslateDefault, &SymbopTranslator::TranslateDefault, &SymbopTranslator::TranslateDefault, &SymbopTranslator::TranslateDefault,
		/* 0x84 */ &SymbopTranslator::TranslateDefault, &SymbopTranslator::TranslateDefault, &SymbopTranslator::TranslateDefault, &SymbopTranslator::TranslateDefault,
		/* 0x88 */ &SymbopTranslator::TranslateDefault, &SymbopTranslator::TranslateDefault, &SymbopTranslator::TranslateDefault, &SymbopTranslator::TranslateDefault,
		/* 0x8C */ &SymbopTranslator::TranslateUnk, &SymbopTranslator::TranslateDefault, &SymbopTranslator::TranslateUnk, &SymbopTranslator::TranslateUnk,

		/* 0x90 */ &SymbopTranslator::TranslateDefault, &SymbopTranslator::TranslateDefault, &SymbopTranslator::TranslateDefault, &SymbopTranslator::TranslateDefault,
		/* 0x94 */ &SymbopTranslator::TranslateDefault, &SymbopTranslator::TranslateDefault, &SymbopTranslator::TranslateDefault, &SymbopTranslator::TranslateDefault,
		/* 0x98 */ &SymbopTranslator::TranslateUnk, &SymbopTranslator::TranslateDefault, &SymbopTranslator::TranslateUnk, &SymbopTranslator::TranslateUnk,
		/* 0x9C */ &SymbopTranslator::TranslateUnk, &SymbopTranslator::TranslateUnk, &SymbopTranslator::TranslateUnk, &SymbopTranslator::TranslateUnk,

		/* 0xA0 */ &SymbopTranslator::TranslateDefault, &SymbopTranslator::TranslateDefault, &SymbopTranslator::TranslateDefault, &SymbopTranslator::TranslateDefault,
		/* 0xA4 */ &SymbopTranslator::TranslateDefault, &SymbopTranslator::TranslateDefault, &SymbopTranslator::TranslateDefault, &SymbopTranslator::TranslateDefault,
		/* 0xA8 */ &SymbopTranslator::TranslateDefault, &SymbopTranslator::TranslateDefault, &SymbopTranslator::TranslateDefault, &SymbopTranslator::TranslateDefault,
		/* 0xAC */ &SymbopTranslator::TranslateUnk, &SymbopTranslator::TranslateUnk, &SymbopTranslator::TranslateDefault, &SymbopTranslator::TranslateDefault,

		/* 0xB0 */ &SymbopTranslator::TranslateDefault, &SymbopTranslator::TranslateDefault, &SymbopTranslator::TranslateDefault, &SymbopTranslator::TranslateDefault,
		/* 0xB4 */ &SymbopTranslator::TranslateDefault, &SymbopTranslator::TranslateDefault, &SymbopTranslator::TranslateDefault, &SymbopTranslator::TranslateDefault,
		/* 0xB8 */ &SymbopTranslator::TranslateDefault, &SymbopTranslator::TranslateDefault, &SymbopTranslator::TranslateDefault, &SymbopTranslator::TranslateDefault,
		/* 0xBC */ &SymbopTranslator::TranslateDefault, &SymbopTranslator::TranslateDefault, &SymbopTranslator::TranslateDefault, &SymbopTranslator::TranslateDefault,

		/* 0xC0 */ &SymbopTranslator::TranslateDefault, &SymbopTranslator::TranslateDefault, &SymbopTranslator::TranslateDefault, &SymbopTranslator::TranslateDefault,
		/* 0xC4 */ &SymbopTranslator::TranslateUnk, &SymbopTranslator::TranslateUnk, &SymbopTranslator::TranslateDefault, &SymbopTranslator::TranslateDefault,
		/* 0xC8 */ &SymbopTranslator::TranslateDefault, &SymbopTranslator::TranslateDefault, &SymbopTranslator::TranslateDefault, &SymbopTranslator::TranslateDefault,
		/* 0xCC */ &SymbopTranslator::TranslateDefault, &SymbopTranslator::TranslateUnk, &SymbopTranslator::TranslateDefault, &SymbopTranslator::TranslateDefault,

		/* 0xD0 */ &SymbopTranslator::TranslateDefault, &SymbopTranslator::TranslateDefault, &SymbopTranslator::TranslateDefault, &SymbopTranslator::TranslateDefault,
		/* 0xD4 */ &SymbopTranslator::TranslateUnk, &SymbopTranslator::TranslateUnk, &SymbopTranslator::TranslateUnk, &SymbopTranslator::TranslateUnk,
		/* 0xD8 */ &SymbopTranslator::TranslateUnk, &SymbopTranslator::TranslateUnk, &SymbopTranslator::TranslateUnk, &SymbopTranslator::TranslateUnk,
		/* 0xDC */ &SymbopTranslator::TranslateUnk, &SymbopTranslator::TranslateUnk, &SymbopTranslator::TranslateUnk, &SymbopTranslator::TranslateUnk,

		/* 0xE0 */ &SymbopTranslator::TranslateDefault, &SymbopTranslator::TranslateDefault, &SymbopTranslator::TranslateDefault, &SymbopTranslator::TranslateDefault,
		/* 0xE4 */ &SymbopTranslator::TranslateUnk, &SymbopTranslator::TranslateUnk, &SymbopTranslator::TranslateUnk, &SymbopTranslator::TranslateUnk,
		/* 0xE8 */ &SymbopTranslator::TranslateDefault, &SymbopTranslator::TranslateDefault, &SymbopTranslator::TranslateDefault, &SymbopTranslator::TranslateDefault,
		/* 0xEC */ &SymbopTranslator::TranslateUnk, &SymbopTranslator::TranslateUnk, &SymbopTranslator::TranslateUnk, &SymbopTranslator::TranslateUnk,

		/* 0xF0 */ &SymbopTranslator::TranslateDefault, &SymbopTranslator::TranslateUnk, &SymbopTranslator::TranslateDefault, &SymbopTranslator::TranslateDefault,
		/* 0xF4 */ &SymbopTranslator::TranslateUnk, &SymbopTranslator::TranslateUnk, &SymbopTranslator::TranslateDefault, &SymbopTranslator::TranslateDefault,
		/* 0xF8 */ &SymbopTranslator::TranslateUnk, &SymbopTranslator::TranslateUnk, &SymbopTranslator::TranslateUnk, &SymbopTranslator::TranslateUnk,
		/* 0xFC */ &SymbopTranslator::TranslateDefault, &SymbopTranslator::TranslateDefault, &SymbopTranslator::TranslateDefault, &SymbopTranslator::TranslateDefault
	}, {
		/* 0x00 */ &SymbopTranslator::TranslateUnk, &SymbopTranslator::TranslateUnk, &SymbopTranslator::TranslateDefault, &SymbopTranslator::TranslateDefault,
		/* 0x04 */ &SymbopTranslator::TranslateUnk, &SymbopTranslator::TranslateDefault, &SymbopTranslator::TranslateUnk, &SymbopTranslator::TranslateUnk,
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

		/* 0x30 */ &SymbopTranslator::TranslateUnk, &SymbopTranslator::TranslateDefault, &SymbopTranslator::TranslateUnk, &SymbopTranslator::TranslateUnk,
		/* 0x34 */ &SymbopTranslator::TranslateDefault, &SymbopTranslator::TranslateUnk, &SymbopTranslator::TranslateUnk, &SymbopTranslator::TranslateUnk,
		/* 0x38 */ &SymbopTranslator::TranslateUnk, &SymbopTranslator::TranslateUnk, &SymbopTranslator::TranslateUnk, &SymbopTranslator::TranslateUnk,
		/* 0x3C */ &SymbopTranslator::TranslateUnk, &SymbopTranslator::TranslateUnk, &SymbopTranslator::TranslateUnk, &SymbopTranslator::TranslateUnk,

		/* 0x40 */ &SymbopTranslator::TranslateDefault, &SymbopTranslator::TranslateDefault, &SymbopTranslator::TranslateDefault, &SymbopTranslator::TranslateDefault,
		/* 0x44 */ &SymbopTranslator::TranslateDefault, &SymbopTranslator::TranslateDefault, &SymbopTranslator::TranslateDefault, &SymbopTranslator::TranslateDefault,
		/* 0x48 */ &SymbopTranslator::TranslateDefault, &SymbopTranslator::TranslateDefault, &SymbopTranslator::TranslateDefault, &SymbopTranslator::TranslateDefault,
		/* 0x4C */ &SymbopTranslator::TranslateDefault, &SymbopTranslator::TranslateDefault, &SymbopTranslator::TranslateDefault, &SymbopTranslator::TranslateDefault,

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

		/* 0x80 */ &SymbopTranslator::TranslateDefault, &SymbopTranslator::TranslateDefault, &SymbopTranslator::TranslateDefault, &SymbopTranslator::TranslateDefault,
		/* 0x84 */ &SymbopTranslator::TranslateDefault, &SymbopTranslator::TranslateDefault, &SymbopTranslator::TranslateDefault, &SymbopTranslator::TranslateDefault,
		/* 0x88 */ &SymbopTranslator::TranslateDefault, &SymbopTranslator::TranslateDefault, &SymbopTranslator::TranslateDefault, &SymbopTranslator::TranslateDefault,
		/* 0x8C */ &SymbopTranslator::TranslateDefault, &SymbopTranslator::TranslateDefault, &SymbopTranslator::TranslateDefault, &SymbopTranslator::TranslateDefault,

		/* 0x90 */ &SymbopTranslator::TranslateDefault, &SymbopTranslator::TranslateDefault, &SymbopTranslator::TranslateDefault, &SymbopTranslator::TranslateDefault,
		/* 0x94 */ &SymbopTranslator::TranslateDefault, &SymbopTranslator::TranslateDefault, &SymbopTranslator::TranslateDefault, &SymbopTranslator::TranslateDefault,
		/* 0x98 */ &SymbopTranslator::TranslateDefault, &SymbopTranslator::TranslateDefault, &SymbopTranslator::TranslateDefault, &SymbopTranslator::TranslateDefault,
		/* 0x9C */ &SymbopTranslator::TranslateDefault, &SymbopTranslator::TranslateDefault, &SymbopTranslator::TranslateDefault, &SymbopTranslator::TranslateDefault,

		/* 0xA0 */ &SymbopTranslator::TranslateUnk, &SymbopTranslator::TranslateUnk, &SymbopTranslator::TranslateDefault, &SymbopTranslator::TranslateDefault,
		/* 0xA4 */ &SymbopTranslator::TranslateDefault, &SymbopTranslator::TranslateDefault, &SymbopTranslator::TranslateUnk, &SymbopTranslator::TranslateUnk,
		/* 0xA8 */ &SymbopTranslator::TranslateUnk, &SymbopTranslator::TranslateUnk, &SymbopTranslator::TranslateUnk, &SymbopTranslator::TranslateDefault,
		/* 0xAC */ &SymbopTranslator::TranslateDefault, &SymbopTranslator::TranslateDefault, &SymbopTranslator::TranslateUnk, &SymbopTranslator::TranslateDefault,

		/* 0xB0 */ &SymbopTranslator::TranslateDefault, &SymbopTranslator::TranslateDefault, &SymbopTranslator::TranslateUnk, &SymbopTranslator::TranslateDefault,
		/* 0xB4 */ &SymbopTranslator::TranslateUnk, &SymbopTranslator::TranslateUnk, &SymbopTranslator::TranslateDefault, &SymbopTranslator::TranslateDefault,
		/* 0xB8 */ &SymbopTranslator::TranslateUnk, &SymbopTranslator::TranslateUnk, &SymbopTranslator::TranslateDefault, &SymbopTranslator::TranslateUnk,
		/* 0xBC */ &SymbopTranslator::TranslateDefault, &SymbopTranslator::TranslateDefault, &SymbopTranslator::TranslateDefault, &SymbopTranslator::TranslateDefault,

		/* 0xC0 */ &SymbopTranslator::TranslateDefault, &SymbopTranslator::TranslateDefault, &SymbopTranslator::TranslateUnk, &SymbopTranslator::TranslateUnk,
		/* 0xC4 */ &SymbopTranslator::TranslateUnk, &SymbopTranslator::TranslateUnk, &SymbopTranslator::TranslateUnk, &SymbopTranslator::TranslateDefault,
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
