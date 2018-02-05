#include "SymbopReverseTranslator.h"
#include "CodeGen.h"
#include "TranslatorUtil.h"

bool SymbopReverseTranslator::Init(RiverCodeGen *cg) {
	codegen = cg;
	return true;
}

bool SymbopReverseTranslator::Translate(const RiverInstruction &rIn, RiverInstruction &rOut) {
	if ((RIVER_FAMILY_RIVER_TRACK != RIVER_FAMILY(rIn.family)) &&
		(RIVER_FAMILY_TRACK != RIVER_FAMILY(rIn.family))) {
		return false;
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
					return false;
				}

				break;

			case 0x9D :
			case 0x58 :
			case 0x8F :
				return false;
				break;
			default :
				CopyInstruction(codegen, rOut, rIn);
				rOut.family |= RIVER_FAMILY_FLAG_IGNORE;
				break;
		};
	} else if (RIVER_FAMILY_TRACK == RIVER_FAMILY(rIn.family)) {
		switch (rIn.opCode) {
			case 0x8F :
				CopyInstruction(codegen, rOut, rIn);
				break;
			default :
				CopyInstruction(codegen, rOut, rIn);
				rOut.family |= RIVER_FAMILY_FLAG_IGNORE;
				break;
		}
	} else {
		CopyInstruction(codegen, rOut, rIn);
		rOut.family |= RIVER_FAMILY_FLAG_IGNORE;
	}

	return true;
}

void SymbopReverseTranslator::TranslatePushReg(RiverInstruction &rOut, const RiverInstruction &rIn) {
	CopyInstruction(codegen, rOut, rIn);
	rOut.opCode += 8;
}

void SymbopReverseTranslator::TranslatePushFlg(RiverInstruction &rOut, const RiverInstruction &rIn) {
	CopyInstruction(codegen, rOut, rIn);
	rOut.opCode += 1;
}

void SymbopReverseTranslator::TranslatePushMem(RiverInstruction &rOut, const RiverInstruction &rIn) {
	CopyInstruction(codegen, rOut, rIn);
	rOut.opCode = 0x8F;
	rOut.subOpCode = 0;
}
