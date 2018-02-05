#include "TranslatorUtil.h"
#include "CodeGen.h"

void CopyInstruction(RiverCodeGen *codegen, RiverInstruction &rOut, const RiverInstruction &rIn) {
	rev_memcpy(&rOut, &rIn, sizeof(rOut));

	for (int i = 0; i < 4; ++i) {
		if (RIVER_OPTYPE_MEM == RIVER_OPTYPE(rIn.opTypes[i])) {
			rOut.operands[i].asAddress = codegen->CloneAddress(*rIn.operands[i].asAddress, rIn.modifiers);
		}
	}
}
