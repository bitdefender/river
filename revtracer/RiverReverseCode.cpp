#include "river.h"
#include "riverinternl.h"
#include "mm.h"
#include "execenv.h"

extern const ConvertInstructionFunc riverReverseCode00[];
extern const ConvertInstructionFunc riverReverseCode0F[];

void CopyInstruction(struct _exec_env *pEnv, struct RiverInstruction *rOut, struct RiverInstruction *rIn);

extern "C" void TranslateReverse(struct _exec_env *pEnv, struct RiverInstruction *rIn, struct RiverInstruction *rOut, DWORD *outCount) {
	const ConvertInstructionFunc *cvtTbl = riverReverseCode00;
	*outCount += 1;


	if (0 == (RIVER_MODIFIER_RIVEROP & rIn->modifiers)) {
		CopyInstruction(pEnv, rOut, rIn);
		rOut->modifiers |= RIVER_MODIFIER_IGNORE;
		return;
	}

	if (RIVER_MODIFIER_EXT & rIn->modifiers) {
		cvtTbl = riverReverseCode0F;
	}

	cvtTbl[rIn->opCode](pEnv, rIn, rOut, outCount);
}

void ReverseUnknown(struct _exec_env *pEnv, struct RiverInstruction *rIn, struct RiverInstruction *rOut, DWORD *outCount) {
	__asm int 3;
}

void ReversePushReg(struct _exec_env *pEnv, struct RiverInstruction *rIn, struct RiverInstruction *rOut, DWORD *outCount) {
	CopyInstruction(pEnv, rOut, rIn);
	rOut->opCode += 8;
}

void ReversePopReg(struct _exec_env *pEnv, struct RiverInstruction *rIn, struct RiverInstruction *rOut, DWORD *outCount) {
	CopyInstruction(pEnv, rOut, rIn);
	rOut->opCode -= 8;
}

void ReversePushf(struct _exec_env *pEnv, struct RiverInstruction *rIn, struct RiverInstruction *rOut, DWORD *outCount) {
	CopyInstruction(pEnv, rOut, rIn);
	rOut->opCode += 1;
}

void ReversePopf(struct _exec_env *pEnv, struct RiverInstruction *rIn, struct RiverInstruction *rOut, DWORD *outCount) {
	CopyInstruction(pEnv, rOut, rIn);
	rOut->opCode -= 1;
}

void ReversePushModRM(struct _exec_env *pEnv, struct RiverInstruction *rIn, struct RiverInstruction *rOut, DWORD *outCount) {
	CopyInstruction(pEnv, rOut, rIn);
	*outCount += 1;

	if (rOut->subOpCode != 6) {
		rOut->modifiers |= RIVER_MODIFIER_IGNORE;
	} else {
		rOut->opCode = 0x8F; // pop
		rOut->subOpCode = 0x00;

		rOut->operands[0].asAddress->modRM &= 0xC7; // clear the subOpCode from the modrm byte
	}
}

void ReversePopModRM(struct _exec_env *pEnv, struct RiverInstruction *rIn, struct RiverInstruction *rOut, DWORD *outCount) {
	CopyInstruction(pEnv, rOut, rIn);
	*outCount += 1;

	if (rOut->subOpCode != 0) {
		rOut->modifiers |= RIVER_MODIFIER_IGNORE;
	} else {
		rOut->opCode = 0xFF; // push
		rOut->subOpCode = 0x06;
		rOut->opCode -= 1;

		rOut->operands[0].asAddress->modRM &= 0xC7; // clear the subOpCode from the modrm byte
		rOut->operands[0].asAddress->modRM |= 0x06 << 3; // set the subOpCode from the modrm byte
	}
}

void Reverse0x83(struct _exec_env *pEnv, struct RiverInstruction *rIn, struct RiverInstruction *rOut, DWORD *outCount) {
	CopyInstruction(pEnv, rOut, rIn);
	*outCount += 1;

	switch (rOut->subOpCode) {
		case 0 : 
		case 5 :
			rOut->subOpCode ^= 5;
			rOut->operands[0].asRegister.versioned -= 0x100; // previous register version
			break;
		default :
			__asm int 3;
	}
}

const ConvertInstructionFunc riverReverseCode00[] = {
	/*0x00*/ReverseUnknown, ReverseUnknown, ReverseUnknown, ReverseUnknown,
	/*0x04*/ReverseUnknown, ReverseUnknown, ReverseUnknown, ReverseUnknown,
	/*0x08*/ReverseUnknown, ReverseUnknown, ReverseUnknown, ReverseUnknown,
	/*0x0C*/ReverseUnknown, ReverseUnknown, ReverseUnknown, ReverseUnknown,

	/*0x10*/ReverseUnknown, ReverseUnknown, ReverseUnknown, ReverseUnknown,
	/*0x14*/ReverseUnknown, ReverseUnknown, ReverseUnknown, ReverseUnknown,
	/*0x18*/ReverseUnknown, ReverseUnknown, ReverseUnknown, ReverseUnknown,
	/*0x1C*/ReverseUnknown, ReverseUnknown, ReverseUnknown, ReverseUnknown,

	/*0x20*/ReverseUnknown, ReverseUnknown, ReverseUnknown, ReverseUnknown,
	/*0x24*/ReverseUnknown, ReverseUnknown, ReverseUnknown, ReverseUnknown,
	/*0x28*/ReverseUnknown, ReverseUnknown, ReverseUnknown, ReverseUnknown,
	/*0x2C*/ReverseUnknown, ReverseUnknown, ReverseUnknown, ReverseUnknown,

	/*0x30*/ReverseUnknown, ReverseUnknown, ReverseUnknown, ReverseUnknown,
	/*0x34*/ReverseUnknown, ReverseUnknown, ReverseUnknown, ReverseUnknown,
	/*0x38*/ReverseUnknown, ReverseUnknown, ReverseUnknown, ReverseUnknown,
	/*0x3C*/ReverseUnknown, ReverseUnknown, ReverseUnknown, ReverseUnknown,

	/*0x40*/ReverseUnknown, ReverseUnknown, ReverseUnknown, ReverseUnknown,
	/*0x44*/ReverseUnknown, ReverseUnknown, ReverseUnknown, ReverseUnknown,
	/*0x48*/ReverseUnknown, ReverseUnknown, ReverseUnknown, ReverseUnknown,
	/*0x4C*/ReverseUnknown, ReverseUnknown, ReverseUnknown, ReverseUnknown,

	/*0x50*/ReversePushReg, ReversePushReg, ReversePushReg, ReversePushReg,
	/*0x54*/ReversePushReg, ReversePushReg, ReversePushReg, ReversePushReg,
	/*0x58*/ReversePopReg, ReversePopReg, ReversePopReg, ReversePopReg,
	/*0x5C*/ReversePopReg, ReversePopReg, ReversePopReg, ReversePopReg,

	/*0x60*/ReverseUnknown, ReverseUnknown, ReverseUnknown, ReverseUnknown,
	/*0x64*/ReverseUnknown, ReverseUnknown, ReverseUnknown, ReverseUnknown,
	/*0x68*/ReverseUnknown, ReverseUnknown, ReverseUnknown, ReverseUnknown,
	/*0x6C*/ReverseUnknown, ReverseUnknown, ReverseUnknown, ReverseUnknown,

	/*0x70*/ReverseUnknown, ReverseUnknown, ReverseUnknown, ReverseUnknown,
	/*0x74*/ReverseUnknown, ReverseUnknown, ReverseUnknown, ReverseUnknown,
	/*0x78*/ReverseUnknown, ReverseUnknown, ReverseUnknown, ReverseUnknown,
	/*0x7C*/ReverseUnknown, ReverseUnknown, ReverseUnknown, ReverseUnknown,

	/*0x80*/ReverseUnknown, ReverseUnknown, ReverseUnknown, Reverse0x83,
	/*0x84*/ReverseUnknown, ReverseUnknown, ReverseUnknown, ReverseUnknown,
	/*0x88*/ReverseUnknown, ReverseUnknown, ReverseUnknown, ReverseUnknown,
	/*0x8C*/ReverseUnknown, ReverseUnknown, ReverseUnknown, ReversePopModRM,

	/*0x90*/ReverseUnknown, ReverseUnknown, ReverseUnknown, ReverseUnknown,
	/*0x94*/ReverseUnknown, ReverseUnknown, ReverseUnknown, ReverseUnknown,
	/*0x98*/ReverseUnknown, ReverseUnknown, ReverseUnknown, ReverseUnknown,
	/*0x9C*/ReversePushf, ReversePopf, ReverseUnknown, ReverseUnknown,

	/*0xA0*/ReverseUnknown, ReverseUnknown, ReverseUnknown, ReverseUnknown,
	/*0xA4*/ReverseUnknown, ReverseUnknown, ReverseUnknown, ReverseUnknown,
	/*0xA8*/ReverseUnknown, ReverseUnknown, ReverseUnknown, ReverseUnknown,
	/*0xAC*/ReverseUnknown, ReverseUnknown, ReverseUnknown, ReverseUnknown,

	/*0xB0*/ReverseUnknown, ReverseUnknown, ReverseUnknown, ReverseUnknown,
	/*0xB4*/ReverseUnknown, ReverseUnknown, ReverseUnknown, ReverseUnknown,
	/*0xB8*/ReverseUnknown, ReverseUnknown, ReverseUnknown, ReverseUnknown,
	/*0xBC*/ReverseUnknown, ReverseUnknown, ReverseUnknown, ReverseUnknown,

	/*0xC0*/ReverseUnknown, ReverseUnknown, ReverseUnknown, ReverseUnknown,
	/*0xC4*/ReverseUnknown, ReverseUnknown, ReverseUnknown, ReverseUnknown,
	/*0xC8*/ReverseUnknown, ReverseUnknown, ReverseUnknown, ReverseUnknown,
	/*0xCC*/ReverseUnknown, ReverseUnknown, ReverseUnknown, ReverseUnknown,

	/*0xD0*/ReverseUnknown, ReverseUnknown, ReverseUnknown, ReverseUnknown,
	/*0xD4*/ReverseUnknown, ReverseUnknown, ReverseUnknown, ReverseUnknown,
	/*0xD8*/ReverseUnknown, ReverseUnknown, ReverseUnknown, ReverseUnknown,
	/*0xDC*/ReverseUnknown, ReverseUnknown, ReverseUnknown, ReverseUnknown,

	/*0xE0*/ReverseUnknown, ReverseUnknown, ReverseUnknown, ReverseUnknown,
	/*0xE4*/ReverseUnknown, ReverseUnknown, ReverseUnknown, ReverseUnknown,
	/*0xE8*/ReverseUnknown, ReverseUnknown, ReverseUnknown, ReverseUnknown,
	/*0xEC*/ReverseUnknown, ReverseUnknown, ReverseUnknown, ReverseUnknown,

	/*0xF0*/ReverseUnknown, ReverseUnknown, ReverseUnknown, ReverseUnknown,
	/*0xF4*/ReverseUnknown, ReverseUnknown, ReverseUnknown, ReverseUnknown,
	/*0xF8*/ReverseUnknown, ReverseUnknown, ReverseUnknown, ReverseUnknown,
	/*0xFC*/ReverseUnknown, ReverseUnknown, ReverseUnknown, ReversePushModRM
};

const ConvertInstructionFunc riverReverseCode0F[] = {
	/*0x00*/ReverseUnknown, ReverseUnknown, ReverseUnknown, ReverseUnknown,
	/*0x04*/ReverseUnknown, ReverseUnknown, ReverseUnknown, ReverseUnknown,
	/*0x08*/ReverseUnknown, ReverseUnknown, ReverseUnknown, ReverseUnknown,
	/*0x0C*/ReverseUnknown, ReverseUnknown, ReverseUnknown, ReverseUnknown,

	/*0x10*/ReverseUnknown, ReverseUnknown, ReverseUnknown, ReverseUnknown,
	/*0x14*/ReverseUnknown, ReverseUnknown, ReverseUnknown, ReverseUnknown,
	/*0x18*/ReverseUnknown, ReverseUnknown, ReverseUnknown, ReverseUnknown,
	/*0x1C*/ReverseUnknown, ReverseUnknown, ReverseUnknown, ReverseUnknown,

	/*0x20*/ReverseUnknown, ReverseUnknown, ReverseUnknown, ReverseUnknown,
	/*0x24*/ReverseUnknown, ReverseUnknown, ReverseUnknown, ReverseUnknown,
	/*0x28*/ReverseUnknown, ReverseUnknown, ReverseUnknown, ReverseUnknown,
	/*0x2C*/ReverseUnknown, ReverseUnknown, ReverseUnknown, ReverseUnknown,

	/*0x30*/ReverseUnknown, ReverseUnknown, ReverseUnknown, ReverseUnknown,
	/*0x34*/ReverseUnknown, ReverseUnknown, ReverseUnknown, ReverseUnknown,
	/*0x38*/ReverseUnknown, ReverseUnknown, ReverseUnknown, ReverseUnknown,
	/*0x3C*/ReverseUnknown, ReverseUnknown, ReverseUnknown, ReverseUnknown,

	/*0x40*/ReverseUnknown, ReverseUnknown, ReverseUnknown, ReverseUnknown,
	/*0x44*/ReverseUnknown, ReverseUnknown, ReverseUnknown, ReverseUnknown,
	/*0x48*/ReverseUnknown, ReverseUnknown, ReverseUnknown, ReverseUnknown,
	/*0x4C*/ReverseUnknown, ReverseUnknown, ReverseUnknown, ReverseUnknown,

	/*0x50*/ReverseUnknown, ReverseUnknown, ReverseUnknown, ReverseUnknown,
	/*0x54*/ReverseUnknown, ReverseUnknown, ReverseUnknown, ReverseUnknown,
	/*0x58*/ReverseUnknown, ReverseUnknown, ReverseUnknown, ReverseUnknown,
	/*0x5C*/ReverseUnknown, ReverseUnknown, ReverseUnknown, ReverseUnknown,

	/*0x60*/ReverseUnknown, ReverseUnknown, ReverseUnknown, ReverseUnknown,
	/*0x64*/ReverseUnknown, ReverseUnknown, ReverseUnknown, ReverseUnknown,
	/*0x68*/ReverseUnknown, ReverseUnknown, ReverseUnknown, ReverseUnknown,
	/*0x6C*/ReverseUnknown, ReverseUnknown, ReverseUnknown, ReverseUnknown,

	/*0x70*/ReverseUnknown, ReverseUnknown, ReverseUnknown, ReverseUnknown,
	/*0x74*/ReverseUnknown, ReverseUnknown, ReverseUnknown, ReverseUnknown,
	/*0x78*/ReverseUnknown, ReverseUnknown, ReverseUnknown, ReverseUnknown,
	/*0x7C*/ReverseUnknown, ReverseUnknown, ReverseUnknown, ReverseUnknown,

	/*0x80*/ReverseUnknown, ReverseUnknown, ReverseUnknown, ReverseUnknown,
	/*0x84*/ReverseUnknown, ReverseUnknown, ReverseUnknown, ReverseUnknown,
	/*0x88*/ReverseUnknown, ReverseUnknown, ReverseUnknown, ReverseUnknown,
	/*0x8C*/ReverseUnknown, ReverseUnknown, ReverseUnknown, ReverseUnknown,

	/*0x90*/ReverseUnknown, ReverseUnknown, ReverseUnknown, ReverseUnknown,
	/*0x94*/ReverseUnknown, ReverseUnknown, ReverseUnknown, ReverseUnknown,
	/*0x98*/ReverseUnknown, ReverseUnknown, ReverseUnknown, ReverseUnknown,
	/*0x9C*/ReverseUnknown, ReverseUnknown, ReverseUnknown, ReverseUnknown,

	/*0xA0*/ReverseUnknown, ReverseUnknown, ReverseUnknown, ReverseUnknown,
	/*0xA4*/ReverseUnknown, ReverseUnknown, ReverseUnknown, ReverseUnknown,
	/*0xA8*/ReverseUnknown, ReverseUnknown, ReverseUnknown, ReverseUnknown,
	/*0xAC*/ReverseUnknown, ReverseUnknown, ReverseUnknown, ReverseUnknown,

	/*0xB0*/ReverseUnknown, ReverseUnknown, ReverseUnknown, ReverseUnknown,
	/*0xB4*/ReverseUnknown, ReverseUnknown, ReverseUnknown, ReverseUnknown,
	/*0xB8*/ReverseUnknown, ReverseUnknown, ReverseUnknown, ReverseUnknown,
	/*0xBC*/ReverseUnknown, ReverseUnknown, ReverseUnknown, ReverseUnknown,

	/*0xC0*/ReverseUnknown, ReverseUnknown, ReverseUnknown, ReverseUnknown,
	/*0xC4*/ReverseUnknown, ReverseUnknown, ReverseUnknown, ReverseUnknown,
	/*0xC8*/ReverseUnknown, ReverseUnknown, ReverseUnknown, ReverseUnknown,
	/*0xCC*/ReverseUnknown, ReverseUnknown, ReverseUnknown, ReverseUnknown,

	/*0xD0*/ReverseUnknown, ReverseUnknown, ReverseUnknown, ReverseUnknown,
	/*0xD4*/ReverseUnknown, ReverseUnknown, ReverseUnknown, ReverseUnknown,
	/*0xD8*/ReverseUnknown, ReverseUnknown, ReverseUnknown, ReverseUnknown,
	/*0xDC*/ReverseUnknown, ReverseUnknown, ReverseUnknown, ReverseUnknown,

	/*0xE0*/ReverseUnknown, ReverseUnknown, ReverseUnknown, ReverseUnknown,
	/*0xE4*/ReverseUnknown, ReverseUnknown, ReverseUnknown, ReverseUnknown,
	/*0xE8*/ReverseUnknown, ReverseUnknown, ReverseUnknown, ReverseUnknown,
	/*0xEC*/ReverseUnknown, ReverseUnknown, ReverseUnknown, ReverseUnknown,

	/*0xF0*/ReverseUnknown, ReverseUnknown, ReverseUnknown, ReverseUnknown,
	/*0xF4*/ReverseUnknown, ReverseUnknown, ReverseUnknown, ReverseUnknown,
	/*0xF8*/ReverseUnknown, ReverseUnknown, ReverseUnknown, ReverseUnknown,
	/*0xFC*/ReverseUnknown, ReverseUnknown, ReverseUnknown, ReverseUnknown
};