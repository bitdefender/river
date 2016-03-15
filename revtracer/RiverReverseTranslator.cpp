#include "CodeGen.h"

void RiverReverseTranslator::CopyInstruction(RiverInstruction &rOut, const RiverInstruction &rIn) {
	memcpy(&rOut, &rIn, sizeof(rOut));

	if (RIVER_OPTYPE_MEM == RIVER_OPTYPE(rIn.opTypes[0])) {
		rOut.operands[0].asAddress = codegen->CloneAddress(*rIn.operands[0].asAddress, rIn.modifiers);
	}

	if (RIVER_OPTYPE_MEM == RIVER_OPTYPE(rIn.opTypes[1])) {
		rOut.operands[1].asAddress = codegen->CloneAddress(*rIn.operands[1].asAddress, rIn.modifiers);
	}
}

bool RiverReverseTranslator::Init(RiverCodeGen *cg) {
	codegen = cg;
	return true;
}

void RiverReverseTranslator::Translate(const RiverInstruction &rIn, RiverInstruction &rOut) {
	if (RIVER_FAMILY_RIVER != RIVER_FAMILY(rIn.family)) {
		CopyInstruction(rOut, rIn);
		rOut.family |= RIVER_FAMILY_FLAG_IGNORE;
		return;
	}

	DWORD dwTable = (RIVER_MODIFIER_EXT & rIn.modifiers) ? 1 : 0;
	(this->*translateOpcodes[dwTable][rIn.opCode])(rOut, rIn);
}

void RiverReverseTranslator::TranslateUnk(RiverInstruction &rOut, const RiverInstruction &rIn) {
	__asm int 3;
}

void RiverReverseTranslator::TranslatePushReg(RiverInstruction &rOut, const RiverInstruction &rIn) {
	CopyInstruction(rOut, rIn);
	rOut.opCode += 8;
}

void RiverReverseTranslator::TranslatePopReg(RiverInstruction &rOut, const RiverInstruction &rIn) {
	CopyInstruction(rOut, rIn);
	rOut.opCode -= 8;
}

void RiverReverseTranslator::TranslatePushf(RiverInstruction &rOut, const RiverInstruction &rIn) {
	CopyInstruction(rOut, rIn);
	rOut.opCode += 1;
}

void RiverReverseTranslator::TranslatePopf(RiverInstruction &rOut, const RiverInstruction &rIn) {
	CopyInstruction(rOut, rIn);
	rOut.opCode -= 1;
}

void RiverReverseTranslator::TranslatePushModRM(RiverInstruction &rOut, const RiverInstruction &rIn) {
	CopyInstruction(rOut, rIn);
	
	if (rOut.subOpCode != 6) {
		rOut.family |= RIVER_FAMILY_FLAG_IGNORE;
	} else {
		rOut.opCode = 0x8F; // pop
		rOut.subOpCode = 0x00;

		rOut.operands[0].asAddress->modRM &= 0xC7; // clear the subOpCode from the modrm byte
	}
}

void RiverReverseTranslator::TranslatePopModRM(RiverInstruction &rOut, const RiverInstruction &rIn) {
	CopyInstruction(rOut, rIn);
	
	if (rOut.subOpCode != 0) {
		rOut.family |= RIVER_FAMILY_FLAG_IGNORE;
	}
	else {
		rOut.opCode = 0xFF; // push
		rOut.subOpCode = 0x06;
		rOut.opCode -= 1;

		rOut.operands[0].asAddress->modRM &= 0xC7; // clear the subOpCode from the modrm byte
		rOut.operands[0].asAddress->modRM |= 0x06 << 3; // set the subOpCode from the modrm byte
	}
}

void RiverReverseTranslator::Translate0x83(RiverInstruction &rOut, const RiverInstruction &rIn) {
	CopyInstruction(rOut, rIn);
	
	switch (rOut.subOpCode) {
		case 0:
		case 5:
			rOut.subOpCode ^= 5;
			rOut.operands[0].asRegister.versioned -= 0x100; // previous register version
			break;
		default:
			__asm int 3;
	}
}




RiverReverseTranslator::TranslateOpcodeFunc RiverReverseTranslator::translateOpcodes[2][0x100] = {
	{
		/*0x00*/&RiverReverseTranslator::TranslateUnk, &RiverReverseTranslator::TranslateUnk, &RiverReverseTranslator::TranslateUnk, &RiverReverseTranslator::TranslateUnk,
		/*0x04*/&RiverReverseTranslator::TranslateUnk, &RiverReverseTranslator::TranslateUnk, &RiverReverseTranslator::TranslateUnk, &RiverReverseTranslator::TranslateUnk,
		/*0x08*/&RiverReverseTranslator::TranslateUnk, &RiverReverseTranslator::TranslateUnk, &RiverReverseTranslator::TranslateUnk, &RiverReverseTranslator::TranslateUnk,
		/*0x0C*/&RiverReverseTranslator::TranslateUnk, &RiverReverseTranslator::TranslateUnk, &RiverReverseTranslator::TranslateUnk, &RiverReverseTranslator::TranslateUnk,

		/*0x10*/&RiverReverseTranslator::TranslateUnk, &RiverReverseTranslator::TranslateUnk, &RiverReverseTranslator::TranslateUnk, &RiverReverseTranslator::TranslateUnk,
		/*0x14*/&RiverReverseTranslator::TranslateUnk, &RiverReverseTranslator::TranslateUnk, &RiverReverseTranslator::TranslateUnk, &RiverReverseTranslator::TranslateUnk,
		/*0x18*/&RiverReverseTranslator::TranslateUnk, &RiverReverseTranslator::TranslateUnk, &RiverReverseTranslator::TranslateUnk, &RiverReverseTranslator::TranslateUnk,
		/*0x1C*/&RiverReverseTranslator::TranslateUnk, &RiverReverseTranslator::TranslateUnk, &RiverReverseTranslator::TranslateUnk, &RiverReverseTranslator::TranslateUnk,

		/*0x20*/&RiverReverseTranslator::TranslateUnk, &RiverReverseTranslator::TranslateUnk, &RiverReverseTranslator::TranslateUnk, &RiverReverseTranslator::TranslateUnk,
		/*0x24*/&RiverReverseTranslator::TranslateUnk, &RiverReverseTranslator::TranslateUnk, &RiverReverseTranslator::TranslateUnk, &RiverReverseTranslator::TranslateUnk,
		/*0x28*/&RiverReverseTranslator::TranslateUnk, &RiverReverseTranslator::TranslateUnk, &RiverReverseTranslator::TranslateUnk, &RiverReverseTranslator::TranslateUnk,
		/*0x2C*/&RiverReverseTranslator::TranslateUnk, &RiverReverseTranslator::TranslateUnk, &RiverReverseTranslator::TranslateUnk, &RiverReverseTranslator::TranslateUnk,

		/*0x30*/&RiverReverseTranslator::TranslateUnk, &RiverReverseTranslator::TranslateUnk, &RiverReverseTranslator::TranslateUnk, &RiverReverseTranslator::TranslateUnk,
		/*0x34*/&RiverReverseTranslator::TranslateUnk, &RiverReverseTranslator::TranslateUnk, &RiverReverseTranslator::TranslateUnk, &RiverReverseTranslator::TranslateUnk,
		/*0x38*/&RiverReverseTranslator::TranslateUnk, &RiverReverseTranslator::TranslateUnk, &RiverReverseTranslator::TranslateUnk, &RiverReverseTranslator::TranslateUnk,
		/*0x3C*/&RiverReverseTranslator::TranslateUnk, &RiverReverseTranslator::TranslateUnk, &RiverReverseTranslator::TranslateUnk, &RiverReverseTranslator::TranslateUnk,

		/*0x40*/&RiverReverseTranslator::TranslateUnk, &RiverReverseTranslator::TranslateUnk, &RiverReverseTranslator::TranslateUnk, &RiverReverseTranslator::TranslateUnk,
		/*0x44*/&RiverReverseTranslator::TranslateUnk, &RiverReverseTranslator::TranslateUnk, &RiverReverseTranslator::TranslateUnk, &RiverReverseTranslator::TranslateUnk,
		/*0x48*/&RiverReverseTranslator::TranslateUnk, &RiverReverseTranslator::TranslateUnk, &RiverReverseTranslator::TranslateUnk, &RiverReverseTranslator::TranslateUnk,
		/*0x4C*/&RiverReverseTranslator::TranslateUnk, &RiverReverseTranslator::TranslateUnk, &RiverReverseTranslator::TranslateUnk, &RiverReverseTranslator::TranslateUnk,

		/*0x50*/&RiverReverseTranslator::TranslatePushReg, &RiverReverseTranslator::TranslatePushReg, &RiverReverseTranslator::TranslatePushReg, &RiverReverseTranslator::TranslatePushReg,
		/*0x54*/&RiverReverseTranslator::TranslatePushReg, &RiverReverseTranslator::TranslatePushReg, &RiverReverseTranslator::TranslatePushReg, &RiverReverseTranslator::TranslatePushReg,
		/*0x58*/&RiverReverseTranslator::TranslatePopReg, &RiverReverseTranslator::TranslatePopReg, &RiverReverseTranslator::TranslatePopReg, &RiverReverseTranslator::TranslatePopReg,
		/*0x5C*/&RiverReverseTranslator::TranslatePopReg, &RiverReverseTranslator::TranslatePopReg, &RiverReverseTranslator::TranslatePopReg, &RiverReverseTranslator::TranslatePopReg,

		/*0x60*/&RiverReverseTranslator::TranslateUnk, &RiverReverseTranslator::TranslateUnk, &RiverReverseTranslator::TranslateUnk, &RiverReverseTranslator::TranslateUnk,
		/*0x64*/&RiverReverseTranslator::TranslateUnk, &RiverReverseTranslator::TranslateUnk, &RiverReverseTranslator::TranslateUnk, &RiverReverseTranslator::TranslateUnk,
		/*0x68*/&RiverReverseTranslator::TranslateUnk, &RiverReverseTranslator::TranslateUnk, &RiverReverseTranslator::TranslateUnk, &RiverReverseTranslator::TranslateUnk,
		/*0x6C*/&RiverReverseTranslator::TranslateUnk, &RiverReverseTranslator::TranslateUnk, &RiverReverseTranslator::TranslateUnk, &RiverReverseTranslator::TranslateUnk,

		/*0x70*/&RiverReverseTranslator::TranslateUnk, &RiverReverseTranslator::TranslateUnk, &RiverReverseTranslator::TranslateUnk, &RiverReverseTranslator::TranslateUnk,
		/*0x74*/&RiverReverseTranslator::TranslateUnk, &RiverReverseTranslator::TranslateUnk, &RiverReverseTranslator::TranslateUnk, &RiverReverseTranslator::TranslateUnk,
		/*0x78*/&RiverReverseTranslator::TranslateUnk, &RiverReverseTranslator::TranslateUnk, &RiverReverseTranslator::TranslateUnk, &RiverReverseTranslator::TranslateUnk,
		/*0x7C*/&RiverReverseTranslator::TranslateUnk, &RiverReverseTranslator::TranslateUnk, &RiverReverseTranslator::TranslateUnk, &RiverReverseTranslator::TranslateUnk,

		/*0x80*/&RiverReverseTranslator::TranslateUnk, &RiverReverseTranslator::TranslateUnk, &RiverReverseTranslator::TranslateUnk, &RiverReverseTranslator::Translate0x83,
		/*0x84*/&RiverReverseTranslator::TranslateUnk, &RiverReverseTranslator::TranslateUnk, &RiverReverseTranslator::TranslateUnk, &RiverReverseTranslator::TranslateUnk,
		/*0x88*/&RiverReverseTranslator::TranslateUnk, &RiverReverseTranslator::TranslateUnk, &RiverReverseTranslator::TranslateUnk, &RiverReverseTranslator::TranslateUnk,
		/*0x8C*/&RiverReverseTranslator::TranslateUnk, &RiverReverseTranslator::TranslateUnk, &RiverReverseTranslator::TranslateUnk, &RiverReverseTranslator::TranslatePopModRM,

		/*0x90*/&RiverReverseTranslator::TranslateUnk, &RiverReverseTranslator::TranslateUnk, &RiverReverseTranslator::TranslateUnk, &RiverReverseTranslator::TranslateUnk,
		/*0x94*/&RiverReverseTranslator::TranslateUnk, &RiverReverseTranslator::TranslateUnk, &RiverReverseTranslator::TranslateUnk, &RiverReverseTranslator::TranslateUnk,
		/*0x98*/&RiverReverseTranslator::TranslateUnk, &RiverReverseTranslator::TranslateUnk, &RiverReverseTranslator::TranslateUnk, &RiverReverseTranslator::TranslateUnk,
		/*0x9C*/&RiverReverseTranslator::TranslatePushf, &RiverReverseTranslator::TranslatePopf, &RiverReverseTranslator::TranslateUnk, &RiverReverseTranslator::TranslateUnk,

		/*0xA0*/&RiverReverseTranslator::TranslateUnk, &RiverReverseTranslator::TranslateUnk, &RiverReverseTranslator::TranslateUnk, &RiverReverseTranslator::TranslateUnk,
		/*0xA4*/&RiverReverseTranslator::TranslateUnk, &RiverReverseTranslator::TranslateUnk, &RiverReverseTranslator::TranslateUnk, &RiverReverseTranslator::TranslateUnk,
		/*0xA8*/&RiverReverseTranslator::TranslateUnk, &RiverReverseTranslator::TranslateUnk, &RiverReverseTranslator::TranslateUnk, &RiverReverseTranslator::TranslateUnk,
		/*0xAC*/&RiverReverseTranslator::TranslateUnk, &RiverReverseTranslator::TranslateUnk, &RiverReverseTranslator::TranslateUnk, &RiverReverseTranslator::TranslateUnk,

		/*0xB0*/&RiverReverseTranslator::TranslateUnk, &RiverReverseTranslator::TranslateUnk, &RiverReverseTranslator::TranslateUnk, &RiverReverseTranslator::TranslateUnk,
		/*0xB4*/&RiverReverseTranslator::TranslateUnk, &RiverReverseTranslator::TranslateUnk, &RiverReverseTranslator::TranslateUnk, &RiverReverseTranslator::TranslateUnk,
		/*0xB8*/&RiverReverseTranslator::TranslateUnk, &RiverReverseTranslator::TranslateUnk, &RiverReverseTranslator::TranslateUnk, &RiverReverseTranslator::TranslateUnk,
		/*0xBC*/&RiverReverseTranslator::TranslateUnk, &RiverReverseTranslator::TranslateUnk, &RiverReverseTranslator::TranslateUnk, &RiverReverseTranslator::TranslateUnk,

		/*0xC0*/&RiverReverseTranslator::TranslateUnk, &RiverReverseTranslator::TranslateUnk, &RiverReverseTranslator::TranslateUnk, &RiverReverseTranslator::TranslateUnk,
		/*0xC4*/&RiverReverseTranslator::TranslateUnk, &RiverReverseTranslator::TranslateUnk, &RiverReverseTranslator::TranslateUnk, &RiverReverseTranslator::TranslateUnk,
		/*0xC8*/&RiverReverseTranslator::TranslateUnk, &RiverReverseTranslator::TranslateUnk, &RiverReverseTranslator::TranslateUnk, &RiverReverseTranslator::TranslateUnk,
		/*0xCC*/&RiverReverseTranslator::TranslateUnk, &RiverReverseTranslator::TranslateUnk, &RiverReverseTranslator::TranslateUnk, &RiverReverseTranslator::TranslateUnk,

		/*0xD0*/&RiverReverseTranslator::TranslateUnk, &RiverReverseTranslator::TranslateUnk, &RiverReverseTranslator::TranslateUnk, &RiverReverseTranslator::TranslateUnk,
		/*0xD4*/&RiverReverseTranslator::TranslateUnk, &RiverReverseTranslator::TranslateUnk, &RiverReverseTranslator::TranslateUnk, &RiverReverseTranslator::TranslateUnk,
		/*0xD8*/&RiverReverseTranslator::TranslateUnk, &RiverReverseTranslator::TranslateUnk, &RiverReverseTranslator::TranslateUnk, &RiverReverseTranslator::TranslateUnk,
		/*0xDC*/&RiverReverseTranslator::TranslateUnk, &RiverReverseTranslator::TranslateUnk, &RiverReverseTranslator::TranslateUnk, &RiverReverseTranslator::TranslateUnk,

		/*0xE0*/&RiverReverseTranslator::TranslateUnk, &RiverReverseTranslator::TranslateUnk, &RiverReverseTranslator::TranslateUnk, &RiverReverseTranslator::TranslateUnk,
		/*0xE4*/&RiverReverseTranslator::TranslateUnk, &RiverReverseTranslator::TranslateUnk, &RiverReverseTranslator::TranslateUnk, &RiverReverseTranslator::TranslateUnk,
		/*0xE8*/&RiverReverseTranslator::TranslateUnk, &RiverReverseTranslator::TranslateUnk, &RiverReverseTranslator::TranslateUnk, &RiverReverseTranslator::TranslateUnk,
		/*0xEC*/&RiverReverseTranslator::TranslateUnk, &RiverReverseTranslator::TranslateUnk, &RiverReverseTranslator::TranslateUnk, &RiverReverseTranslator::TranslateUnk,

		/*0xF0*/&RiverReverseTranslator::TranslateUnk, &RiverReverseTranslator::TranslateUnk, &RiverReverseTranslator::TranslateUnk, &RiverReverseTranslator::TranslateUnk,
		/*0xF4*/&RiverReverseTranslator::TranslateUnk, &RiverReverseTranslator::TranslateUnk, &RiverReverseTranslator::TranslateUnk, &RiverReverseTranslator::TranslateUnk,
		/*0xF8*/&RiverReverseTranslator::TranslateUnk, &RiverReverseTranslator::TranslateUnk, &RiverReverseTranslator::TranslateUnk, &RiverReverseTranslator::TranslateUnk,
		/*0xFC*/&RiverReverseTranslator::TranslateUnk, &RiverReverseTranslator::TranslateUnk, &RiverReverseTranslator::TranslateUnk, &RiverReverseTranslator::TranslatePushModRM
	}, {
		/*0x00*/&RiverReverseTranslator::TranslateUnk, &RiverReverseTranslator::TranslateUnk, &RiverReverseTranslator::TranslateUnk, &RiverReverseTranslator::TranslateUnk,
		/*0x04*/&RiverReverseTranslator::TranslateUnk, &RiverReverseTranslator::TranslateUnk, &RiverReverseTranslator::TranslateUnk, &RiverReverseTranslator::TranslateUnk,
		/*0x08*/&RiverReverseTranslator::TranslateUnk, &RiverReverseTranslator::TranslateUnk, &RiverReverseTranslator::TranslateUnk, &RiverReverseTranslator::TranslateUnk,
		/*0x0C*/&RiverReverseTranslator::TranslateUnk, &RiverReverseTranslator::TranslateUnk, &RiverReverseTranslator::TranslateUnk, &RiverReverseTranslator::TranslateUnk,

		/*0x10*/&RiverReverseTranslator::TranslateUnk, &RiverReverseTranslator::TranslateUnk, &RiverReverseTranslator::TranslateUnk, &RiverReverseTranslator::TranslateUnk,
		/*0x14*/&RiverReverseTranslator::TranslateUnk, &RiverReverseTranslator::TranslateUnk, &RiverReverseTranslator::TranslateUnk, &RiverReverseTranslator::TranslateUnk,
		/*0x18*/&RiverReverseTranslator::TranslateUnk, &RiverReverseTranslator::TranslateUnk, &RiverReverseTranslator::TranslateUnk, &RiverReverseTranslator::TranslateUnk,
		/*0x1C*/&RiverReverseTranslator::TranslateUnk, &RiverReverseTranslator::TranslateUnk, &RiverReverseTranslator::TranslateUnk, &RiverReverseTranslator::TranslateUnk,

		/*0x20*/&RiverReverseTranslator::TranslateUnk, &RiverReverseTranslator::TranslateUnk, &RiverReverseTranslator::TranslateUnk, &RiverReverseTranslator::TranslateUnk,
		/*0x24*/&RiverReverseTranslator::TranslateUnk, &RiverReverseTranslator::TranslateUnk, &RiverReverseTranslator::TranslateUnk, &RiverReverseTranslator::TranslateUnk,
		/*0x28*/&RiverReverseTranslator::TranslateUnk, &RiverReverseTranslator::TranslateUnk, &RiverReverseTranslator::TranslateUnk, &RiverReverseTranslator::TranslateUnk,
		/*0x2C*/&RiverReverseTranslator::TranslateUnk, &RiverReverseTranslator::TranslateUnk, &RiverReverseTranslator::TranslateUnk, &RiverReverseTranslator::TranslateUnk,

		/*0x30*/&RiverReverseTranslator::TranslateUnk, &RiverReverseTranslator::TranslateUnk, &RiverReverseTranslator::TranslateUnk, &RiverReverseTranslator::TranslateUnk,
		/*0x34*/&RiverReverseTranslator::TranslateUnk, &RiverReverseTranslator::TranslateUnk, &RiverReverseTranslator::TranslateUnk, &RiverReverseTranslator::TranslateUnk,
		/*0x38*/&RiverReverseTranslator::TranslateUnk, &RiverReverseTranslator::TranslateUnk, &RiverReverseTranslator::TranslateUnk, &RiverReverseTranslator::TranslateUnk,
		/*0x3C*/&RiverReverseTranslator::TranslateUnk, &RiverReverseTranslator::TranslateUnk, &RiverReverseTranslator::TranslateUnk, &RiverReverseTranslator::TranslateUnk,

		/*0x40*/&RiverReverseTranslator::TranslateUnk, &RiverReverseTranslator::TranslateUnk, &RiverReverseTranslator::TranslateUnk, &RiverReverseTranslator::TranslateUnk,
		/*0x44*/&RiverReverseTranslator::TranslateUnk, &RiverReverseTranslator::TranslateUnk, &RiverReverseTranslator::TranslateUnk, &RiverReverseTranslator::TranslateUnk,
		/*0x48*/&RiverReverseTranslator::TranslateUnk, &RiverReverseTranslator::TranslateUnk, &RiverReverseTranslator::TranslateUnk, &RiverReverseTranslator::TranslateUnk,
		/*0x4C*/&RiverReverseTranslator::TranslateUnk, &RiverReverseTranslator::TranslateUnk, &RiverReverseTranslator::TranslateUnk, &RiverReverseTranslator::TranslateUnk,

		/*0x50*/&RiverReverseTranslator::TranslateUnk, &RiverReverseTranslator::TranslateUnk, &RiverReverseTranslator::TranslateUnk, &RiverReverseTranslator::TranslateUnk,
		/*0x54*/&RiverReverseTranslator::TranslateUnk, &RiverReverseTranslator::TranslateUnk, &RiverReverseTranslator::TranslateUnk, &RiverReverseTranslator::TranslateUnk,
		/*0x58*/&RiverReverseTranslator::TranslateUnk, &RiverReverseTranslator::TranslateUnk, &RiverReverseTranslator::TranslateUnk, &RiverReverseTranslator::TranslateUnk,
		/*0x5C*/&RiverReverseTranslator::TranslateUnk, &RiverReverseTranslator::TranslateUnk, &RiverReverseTranslator::TranslateUnk, &RiverReverseTranslator::TranslateUnk,

		/*0x60*/&RiverReverseTranslator::TranslateUnk, &RiverReverseTranslator::TranslateUnk, &RiverReverseTranslator::TranslateUnk, &RiverReverseTranslator::TranslateUnk,
		/*0x64*/&RiverReverseTranslator::TranslateUnk, &RiverReverseTranslator::TranslateUnk, &RiverReverseTranslator::TranslateUnk, &RiverReverseTranslator::TranslateUnk,
		/*0x68*/&RiverReverseTranslator::TranslateUnk, &RiverReverseTranslator::TranslateUnk, &RiverReverseTranslator::TranslateUnk, &RiverReverseTranslator::TranslateUnk,
		/*0x6C*/&RiverReverseTranslator::TranslateUnk, &RiverReverseTranslator::TranslateUnk, &RiverReverseTranslator::TranslateUnk, &RiverReverseTranslator::TranslateUnk,

		/*0x70*/&RiverReverseTranslator::TranslateUnk, &RiverReverseTranslator::TranslateUnk, &RiverReverseTranslator::TranslateUnk, &RiverReverseTranslator::TranslateUnk,
		/*0x74*/&RiverReverseTranslator::TranslateUnk, &RiverReverseTranslator::TranslateUnk, &RiverReverseTranslator::TranslateUnk, &RiverReverseTranslator::TranslateUnk,
		/*0x78*/&RiverReverseTranslator::TranslateUnk, &RiverReverseTranslator::TranslateUnk, &RiverReverseTranslator::TranslateUnk, &RiverReverseTranslator::TranslateUnk,
		/*0x7C*/&RiverReverseTranslator::TranslateUnk, &RiverReverseTranslator::TranslateUnk, &RiverReverseTranslator::TranslateUnk, &RiverReverseTranslator::TranslateUnk,

		/*0x80*/&RiverReverseTranslator::TranslateUnk, &RiverReverseTranslator::TranslateUnk, &RiverReverseTranslator::TranslateUnk, &RiverReverseTranslator::TranslateUnk,
		/*0x84*/&RiverReverseTranslator::TranslateUnk, &RiverReverseTranslator::TranslateUnk, &RiverReverseTranslator::TranslateUnk, &RiverReverseTranslator::TranslateUnk,
		/*0x88*/&RiverReverseTranslator::TranslateUnk, &RiverReverseTranslator::TranslateUnk, &RiverReverseTranslator::TranslateUnk, &RiverReverseTranslator::TranslateUnk,
		/*0x8C*/&RiverReverseTranslator::TranslateUnk, &RiverReverseTranslator::TranslateUnk, &RiverReverseTranslator::TranslateUnk, &RiverReverseTranslator::TranslateUnk,

		/*0x90*/&RiverReverseTranslator::TranslateUnk, &RiverReverseTranslator::TranslateUnk, &RiverReverseTranslator::TranslateUnk, &RiverReverseTranslator::TranslateUnk,
		/*0x94*/&RiverReverseTranslator::TranslateUnk, &RiverReverseTranslator::TranslateUnk, &RiverReverseTranslator::TranslateUnk, &RiverReverseTranslator::TranslateUnk,
		/*0x98*/&RiverReverseTranslator::TranslateUnk, &RiverReverseTranslator::TranslateUnk, &RiverReverseTranslator::TranslateUnk, &RiverReverseTranslator::TranslateUnk,
		/*0x9C*/&RiverReverseTranslator::TranslateUnk, &RiverReverseTranslator::TranslateUnk, &RiverReverseTranslator::TranslateUnk, &RiverReverseTranslator::TranslateUnk,

		/*0xA0*/&RiverReverseTranslator::TranslateUnk, &RiverReverseTranslator::TranslateUnk, &RiverReverseTranslator::TranslateUnk, &RiverReverseTranslator::TranslateUnk,
		/*0xA4*/&RiverReverseTranslator::TranslateUnk, &RiverReverseTranslator::TranslateUnk, &RiverReverseTranslator::TranslateUnk, &RiverReverseTranslator::TranslateUnk,
		/*0xA8*/&RiverReverseTranslator::TranslateUnk, &RiverReverseTranslator::TranslateUnk, &RiverReverseTranslator::TranslateUnk, &RiverReverseTranslator::TranslateUnk,
		/*0xAC*/&RiverReverseTranslator::TranslateUnk, &RiverReverseTranslator::TranslateUnk, &RiverReverseTranslator::TranslateUnk, &RiverReverseTranslator::TranslateUnk,

		/*0xB0*/&RiverReverseTranslator::TranslateUnk, &RiverReverseTranslator::TranslateUnk, &RiverReverseTranslator::TranslateUnk, &RiverReverseTranslator::TranslateUnk,
		/*0xB4*/&RiverReverseTranslator::TranslateUnk, &RiverReverseTranslator::TranslateUnk, &RiverReverseTranslator::TranslateUnk, &RiverReverseTranslator::TranslateUnk,
		/*0xB8*/&RiverReverseTranslator::TranslateUnk, &RiverReverseTranslator::TranslateUnk, &RiverReverseTranslator::TranslateUnk, &RiverReverseTranslator::TranslateUnk,
		/*0xBC*/&RiverReverseTranslator::TranslateUnk, &RiverReverseTranslator::TranslateUnk, &RiverReverseTranslator::TranslateUnk, &RiverReverseTranslator::TranslateUnk,

		/*0xC0*/&RiverReverseTranslator::TranslateUnk, &RiverReverseTranslator::TranslateUnk, &RiverReverseTranslator::TranslateUnk, &RiverReverseTranslator::TranslateUnk,
		/*0xC4*/&RiverReverseTranslator::TranslateUnk, &RiverReverseTranslator::TranslateUnk, &RiverReverseTranslator::TranslateUnk, &RiverReverseTranslator::TranslateUnk,
		/*0xC8*/&RiverReverseTranslator::TranslateUnk, &RiverReverseTranslator::TranslateUnk, &RiverReverseTranslator::TranslateUnk, &RiverReverseTranslator::TranslateUnk,
		/*0xCC*/&RiverReverseTranslator::TranslateUnk, &RiverReverseTranslator::TranslateUnk, &RiverReverseTranslator::TranslateUnk, &RiverReverseTranslator::TranslateUnk,

		/*0xD0*/&RiverReverseTranslator::TranslateUnk, &RiverReverseTranslator::TranslateUnk, &RiverReverseTranslator::TranslateUnk, &RiverReverseTranslator::TranslateUnk,
		/*0xD4*/&RiverReverseTranslator::TranslateUnk, &RiverReverseTranslator::TranslateUnk, &RiverReverseTranslator::TranslateUnk, &RiverReverseTranslator::TranslateUnk,
		/*0xD8*/&RiverReverseTranslator::TranslateUnk, &RiverReverseTranslator::TranslateUnk, &RiverReverseTranslator::TranslateUnk, &RiverReverseTranslator::TranslateUnk,
		/*0xDC*/&RiverReverseTranslator::TranslateUnk, &RiverReverseTranslator::TranslateUnk, &RiverReverseTranslator::TranslateUnk, &RiverReverseTranslator::TranslateUnk,

		/*0xE0*/&RiverReverseTranslator::TranslateUnk, &RiverReverseTranslator::TranslateUnk, &RiverReverseTranslator::TranslateUnk, &RiverReverseTranslator::TranslateUnk,
		/*0xE4*/&RiverReverseTranslator::TranslateUnk, &RiverReverseTranslator::TranslateUnk, &RiverReverseTranslator::TranslateUnk, &RiverReverseTranslator::TranslateUnk,
		/*0xE8*/&RiverReverseTranslator::TranslateUnk, &RiverReverseTranslator::TranslateUnk, &RiverReverseTranslator::TranslateUnk, &RiverReverseTranslator::TranslateUnk,
		/*0xEC*/&RiverReverseTranslator::TranslateUnk, &RiverReverseTranslator::TranslateUnk, &RiverReverseTranslator::TranslateUnk, &RiverReverseTranslator::TranslateUnk,

		/*0xF0*/&RiverReverseTranslator::TranslateUnk, &RiverReverseTranslator::TranslateUnk, &RiverReverseTranslator::TranslateUnk, &RiverReverseTranslator::TranslateUnk,
		/*0xF4*/&RiverReverseTranslator::TranslateUnk, &RiverReverseTranslator::TranslateUnk, &RiverReverseTranslator::TranslateUnk, &RiverReverseTranslator::TranslateUnk,
		/*0xF8*/&RiverReverseTranslator::TranslateUnk, &RiverReverseTranslator::TranslateUnk, &RiverReverseTranslator::TranslateUnk, &RiverReverseTranslator::TranslateUnk,
		/*0xFC*/&RiverReverseTranslator::TranslateUnk, &RiverReverseTranslator::TranslateUnk, &RiverReverseTranslator::TranslateUnk, &RiverReverseTranslator::TranslateUnk
	}
};

