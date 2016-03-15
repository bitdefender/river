#include "SymbopReverseTranslator.h"
#include "CodeGen.h"

void SymbopReverseTranslator::CopyInstruction(RiverInstruction &rOut, const RiverInstruction &rIn) {
	memcpy(&rOut, &rIn, sizeof(rOut));

	if (RIVER_OPTYPE_MEM == RIVER_OPTYPE(rIn.opTypes[0])) {
		rOut.operands[0].asAddress = codegen->CloneAddress(*rIn.operands[0].asAddress, rIn.modifiers);
	}

	if (RIVER_OPTYPE_MEM == RIVER_OPTYPE(rIn.opTypes[1])) {
		rOut.operands[1].asAddress = codegen->CloneAddress(*rIn.operands[1].asAddress, rIn.modifiers);
	}
}

bool SymbopReverseTranslator::Init(RiverCodeGen *cg) {
	codegen = cg;
	return true;
}

void SymbopReverseTranslator::Translate(const RiverInstruction &rIn, RiverInstruction &rOut) {
	if ((RIVER_FAMILY_RIVER_TRACK != RIVER_FAMILY(rIn.family)) &&
		(RIVER_FAMILY_TRACK != RIVER_FAMILY(rIn.family))) {
		__asm int 3;
	}

	if (RIVER_FAMILY_RIVER_TRACK == RIVER_FAMILY(rIn.family)) {
		switch (rIn.opCode) {
			case 0x9C : // pushf
				TranslatePushFlg(rOut, rIn);
				break;
			case 0x50 : // push reg
				TranslatePushReg(rOut, rIn);
				break;
			case 0xFF : // push mem
				if (6 == rIn.subOpCode) {
					TranslatePushMem(rOut, rIn);
				} else {
					__asm int 3;
				}

				break;

			case 0x9D :
			case 0x58 :
			case 0x8F :
				__asm int 3;
				break;
			default :
				CopyInstruction(rOut, rIn);
				rOut.family |= RIVER_FAMILY_FLAG_IGNORE;
				break;
		};
	} else if (RIVER_FAMILY_TRACK == RIVER_FAMILY(rIn.family)) {
		switch (rIn.opCode) {
			case 0x8F :
				CopyInstruction(rOut, rIn);
				break;
			default :
				CopyInstruction(rOut, rIn);
				rOut.family |= RIVER_FAMILY_FLAG_IGNORE;
				break;
		}
	} else {
		CopyInstruction(rOut, rIn);
		rOut.family |= RIVER_FAMILY_FLAG_IGNORE;
	}
}

void SymbopReverseTranslator::TranslatePushReg(RiverInstruction &rOut, const RiverInstruction &rIn) {
	CopyInstruction(rOut, rIn);
	rOut.opCode += 8;
}

void SymbopReverseTranslator::TranslatePushFlg(RiverInstruction &rOut, const RiverInstruction &rIn) {
	CopyInstruction(rOut, rIn);
	rOut.opCode += 1;
}

void SymbopReverseTranslator::TranslatePushMem(RiverInstruction &rOut, const RiverInstruction &rIn) {
	CopyInstruction(rOut, rIn);
	rOut.opCode = 0x8F;
	rOut.subOpCode = 0;
}
