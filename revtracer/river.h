#ifndef _RIVER_H
#define _RIVER_H

/* river (spelled backwards) stands for the REVersible Intermediate Representation */
/* river is a fixed length extended x86 instruction set */
/* river is designed to be efficiently translated to and from x86 */

#include "environment.h"

/* River mofifiers (present in riverInstruction.modifiers) */
#define RIVER_MODIFIER_EXT 			0x0001
#define RIVER_MODIFIER_ESSEG		0x0002
#define RIVER_MODIFIER_CSSEG		0x0004
#define RIVER_MODIFIER_SSSEG		0x0008
#define RIVER_MODIFIER_DSSEG		0x0010
#define RIVER_MODIFIER_FSSEG		0x0020
#define RIVER_MODIFIER_GSSEG		0x0040
/* no corresponding prefix */
#define RIVER_MODIFIER_O8			0x0080
#define RIVER_MODIFIER_O16 			0x0100
#define RIVER_MODIFIER_A16 			0x0200
#define RIVER_MODIFIER_LOCK			0x0400
#define RIVER_MODIFIER_REP 			0x0800
#define RIVER_MODIFIER_REPZ			0x1000
#define RIVER_MODIFIER_REPNZ		0x2000

/* this instruction is a meta instruction... it respecifies the previous one in order to optimize reverse code generation */
#define RIVER_MODIFIER_METAOP		0x1000
/* lazy deletion flag */
#define RIVER_MODIFIER_IGNORE		0x2000
/* this river instruction uses the original xSP register */
#define RIVER_MODIFIER_ORIG_xSP		0x4000
/* this is a river instruction */
#define RIVER_MODIFIER_RIVEROP		0x8000

/* River translation flags present in the pFlags argument */
#define RIVER_FLAG_PFX				0x00000001
#define RIVER_FLAG_OPCODE			0x00000002
#define RIVER_FLAG_BRANCH			0x00000100

/* River virtual register names */
#define RIVER_REG_xAX				0x00
#define RIVER_REG_xCX				0x01
#define RIVER_REG_xDX				0x02
#define RIVER_REG_xBX				0x03
#define RIVER_REG_xSP				0x04
#define RIVER_REG_xBP				0x05
#define RIVER_REG_xSI				0x06
#define RIVER_REG_xDI				0x07

/* River virtual register sizes, to e used in conjunction with the register names*/
#define RIVER_REG_SZ32				0x00
#define RIVER_REG_SZ16				0x08
#define RIVER_REG_SZ8_L				0x10
#define RIVER_REG_SZ8_H				0x18

/* River void virtual register */
#define RIVER_REG_NONE				0x20

union RiverRegister {
	DWORD versioned;
	BYTE name;
};

/* River address components, to be used in RiverAddress::type */
#define RIVER_ADDR_DISP8			0x01
#define RIVER_ADDR_DISP				0x02
#define RIVER_ADDR_SCALE			0x04
#define RIVER_ADDR_BASE				0x08
#define RIVER_ADDR_INDEX			0x10
/* Marks the address as needing recalculation */
#define RIVER_ADDR_DIRTY			0x80

#include "RiverAddress.h"

/* River operand types */
#define RIVER_OPTYPE_IMM			0x00
#define RIVER_OPTYPE_REG			0x04
#define RIVER_OPTYPE_MEM			0x08
#define RIVER_OPTYPE_NONE			0xFF

/* River operand sizes */
#define RIVER_OPSIZE_32				0x00
#define RIVER_OPSIZE_16				0x01
#define RIVER_OPSIZE_8				0x02

/* River operation specifiers */
#define RIVER_SPEC_MODIFIES_OP1		0x0001
#define RIVER_SPEC_MODIFIES_OP2		0x0002
#define RIVER_SPEC_MODIFIES_FLG		0x0004
/* Modifies some onther fields also (maybe use a function table) */
#define RIVER_SPEC_MODIFIES_xSP		0x0008

/* Use a secondary table for lookup*/
#define RIVER_SPEC_MODIFIES_EXT		0x8000

union RiverOperand {
	BYTE asImm8;
	WORD asImm16;
	DWORD asImm32;
	RiverRegister asRegister;
	RiverAddress *asAddress;
};

struct RiverInstruction {
	WORD modifiers;
	WORD specifiers;
	BYTE opCode;

	BYTE subOpCode;

	BYTE opTypes[2];
	union RiverOperand operands[2];
};

//#include "execenv.h"



#endif
