#include "river.h"
#include "riverinternl.h"
#include "mm.h"
#include "execenv.h"

struct RiverAddress *CloneAddress(RiverCodeGen *cg, struct RiverAddress *mem) {
	struct RiverAddress *ret = cg->AllocAddr();
	memcpy(ret, mem, sizeof(*ret));
	return ret;
}

void CopyInstruction(RiverCodeGen *cg, struct RiverInstruction *rOut, struct RiverInstruction *rIn) {
	memcpy(rOut, rIn, sizeof(*rOut));

	if ((RIVER_OPTYPE_NONE != rIn->opTypes[0]) && (RIVER_OPTYPE_MEM & rIn->opTypes[0])) {
		rOut->operands[0].asAddress = CloneAddress(cg, rIn->operands[0].asAddress);
	}

	if ((RIVER_OPTYPE_NONE != rIn->opTypes[1]) && (RIVER_OPTYPE_MEM & rIn->opTypes[1])) {
		rOut->operands[1].asAddress = CloneAddress(cg, rIn->operands[1].asAddress);
	}
}

void MakeSaveFlags(struct RiverInstruction *rOut) {
	rOut->opCode = 0x9C; // PUSHF
	rOut->modifiers = RIVER_MODIFIER_RIVEROP;
	rOut->specifiers = 0; // maybe

	rOut->opTypes[0] = rOut->opTypes[1] = RIVER_OPTYPE_NONE;
}

inline void MakeSaveReg(RiverCodeGen *cg, struct RiverInstruction *rOut, union RiverRegister *reg, unsigned short auxFlags) {
	unsigned char rg = (reg->name & 0x07) | RIVER_OPSIZE_32; 


	rOut->opCode = 0x50; //PUSH
	rOut->modifiers = RIVER_MODIFIER_RIVEROP | auxFlags;
	rOut->specifiers = 0;

	rOut->opTypes[0] = RIVER_OPTYPE_REG | RIVER_OPSIZE_32;
	rOut->operands[0].asRegister.versioned = cg->GetPrevReg(rg);
	//rOut->operands[0].asRegister.name &= 0x07; // eliminate all size prefixes
	//rOut->operands[0].asRegister.versioned |= RIVER_OPSIZE_32;

	rOut->opTypes[1] = RIVER_OPTYPE_NONE;
}

inline void MakeSaveMem(RiverCodeGen *cg, struct RiverInstruction *rOut, struct RiverAddress *mem, unsigned short auxFlags) {
	rOut->opCode = 0xFF; // PUSH (ext + 6)
	rOut->modifiers = RIVER_MODIFIER_RIVEROP | auxFlags;
	rOut->subOpCode = 0x06;

	rOut->opTypes[0] = RIVER_OPTYPE_MEM | RIVER_OPSIZE_32;
	rOut->operands[0].asAddress = CloneAddress(cg, mem);
	rOut->operands[0].asAddress->modRM &= 0xC7;
	rOut->operands[0].asAddress->modRM |= rOut->subOpCode << 3;


	rOut->opTypes[1] = RIVER_OPTYPE_NONE;
}


void MakeAddNoFlagsRegImm8(RiverCodeGen *cg, struct RiverInstruction *rOut, union RiverRegister *reg, unsigned char offset, unsigned short auxFlags) {
	unsigned char rg = (reg->name & 0x07) | RIVER_OPSIZE_32;

	rOut->opCode = 0x83;
	rOut->subOpCode = 0;
	rOut->modifiers = RIVER_MODIFIER_RIVEROP | auxFlags;
	rOut->specifiers = 0;

	rOut->opTypes[0] = RIVER_OPTYPE_REG | RIVER_OPSIZE_32;
	rOut->operands[0].asRegister.versioned = cg->GetCurrentReg(rg);

	rOut->opTypes[1] = RIVER_OPTYPE_IMM | RIVER_OPSIZE_8;
	rOut->operands[1].asImm8 = offset;
}

void MakeSubNoFlagsRegImm8(RiverCodeGen *cg, struct RiverInstruction *rOut, union RiverRegister *reg, unsigned char offset, unsigned short auxFlags) {
	unsigned char rg = (reg->name & 0x07) | RIVER_OPSIZE_32;

	rOut->opCode = 0x83;
	rOut->subOpCode = 5;
	rOut->modifiers = RIVER_MODIFIER_RIVEROP | auxFlags;
	rOut->specifiers = 0;

	rOut->opTypes[0] = RIVER_OPTYPE_REG | RIVER_OPSIZE_32;
	rOut->operands[0].asRegister.versioned = cg->GetCurrentReg(rg);

	rOut->opTypes[1] = RIVER_OPTYPE_IMM | RIVER_OPSIZE_8;
	rOut->operands[1].asImm8 = offset;
}


void MakeSaveOp(RiverCodeGen *cg, struct RiverInstruction *rOut, unsigned char opType, union RiverOperand *op) {
	switch (opType) {
		case RIVER_OPTYPE_NONE :
		case RIVER_OPTYPE_IMM :
			__asm int 3;
			break;
		case RIVER_OPTYPE_REG :
			MakeSaveReg(cg, rOut, &op->asRegister, 0);
			break;
		case RIVER_OPTYPE_MEM :
			MakeSaveMem(cg, rOut, op->asAddress, 0);
			break;
	}
}

void MakeSaveAtEsp(RiverCodeGen *cg, struct RiverInstruction *rOut) {
	struct RiverAddress rTmp;

	rTmp.type = RIVER_ADDR_BASE | RIVER_ADDR_DISP8;
	rTmp.base.versioned = cg->GetPrevReg(RIVER_REG_xSP); // Here lies the original xSP
	rTmp.index.versioned = RIVER_REG_NONE; 
	rTmp.disp.d8 = 0xFC;

	rTmp.modRM = 0x70; // actually save xAX (because xSP is used for other stuff

	MakeSaveMem(cg, rOut, &rTmp, RIVER_MODIFIER_ORIG_xSP);
}

void SaveUnknown(RiverCodeGen *cg, struct RiverInstruction *rIn, struct RiverInstruction *rOut, DWORD *outCount) {
	__asm int 3;
}

void SaveOperands(RiverCodeGen *cg, struct RiverInstruction *rIn, struct RiverInstruction *rOut, DWORD *outCount) {

	if (RIVER_SPEC_MODIFIES_FLG & rIn->specifiers) {
		MakeSaveFlags(rOut);
		(*outCount)++;
		rOut++;
	}

	if (RIVER_SPEC_MODIFIES_OP2 & rIn->specifiers) {
		MakeSaveOp(cg, rOut, rIn->opTypes[1], &rIn->operands[1]);
		(*outCount)++;
		rOut++;
	}

	if (RIVER_SPEC_MODIFIES_OP1 & rIn->specifiers) {
		MakeSaveOp(cg, rOut, rIn->opTypes[0], &rIn->operands[0]);
		(*outCount)++;
		rOut++;
	}

	CopyInstruction(cg, rOut, rIn);
	(*outCount)++;
}

/* uses specifier fields to save modified operands */
void SaveDefault(RiverCodeGen *cg, struct RiverInstruction *rIn, struct RiverInstruction *rOut, DWORD *outCount) {
	if (RIVER_SPEC_MODIFIES_xSP & rIn->specifiers) {
		__asm int 3;
	}

	SaveOperands(cg, rIn, rOut, outCount);
}

void SavePush(RiverCodeGen *cg, struct RiverInstruction *rIn, struct RiverInstruction *rOut, DWORD *outCount) {
	RiverRegister tmpReg;

	MakeSaveAtEsp(cg, rOut);
	(*outCount)++;
	rOut++;

	tmpReg.versioned = RIVER_REG_xSP;
	MakeSaveReg(cg, rOut, &tmpReg, RIVER_MODIFIER_ORIG_xSP);
	(*outCount)++;
	rOut++;

	tmpReg.versioned = RIVER_REG_xSP;
	MakeSubNoFlagsRegImm8(cg, rOut, &tmpReg, 0x04, RIVER_MODIFIER_ORIG_xSP | RIVER_MODIFIER_METAOP);
	(*outCount)++;
	rOut++;


	//MakeCustomAddEsp4
	//*outCount++;
	//rOut++;
	
	SaveOperands(cg, rIn, rOut, outCount);
}

void SavePop(RiverCodeGen *cg, struct RiverInstruction *rIn, struct RiverInstruction *rOut, DWORD *outCount) {
	RiverRegister tmpReg;

	tmpReg.name = RIVER_REG_xSP;
	MakeSaveReg(cg, rOut, &tmpReg, RIVER_MODIFIER_ORIG_xSP);
	(*outCount)++;
	rOut++; 

	tmpReg.versioned = RIVER_REG_xSP;
	MakeAddNoFlagsRegImm8(cg, rOut, &tmpReg, 0x04, RIVER_MODIFIER_ORIG_xSP | RIVER_MODIFIER_METAOP);
	(*outCount)++;
	rOut++;
	
	
	//MakeCustomSubEsp4
	//*outCount++;
	//rOut++;
	SaveOperands(cg, rIn, rOut, outCount);
}

extern const ConvertInstructionFunc riverSaveCode00[];
extern const ConvertInstructionFunc riverSaveCode0F[];

void TranslateSave(RiverCodeGen *cg, struct RiverInstruction *rIn, struct RiverInstruction *rOut, DWORD *outCount) {
	const ConvertInstructionFunc *cvtTbl = riverSaveCode00;

	if (RIVER_MODIFIER_EXT & rIn->modifiers) {
		cvtTbl = riverSaveCode0F;
	}

	cvtTbl[rIn->opCode](cg, rIn, rOut, outCount);
}


const ConvertInstructionFunc riverSaveCode00[] = {
	/*0x00*/SaveUnknown, SaveUnknown, SaveUnknown, SaveUnknown,
	/*0x04*/SaveUnknown, SaveUnknown, SaveUnknown, SaveUnknown,
	/*0x08*/SaveUnknown, SaveUnknown, SaveUnknown, SaveUnknown,
	/*0x0C*/SaveUnknown, SaveUnknown, SaveUnknown, SaveUnknown,

	/*0x10*/SaveUnknown, SaveUnknown, SaveUnknown, SaveUnknown,
	/*0x14*/SaveUnknown, SaveUnknown, SaveUnknown, SaveUnknown,
	/*0x18*/SaveUnknown, SaveUnknown, SaveUnknown, SaveUnknown,
	/*0x1C*/SaveUnknown, SaveUnknown, SaveUnknown, SaveUnknown,

	/*0x20*/SaveUnknown, SaveUnknown, SaveUnknown, SaveUnknown,
	/*0x24*/SaveUnknown, SaveUnknown, SaveUnknown, SaveUnknown,
	/*0x28*/SaveUnknown, SaveUnknown, SaveUnknown, SaveUnknown,
	/*0x2C*/SaveUnknown, SaveUnknown, SaveUnknown, SaveUnknown,

	/*0x30*/SaveUnknown, SaveUnknown, SaveUnknown, SaveDefault,
	/*0x34*/SaveUnknown, SaveUnknown, SaveUnknown, SaveUnknown,
	/*0x38*/SaveUnknown, SaveUnknown, SaveUnknown, SaveDefault,
	/*0x3C*/SaveUnknown, SaveUnknown, SaveUnknown, SaveUnknown,

	/*0x40*/SaveDefault, SaveDefault, SaveDefault, SaveDefault,
	/*0x44*/SaveDefault, SaveDefault, SaveDefault, SaveDefault,
	/*0x48*/SaveDefault, SaveDefault, SaveDefault, SaveDefault,
	/*0x4C*/SaveDefault, SaveDefault, SaveDefault, SaveDefault,

	/*0x50*/SavePush, SavePush, SavePush, SavePush,
	/*0x54*/SavePush, SavePush, SavePush, SavePush,
	/*0x58*/SavePop, SavePop, SavePop, SavePop,
	/*0x5C*/SavePop, SavePop, SavePop, SavePop,

	/*0x60*/SaveUnknown, SaveUnknown, SaveUnknown, SaveUnknown,
	/*0x64*/SaveUnknown, SaveUnknown, SaveUnknown, SaveUnknown,
	/*0x68*/SaveUnknown, SaveUnknown, SaveUnknown, SaveUnknown,
	/*0x6C*/SaveUnknown, SaveUnknown, SaveUnknown, SaveUnknown,

	/*0x70*/SaveDefault, SaveDefault, SaveDefault, SaveDefault,
	/*0x74*/SaveDefault, SaveDefault, SaveDefault, SaveDefault,
	/*0x78*/SaveDefault, SaveDefault, SaveDefault, SaveDefault,
	/*0x7C*/SaveDefault, SaveDefault, SaveDefault, SaveDefault,

	/*0x80*/SaveUnknown, SaveUnknown, SaveUnknown, SaveDefault,
	/*0x84*/SaveUnknown, SaveUnknown, SaveUnknown, SaveUnknown,
	/*0x88*/SaveUnknown, SaveDefault, SaveUnknown, SaveDefault,
	/*0x8C*/SaveUnknown, SaveUnknown, SaveUnknown, SaveUnknown,

	/*0x90*/SaveUnknown, SaveUnknown, SaveUnknown, SaveUnknown,
	/*0x94*/SaveUnknown, SaveUnknown, SaveUnknown, SaveUnknown,
	/*0x98*/SaveUnknown, SaveUnknown, SaveUnknown, SaveUnknown,
	/*0x9C*/SaveUnknown, SaveUnknown, SaveUnknown, SaveUnknown,

	/*0xA0*/SaveUnknown, SaveUnknown, SaveUnknown, SaveUnknown,
	/*0xA4*/SaveUnknown, SaveUnknown, SaveUnknown, SaveUnknown,
	/*0xA8*/SaveUnknown, SaveUnknown, SaveUnknown, SaveUnknown,
	/*0xAC*/SaveUnknown, SaveUnknown, SaveUnknown, SaveUnknown,

	/*0xB0*/SaveDefault, SaveDefault, SaveDefault, SaveDefault,
	/*0xB4*/SaveDefault, SaveDefault, SaveDefault, SaveDefault,
	/*0xB8*/SaveDefault, SaveDefault, SaveDefault, SaveDefault,
	/*0xBC*/SaveDefault, SaveDefault, SaveDefault, SaveDefault,

	/*0xC0*/SaveUnknown, SaveUnknown, SaveUnknown, SavePop,
	/*0xC4*/SaveUnknown, SaveUnknown, SaveUnknown, SaveUnknown,
	/*0xC8*/SaveUnknown, SaveUnknown, SaveUnknown, SaveUnknown,
	/*0xCC*/SaveUnknown, SaveUnknown, SaveUnknown, SaveUnknown,

	/*0xD0*/SaveUnknown, SaveUnknown, SaveUnknown, SaveUnknown,
	/*0xD4*/SaveUnknown, SaveUnknown, SaveUnknown, SaveUnknown,
	/*0xD8*/SaveUnknown, SaveUnknown, SaveUnknown, SaveUnknown,
	/*0xDC*/SaveUnknown, SaveUnknown, SaveUnknown, SaveUnknown,

	/*0xE0*/SaveUnknown, SaveUnknown, SaveUnknown, SaveUnknown,
	/*0xE4*/SaveUnknown, SaveUnknown, SaveUnknown, SaveUnknown,
	/*0xE8*/SaveUnknown, SaveOperands, SaveUnknown, SaveUnknown,
	/*0xEC*/SaveUnknown, SaveUnknown, SaveUnknown, SaveUnknown,

	/*0xF0*/SaveUnknown, SaveUnknown, SaveUnknown, SaveUnknown,
	/*0xF4*/SaveUnknown, SaveUnknown, SaveUnknown, SaveUnknown,
	/*0xF8*/SaveUnknown, SaveUnknown, SaveUnknown, SaveUnknown,
	/*0xFC*/SaveUnknown, SaveUnknown, SaveUnknown, SaveUnknown
};

const ConvertInstructionFunc riverSaveCode0F[] = {
	/*0x00*/SaveUnknown, SaveUnknown, SaveUnknown, SaveUnknown,
	/*0x04*/SaveUnknown, SaveUnknown, SaveUnknown, SaveUnknown,
	/*0x08*/SaveUnknown, SaveUnknown, SaveUnknown, SaveUnknown,
	/*0x0C*/SaveUnknown, SaveUnknown, SaveUnknown, SaveUnknown,

	/*0x10*/SaveUnknown, SaveUnknown, SaveUnknown, SaveUnknown,
	/*0x14*/SaveUnknown, SaveUnknown, SaveUnknown, SaveUnknown,
	/*0x18*/SaveUnknown, SaveUnknown, SaveUnknown, SaveUnknown,
	/*0x1C*/SaveUnknown, SaveUnknown, SaveUnknown, SaveUnknown,

	/*0x20*/SaveUnknown, SaveUnknown, SaveUnknown, SaveUnknown,
	/*0x24*/SaveUnknown, SaveUnknown, SaveUnknown, SaveUnknown,
	/*0x28*/SaveUnknown, SaveUnknown, SaveUnknown, SaveUnknown,
	/*0x2C*/SaveUnknown, SaveUnknown, SaveUnknown, SaveUnknown,

	/*0x30*/SaveUnknown, SaveUnknown, SaveUnknown, SaveUnknown,
	/*0x34*/SaveUnknown, SaveUnknown, SaveUnknown, SaveUnknown,
	/*0x38*/SaveUnknown, SaveUnknown, SaveUnknown, SaveUnknown,
	/*0x3C*/SaveUnknown, SaveUnknown, SaveUnknown, SaveUnknown,

	/*0x40*/SaveUnknown, SaveUnknown, SaveUnknown, SaveUnknown,
	/*0x44*/SaveUnknown, SaveUnknown, SaveUnknown, SaveUnknown,
	/*0x48*/SaveUnknown, SaveUnknown, SaveUnknown, SaveUnknown,
	/*0x4C*/SaveUnknown, SaveUnknown, SaveUnknown, SaveUnknown,

	/*0x50*/SaveUnknown, SaveUnknown, SaveUnknown, SaveUnknown,
	/*0x54*/SaveUnknown, SaveUnknown, SaveUnknown, SaveUnknown,
	/*0x58*/SaveUnknown, SaveUnknown, SaveUnknown, SaveUnknown,
	/*0x5C*/SaveUnknown, SaveUnknown, SaveUnknown, SaveUnknown,

	/*0x60*/SaveUnknown, SaveUnknown, SaveUnknown, SaveUnknown,
	/*0x64*/SaveUnknown, SaveUnknown, SaveUnknown, SaveUnknown,
	/*0x68*/SaveUnknown, SaveUnknown, SaveUnknown, SaveUnknown,
	/*0x6C*/SaveUnknown, SaveUnknown, SaveUnknown, SaveUnknown,

	/*0x70*/SaveUnknown, SaveUnknown, SaveUnknown, SaveUnknown,
	/*0x74*/SaveUnknown, SaveUnknown, SaveUnknown, SaveUnknown,
	/*0x78*/SaveUnknown, SaveUnknown, SaveUnknown, SaveUnknown,
	/*0x7C*/SaveUnknown, SaveUnknown, SaveUnknown, SaveUnknown,

	/*0x80*/SaveUnknown, SaveUnknown, SaveUnknown, SaveUnknown,
	/*0x84*/SaveUnknown, SaveUnknown, SaveUnknown, SaveUnknown,
	/*0x88*/SaveUnknown, SaveUnknown, SaveUnknown, SaveUnknown,
	/*0x8C*/SaveUnknown, SaveUnknown, SaveUnknown, SaveUnknown,

	/*0x90*/SaveUnknown, SaveUnknown, SaveUnknown, SaveUnknown,
	/*0x94*/SaveUnknown, SaveUnknown, SaveUnknown, SaveUnknown,
	/*0x98*/SaveUnknown, SaveUnknown, SaveUnknown, SaveUnknown,
	/*0x9C*/SaveUnknown, SaveUnknown, SaveUnknown, SaveUnknown,

	/*0xA0*/SaveUnknown, SaveUnknown, SaveUnknown, SaveUnknown,
	/*0xA4*/SaveUnknown, SaveUnknown, SaveUnknown, SaveUnknown,
	/*0xA8*/SaveUnknown, SaveUnknown, SaveUnknown, SaveUnknown,
	/*0xAC*/SaveUnknown, SaveUnknown, SaveUnknown, SaveUnknown,

	/*0xB0*/SaveUnknown, SaveUnknown, SaveUnknown, SaveUnknown,
	/*0xB4*/SaveUnknown, SaveUnknown, SaveUnknown, SaveUnknown,
	/*0xB8*/SaveUnknown, SaveUnknown, SaveUnknown, SaveUnknown,
	/*0xBC*/SaveUnknown, SaveUnknown, SaveUnknown, SaveUnknown,

	/*0xC0*/SaveUnknown, SaveUnknown, SaveUnknown, SaveUnknown,
	/*0xC4*/SaveUnknown, SaveUnknown, SaveUnknown, SaveUnknown,
	/*0xC8*/SaveUnknown, SaveUnknown, SaveUnknown, SaveUnknown,
	/*0xCC*/SaveUnknown, SaveUnknown, SaveUnknown, SaveUnknown,

	/*0xD0*/SaveUnknown, SaveUnknown, SaveUnknown, SaveUnknown,
	/*0xD4*/SaveUnknown, SaveUnknown, SaveUnknown, SaveUnknown,
	/*0xD8*/SaveUnknown, SaveUnknown, SaveUnknown, SaveUnknown,
	/*0xDC*/SaveUnknown, SaveUnknown, SaveUnknown, SaveUnknown,

	/*0xE0*/SaveUnknown, SaveUnknown, SaveUnknown, SaveUnknown,
	/*0xE4*/SaveUnknown, SaveUnknown, SaveUnknown, SaveUnknown,
	/*0xE8*/SaveUnknown, SaveUnknown, SaveUnknown, SaveUnknown,
	/*0xEC*/SaveUnknown, SaveUnknown, SaveUnknown, SaveUnknown,

	/*0xF0*/SaveUnknown, SaveUnknown, SaveUnknown, SaveUnknown,
	/*0xF4*/SaveUnknown, SaveUnknown, SaveUnknown, SaveUnknown,
	/*0xF8*/SaveUnknown, SaveUnknown, SaveUnknown, SaveUnknown,
	/*0xFC*/SaveUnknown, SaveUnknown, SaveUnknown, SaveUnknown
};