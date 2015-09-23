#ifndef _RIVER_H
#define _RIVER_H

/* river (spelled backwards) stands for the REVersible Intermediate Representation */
/* river is a fixed length extended x86 instruction set */
/* river is designed to be efficiently translated to and from x86 */

#include "environment.h"

/* River mofifiers (present in riverInstruction.modifiers) */
#define RIVER_MODIFIER_NOSEG		0x0000
#define RIVER_MODIFIER_ESSEG		0x0001
#define RIVER_MODIFIER_CSSEG		0x0002
#define RIVER_MODIFIER_SSSEG		0x0003
#define RIVER_MODIFIER_DSSEG		0x0004
#define RIVER_MODIFIER_FSSEG		0x0005
#define RIVER_MODIFIER_GSSEG		0x0006


#define RIVER_MODIFIER_EXT 			0x0008
/* no corresponding prefix */
#define RIVER_MODIFIER_O8			0x0010
#define RIVER_MODIFIER_O16 			0x0020
#define RIVER_MODIFIER_A16 			0x0040
#define RIVER_MODIFIER_LOCK			0x0080
#define RIVER_MODIFIER_REP 			0x0100
#define RIVER_MODIFIER_REPZ			0x0200
#define RIVER_MODIFIER_REPNZ		0x0400

#define RIVER_FAMILY(fam) ((fam) & 0x1F)

#define RIVER_FAMILY_NATIVE				0x00
#define RIVER_FAMILY_RIVER				0x01
#define RIVER_FAMILY_TRACK				0x02
#define RIVER_FAMILY_PRETRACK			0x03
#define RIVER_FAMILY_PREMETAOP			0x04
#define RIVER_FAMILY_POSTMETAOP			0x05

#define RIVER_FAMILY_FLAG_METAPROCESSED	0x20
#define RIVER_FAMILY_FLAG_ORIG_xSP		0x40
#define RIVER_FAMILY_FLAG_IGNORE		0x80

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

#define RIVER_REG_SEGMENT			0x20
#define RIVER_REG_ES				0x20
#define RIVER_REG_CS				0x21
#define RIVER_REG_SS				0x22
#define RIVER_REG_DS				0x23
#define RIVER_REG_FS				0x24
#define RIVER_REG_GS				0x25

#define RIVER_REG_CONTROL			0x30
#define RIVER_REG_CR0				0x30
#define RIVER_REG_CR2				0x32
#define RIVER_REG_CR3				0x33
#define RIVER_REG_CR4				0x34

#define RIVER_REG_DEBUG				0x40
#define RIVER_REG_DR0				0x40
#define RIVER_REG_DR1				0x41
#define RIVER_REG_DR2				0x42
#define RIVER_REG_DR3				0x43
#define RIVER_REG_DR4				0x44
#define RIVER_REG_DR5				0x45
#define RIVER_REG_DR6				0x46
#define RIVER_REG_DR7				0x47

BYTE GetFundamentalRegister(BYTE reg);

/* TODO: add MM0-7 and XMM0-7 */

/* River void virtual register */
#define RIVER_REG_NONE				0x20

/* Define some widely used registers */
#define RIVER_REG_AL				(RIVER_REG_xAX | RIVER_REG_SZ8_L)
#define RIVER_REG_AH				(RIVER_REG_xAX | RIVER_REG_SZ8_H)
#define RIVER_REG_CL				(RIVER_REG_xCX | RIVER_REG_SZ8_L)

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
#define RIVER_OPTYPE_NONE			0x00
#define RIVER_OPTYPE_IMM			0x04
#define RIVER_OPTYPE_REG			0x08
#define RIVER_OPTYPE_MEM			0x0C
#define RIVER_OPTYPE_ALL			0x10

#define RIVER_OPTYPE(type) ((type) & 0x1C)
#define RIVER_OPSIZE(type) ((type) & 0x03)

/* River operand sizes */
#define RIVER_OPSIZE_32				0x00
#define RIVER_OPSIZE_16				0x01
#define RIVER_OPSIZE_8				0x02

/* River operand flags */
#define RIVER_OPFLAG_IMPLICIT		0x80

/* River operation specifiers */
#define RIVER_SPEC_MODIFIES_OP1		0x0001
#define RIVER_SPEC_MODIFIES_OP2		0x0002
#define RIVER_SPEC_MODIFIES_OP3		0x0004
#define RIVER_SPEC_MODIFIES_OP4		0x0008
#define RIVER_SPEC_MODIFIES_OP(idx) (0x0001 << (idx))

#define RIVER_SPEC_MODIFIES_FLG		0x0010
/* Modifies some onther fields also (maybe use a function table) */
#define RIVER_SPEC_MODIFIES_xSP		0x0020

/* Modifies customm fields, must have a custom save/restore function */
#define RIVER_SPEC_MODIFIES_CUSTOM  0x0040

/* Memory contents gets not addressed. (LEA for instance). */
#define RIVER_SPEC_IGNORES_MEMORY	0x0080

/* Means that the particular operand is only used as a destination */
#define RIVER_SPEC_IGNORES_OP1		0x0100
#define RIVER_SPEC_IGNORES_OP2		0x0200
#define RIVER_SPEC_IGNORES_OP3		0x0400
#define RIVER_SPEC_IGNORES_OP4		0x0800
#define RIVER_SPEC_IGNORES_OP(idx)	(0x0100 << (idx))
#define RIVER_SPEC_IGNORES_FLG		0x1000

/* Operation flags */
#define RIVER_SPEC_FLAG_CF			0x01
#define RIVER_SPEC_FLAG_PF			0x02
#define RIVER_SPEC_FLAG_AF			0x04
#define RIVER_SPEC_FLAG_ZF			0x08
#define RIVER_SPEC_FLAG_SF			0x10
#define RIVER_SPEC_FLAG_OF			0x20
#define RIVER_SPEC_FLAG_DF			0x40
#define RIVER_SPEC_FLAG_EXT			0x80

#define RIVER_SPEC_FLAG_SZAPC		(RIVER_SPEC_FLAG_PF | RIVER_SPEC_FLAG_AF | RIVER_SPEC_FLAG_ZF | RIVER_SPEC_FLAG_SF | RIVER_SPEC_FLAG_CF)
#define RIVER_SPEC_FLAG_OSZAP		(RIVER_SPEC_FLAG_PF | RIVER_SPEC_FLAG_AF | RIVER_SPEC_FLAG_ZF | RIVER_SPEC_FLAG_SF | RIVER_SPEC_FLAG_OF)
#define RIVER_SPEC_FLAG_OSZAPC		(RIVER_SPEC_FLAG_CF | RIVER_SPEC_FLAG_OSZAP)


/* Use a secondary table for lookup*/
#define RIVER_SPEC_MODIFIES_EXT		0x8000

/* When xSP is needed these registers can be used for the swap */
#define RIVER_UNUSED_xAX			0x01
#define RIVER_UNUSED_xCX			0x02
#define RIVER_UNUSED_xDX			0x04
#define RIVER_UNUSED_xBX			0x08
#define RIVER_UNUSED_ALL			0x0F

union RiverOperand {
	BYTE asImm8;
	WORD asImm16;
	DWORD asImm32;
	RiverRegister asRegister;
	RiverAddress *asAddress;
};

struct RiverInstruction {
	WORD modifiers; // modifiers introduced by prefixes or opcode
	WORD specifiers; // instruction specific flags

	BYTE family; // instruction family (allows for multiple instruction streams to be interleaved
	BYTE unusedRegisters;
	BYTE opCode;
	BYTE subOpCode;

	BYTE modFlags, testFlags;

	BYTE opTypes[4];
	union RiverOperand operands[4];

	inline void PromoteModifiers() {
		for (int i = 0; i < 4; ++i) {
			if (RIVER_OPTYPE_MEM == RIVER_OPTYPE(opTypes[i])) {
				if (operands[i].asAddress->HasSegment()) {
					modifiers |= operands[i].asAddress->GetSegment();
					break;
				}

				/*if ((operands[0].asAddress->type & RIVER_ADDR_BASE) && (RIVER_REG_xSP == GetFundamentalRegister(operands[0].asAddress->base.name))) {
				modifiers |= RIVER_MODIFIER_ORIG_xSP;
				}

				if ((operands[0].asAddress->type & RIVER_ADDR_INDEX) && (RIVER_REG_xSP == GetFundamentalRegister(operands[0].asAddress->index.name))) {
				modifiers |= RIVER_MODIFIER_ORIG_xSP;
				}*/
			}
		}
	}

	inline BYTE GetUnusedRegister() const {
		if (unusedRegisters & 0x03) {
			if (unusedRegisters & 0x01) {
				return RIVER_REG_xAX;
			}
			return RIVER_REG_xCX;
		} else if (unusedRegisters & 0x0C) {
			if (unusedRegisters & 0x04) {
				return RIVER_REG_xDX;
			}
			return RIVER_REG_xBX;
		} else {
			__asm int 3;
		}
	}

	void TrackUnusedRegistersOperand(BYTE optype, const RiverOperand &op) {
		BYTE reg;
		switch (RIVER_OPTYPE(optype)) {
			case RIVER_OPTYPE_IMM:
				break;
			case RIVER_OPTYPE_REG:
				reg = GetFundamentalRegister(op.asRegister.name);
				if (reg < 4) {
					unusedRegisters &= ~(1 << reg);
				}
				break;
			case RIVER_OPTYPE_MEM:
				if ((op.asAddress->type & RIVER_ADDR_BASE) || (op.asAddress->type == 0)){
					reg = GetFundamentalRegister(op.asAddress->base.name);
					if (reg < 4) {
						unusedRegisters &= ~(1 << reg);
					}
				}
				if (op.asAddress->type & RIVER_ADDR_INDEX) {
					reg = GetFundamentalRegister(op.asAddress->index.name);
					if (reg < 4) {
						unusedRegisters &= ~(1 << reg);
					}
				}
				break;
		}
	}

	void TrackEspOperand(BYTE optype, const RiverOperand &op) {
		BYTE reg; 
		switch (RIVER_OPTYPE(optype)) {
			case RIVER_OPTYPE_IMM:
				break;
			case RIVER_OPTYPE_REG:
				reg = GetFundamentalRegister(op.asRegister.name);
				if (RIVER_REG_xSP == reg) {
					specifiers |= RIVER_SPEC_MODIFIES_xSP;
					family |= RIVER_FAMILY_FLAG_ORIG_xSP;
				}
				break;
			case RIVER_OPTYPE_MEM:
				if ((op.asAddress->type & RIVER_ADDR_BASE) || (0 == op.asAddress->type)) {
					reg = GetFundamentalRegister(op.asAddress->base.name);
					if (RIVER_REG_xSP == reg) {
						specifiers |= RIVER_SPEC_MODIFIES_xSP;
						family |= RIVER_FAMILY_FLAG_ORIG_xSP;
					}
				}
				if (op.asAddress->type & RIVER_ADDR_INDEX) {
					reg = GetFundamentalRegister(op.asAddress->index.name);
					if (RIVER_REG_xSP == reg) {
						specifiers |= RIVER_SPEC_MODIFIES_xSP;
						family |= RIVER_FAMILY_FLAG_ORIG_xSP;
					}
				}
				break;
		}

		if (RIVER_FAMILY_NATIVE == RIVER_FAMILY(family)) {
			family &= ~RIVER_FAMILY_FLAG_ORIG_xSP;
		}
	}

	void TrackEspAsParameter() {
		for (int i = 0; i < 4; ++i) {
			TrackEspOperand(opTypes[i], operands[i]);
		}
	}

	void TrackUnusedRegisters() {
		unusedRegisters = 0x00;
		if ((RIVER_SPEC_MODIFIES_xSP & specifiers) || (RIVER_FAMILY_FLAG_ORIG_xSP & family)) {
			unusedRegisters = RIVER_UNUSED_ALL;

			for (int i = 0; i < 4; ++i) {
				//if (RIVER_SPEC_MODIFIES_OP2 & specifiers) {
				TrackUnusedRegistersOperand(opTypes[i], operands[i]);
				//}
			}
		}
	}
};

//#include "execenv.h"



#endif
