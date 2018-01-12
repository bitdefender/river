#include "river.h"
#include "CodeGen.h"

nodep::BYTE GetFundamentalRegister(nodep::BYTE reg) {
	if (reg < 0x20) {
		return reg & 0x07;
	}
	return reg;
}

void RiverAddress::DecodeFlags(nodep::WORD flags) {
	if (flags & 0x0007) {
		SetSegment((nodep::BYTE)flags & 0x0007);
	}
}

nodep::BYTE RiverAddress::GetUnusedRegisters() const {
	nodep::BYTE ret = 0xCF;
	nodep::BYTE tmp;

	if ((0 == type) || (type & RIVER_ADDR_BASE)) {
		tmp = GetFundamentalRegister(base.name);
		ret &= ~(1 << tmp);
	}

	if (type & RIVER_ADDR_INDEX) {
		tmp = GetFundamentalRegister(index.name);
		ret &= ~(1 << tmp);
	}

	return ret;
}

nodep::BYTE RiverAddress32::DecodeRegister(nodep::BYTE id, nodep::WORD flags) {
	if (flags & RIVER_MODIFIER_O8) { // we expect a 8 byte register
		return (id & 0x03) | ((id & 0x04) ? RIVER_REG_SZ8_H : RIVER_REG_SZ8_L);
	} else if (flags & RIVER_MODIFIER_O16) { // we expect a 16 byte register
		return id | RIVER_REG_SZ16;
	} else {
		return id | RIVER_REG_SZ32;
	}
}

nodep::BYTE RiverAddress32::EncodeRegister(nodep::WORD flags) {
	if (flags & RIVER_MODIFIER_O8) { // we expect a 8 byte register
		return (base.name & 0x03) + (((base.name & 0x04) == RIVER_REG_SZ8_H) ? 4 : 0);
	} else {
		return base.name & 0x07;
	}
}

bool RiverAddress32::DecodeSib(RiverCodeGen &cg, unsigned char *&px86) {
	sib = *px86;

	nodep::BYTE ss = sib >> 6;
	nodep::BYTE idx = (sib >> 3) & 0x7;
	nodep::BYTE bs = sib & 0x7;

	type |= RIVER_ADDR_SCALE;
	CopyScale(ss);

	if (4 != idx) {
		type |= RIVER_ADDR_INDEX;
		index.versioned = cg.GetCurrentReg(idx);
	}

	if (5 == bs) {
		nodep::BYTE mod = modRM >> 6;

		type |= (1 & mod) ? RIVER_ADDR_DISP8 : RIVER_ADDR_DISP;
		if (0 != mod) {
			type |= RIVER_ADDR_BASE;
			base.versioned = cg.GetCurrentReg(RIVER_REG_xBP);
		}
	} else {
		type |= RIVER_ADDR_BASE;
		base.versioned = cg.GetCurrentReg(bs);
	}

	px86++;
	return true;
}

bool RiverAddress32::DecodeFromx86(RiverCodeGen &cg, unsigned char *&px86, nodep::BYTE &extra, nodep::WORD flags) {
	scaleAndSegment = 0;
	DecodeFlags(flags);
	index.versioned = RIVER_REG_NONE;
	base.versioned = RIVER_REG_NONE;
	sib = 0;
	disp.d32 = 0;
	type = 0;

	modRM = *px86;
	px86++;

	nodep::BYTE mod = modRM >> 6, rm = modRM & 0x07;
	extra = (modRM >> 3) & 0x07;

	switch (mod) {
		case 0 :
			type = 0;
			break;
		case 1 :
			type = RIVER_ADDR_DISP8;
			break;
		case 2 :
			type = RIVER_ADDR_DISP;
			break;
		case 3 :
			type = 0; // direct register
			base.versioned = cg.GetCurrentReg(DecodeRegister(rm, flags));
			return true;
	}

	if (4 == rm) {
		DecodeSib(cg, px86);
	} else if ((0 == mod) && (5 == rm)) {
		type = RIVER_ADDR_DISP;
	} else {
		type |= RIVER_ADDR_BASE;
		base.versioned = cg.GetCurrentReg(rm);
	}

	if (type & RIVER_ADDR_DISP8) {
		disp.d8 = *px86;
		px86++;
	} else if (type & RIVER_ADDR_DISP) {
		disp.d32 = *(nodep::DWORD *)px86;
		px86 += 4;
	}

	return true;
}

#define RIVER_REG_EXP32(rg)  ((rg) & 0x07)

void RiverAddress32::FixEsp() {
	if (RIVER_REG_NONE != base.versioned) {
		if (RIVER_REG_EXP32(base.name) == RIVER_REG_xSP) {
			base.versioned = RIVER_REG_xAX;
			type |= RIVER_ADDR_DIRTY;
		}
	}

	if (type & RIVER_ADDR_INDEX) {
		if (RIVER_REG_EXP32(index.name) == RIVER_REG_xSP) {
			index.name = RIVER_REG_xAX;
			type |= RIVER_ADDR_DIRTY;
		}
	}
}

bool RiverAddress32::CleanAddr(nodep::WORD flags) {
	unsigned char mod, rm;
	
	type &= ~RIVER_ADDR_DIRTY;
	if (0 == type) {
		mod = 3;
		rm = EncodeRegister(flags);
	} else {

		// move esp from base to index
		if (type & RIVER_ADDR_BASE) {
			if (RIVER_REG_EXP32(base.name) == RIVER_REG_xSP) {
				if (type & RIVER_ADDR_INDEX) {
					return false;
				} else {
					index.versioned = base.versioned;
					base.name = RIVER_REG_NONE;

					type &= ~RIVER_ADDR_BASE;
					type |= RIVER_ADDR_INDEX | RIVER_ADDR_SCALE;

					scaleAndSegment = 0;
				}
			}
		}

		switch (type & (RIVER_ADDR_DISP8 | RIVER_ADDR_DISP)) {
			case 0:
				mod = 0;
				break;
			case RIVER_ADDR_DISP8:
				mod = 1;
				break;
			case RIVER_ADDR_DISP:
				mod = 2;
				break;
			default:
				return false;
		}

		if ((type & RIVER_ADDR_INDEX) || (type & RIVER_ADDR_SCALE)) {
		//if ((type & RIVER_ADDR_INDEX) && (type & RIVER_ADDR_SCALE)) {
			// handle sib
			rm = 4;

			unsigned char ss, idx, bs;
			if (type & RIVER_ADDR_SCALE) {
				ss = GetScaleBits();
			} else {
				ss = 0;
			}

			if (type & RIVER_ADDR_BASE) {
				bs = base.name;
			} else {
				bs = 5;
			}

			if (type & RIVER_ADDR_INDEX) {
				idx = index.name;
			} else {
				idx = 4;
			}

			sib = (ss << 6) | (idx << 3) | (bs);
		} else {
			if (0 == (type & RIVER_ADDR_BASE)) { // there is no base
				if (type & RIVER_ADDR_DISP) { // simply a displacement
					mod = 0;
					rm = 5;
				} else { // no 32bit displacement either
					return false;
				}
			} else {
				rm = base.name;
			}
		}
	}

	modRM = (mod << 6) | rm;
	return true;
}


bool RiverAddress32::EncodeTox86(unsigned char *&px86, nodep::BYTE extra, nodep::BYTE family, nodep::WORD modifiers) {
	if (family & RIVER_FAMILY_FLAG_ORIG_xSP) {
		RiverAddress32 rAddr;
		rev_memcpy(&rAddr, this, sizeof(rAddr));

		rAddr.FixEsp();
		return rAddr.EncodeTox86(px86, extra, family & (~RIVER_FAMILY_FLAG_ORIG_xSP), modifiers);
	}

	if (type & RIVER_ADDR_DIRTY) {
		if (!CleanAddr(modifiers)) {
			DEBUG_BREAK;
			return false;
		}
	}

	*px86 = (modRM & 0xC7) | (extra << 3);
	px86++;

	if (type & (RIVER_ADDR_INDEX | RIVER_ADDR_SCALE)) {
		*px86 = sib;
		px86++;
	}

	if (type & RIVER_ADDR_DISP8) {
		*px86 = disp.d8;
		px86++;
	} else if (type & RIVER_ADDR_DISP) {
		*(nodep::DWORD *)px86 = disp.d32;
		px86 += 4;
	}

	return true;
}
