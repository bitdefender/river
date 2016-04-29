#include "SymbopSaveTranslator.h"

#include "CodeGen.h"

#include "SymbolicEnvironment.h"

#include <stdint.h>

void SymbopSaveTranslator::CopyInstruction(RiverInstruction &rOut, const RiverInstruction &rIn) {
	memcpy(&rOut, &rIn, sizeof(rOut));

	for (int i = 0; i < 4; ++i) {
		if (RIVER_OPTYPE_MEM == RIVER_OPTYPE(rIn.opTypes[i])) {
			rOut.operands[i].asAddress = codegen->CloneAddress(*rIn.operands[i].asAddress, rIn.modifiers);
		}
	}
}

bool SymbopSaveTranslator::Init(RiverCodeGen *cg) {
	codegen = cg;
	return true;

}

void SymbopSaveTranslator::MakePushFlg(BYTE flags, RiverInstruction *&rOut, DWORD &instrCount) {
	rOut->opCode = 0x9C;
	rOut->subOpCode = 0;
	rOut->modifiers = 0;
	rOut->family = RIVER_FAMILY_RIVER_TRACK;

	rOut->opTypes[0] = RIVER_OPTYPE_IMM | RIVER_OPSIZE_8;
	rOut->operands[0].asImm8 = flags;

	rOut->opTypes[1] = rOut->opTypes[2] = rOut->opTypes[3] = RIVER_OPTYPE_NONE;

	rOut++;
	instrCount++;
}

void SymbopSaveTranslator::MakePushReg(RiverRegister reg, RiverInstruction *&rOut, DWORD &instrCount) {
	rOut->opCode = 0x50;
	rOut->subOpCode = 0;
	rOut->modifiers = 0;
	rOut->family = RIVER_FAMILY_RIVER_TRACK;

	rOut->opTypes[0] = RIVER_OPTYPE_REG | RIVER_OPSIZE_32;
	rOut->operands[0].asRegister.versioned = reg.versioned;

	rOut->opTypes[1] = rOut->opTypes[2] = rOut->opTypes[3] = RIVER_OPTYPE_NONE;

	rOut++;
	instrCount++;
}

void SymbopSaveTranslator::MakePushMem(const RiverInstruction &rIn, RiverInstruction *&rOut, DWORD &instrCount) {
	rOut->opCode = 0xFF;
	rOut->subOpCode = 6;
	rOut->modifiers = 0;
	rOut->family = RIVER_FAMILY_RIVER_TRACK;

	rOut->opTypes[0] = RIVER_OPTYPE_MEM | RIVER_OPSIZE_32;
	rOut->operands[0].asAddress = codegen->CloneAddress(*rIn.operands[0].asAddress, rIn.modifiers);

	rOut->opTypes[1] = RIVER_OPTYPE_IMM | RIVER_OPSIZE_8;
	rOut->operands[1].asImm8 = rIn.operands[1].asImm8;
	
	rOut->opTypes[2] = rOut->opTypes[3] = RIVER_OPTYPE_NONE;

	rOut++;
	instrCount++;
}

bool SymbopSaveTranslator::Translate(const RiverInstruction &rIn, RiverInstruction *rOut, DWORD &instrCount) {
	if (RIVER_FAMILY_TRACK != RIVER_FAMILY(rIn.family)) {
		__asm int 3;
	}

	switch (rIn.opCode) {
		case 0x9D : //markflags
			MakePushFlg(rIn.operands[0].asImm8, rOut, instrCount);
			break;
		case 0x58 : //markreg
			MakePushReg(rIn.operands[0].asRegister, rOut, instrCount);
			break;
	}

	CopyInstruction(*rOut, rIn);
	rOut++;
	instrCount++;

	switch (rIn.opCode) {
		case 0x8F: //markmem
			MakePushMem(rIn, rOut, instrCount);
			break;
	}

	return true;
}
