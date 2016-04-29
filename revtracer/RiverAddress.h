#ifndef _RIVER_ADDRESS_H
#define _RIVER_ADDRESS_H

//#include "river.h"
//#include "CodeGen.h"		

class RiverCodeGen;

struct RiverAddress {
	rev::BYTE type; 						/* 0x00 - combination of RIVER_ADDR* flags */
	rev::BYTE scaleAndSegment;			/* 0x01 - scale 1, 2, 4, or 8 */
	rev::BYTE modRM; 					/* 0x02 - original modrm byte */
	rev::BYTE sib;   					/* 0x03 - original sib byte */
	union RiverRegister base;		/* 0x04 - base register */
	union RiverRegister index;		/* 0x08 - index register */
	union {
		rev::BYTE d8;
		rev::DWORD d32;
	} disp;							/* 0x0C - displacement */

	inline rev::BYTE GetScale() const {
		return 1 << (scaleAndSegment & 0x3);
	}

	inline rev::BYTE GetScaleBits() const {
		return scaleAndSegment & 0x03;
	}

	inline void CopyScale(rev::BYTE scale) {
		scaleAndSegment &= 0xFC;
		scaleAndSegment |= scale;
	}

	inline bool SetScale(rev::BYTE scale) {
		static const rev::BYTE sTbl[] = { 0xFF, 0, 1, 0xFF, 2, 0xFF, 0xFF, 0xFF, 4 };
		rev::BYTE sb = sTbl[scale];
		if (0xFF == sb) {
			return false;
		}
		CopyScale(sb);
	}

	inline rev::BYTE HasSegment() const {
		return scaleAndSegment & 0xFC;
	}

	inline rev::BYTE GetSegment() const {
		return scaleAndSegment >> 2;
	}

	inline void SetSegment(rev::BYTE segment) {
		scaleAndSegment &= 0x03;
		scaleAndSegment |= segment << 2;
	}

	rev::BYTE GetUnusedRegisters() const;

	void DecodeFlags(rev::WORD flags);

	virtual bool DecodeFromx86(RiverCodeGen &cg, unsigned char *&px86, rev::BYTE &extra, rev::WORD flags) = 0;
	virtual bool EncodeTox86(unsigned char *&px86, rev::BYTE extra, rev::BYTE family, rev::WORD modifiers) = 0;
};

struct RiverAddress16 : public RiverAddress {
	bool DecodeSib(RiverCodeGen &cg, unsigned char *&px86);

	virtual bool DecodeFromx86(RiverCodeGen &cg, unsigned char *&px86, rev::BYTE &extra, rev::WORD flags);
	virtual bool EncodeTox86(unsigned char *&px86, rev::BYTE extra, rev::BYTE family, rev::WORD modifiers);
};

struct RiverAddress32 : public RiverAddress {
	void FixEsp();
	bool CleanAddr(rev::WORD flags);
	
	rev::BYTE DecodeRegister(rev::BYTE id, rev::WORD flags);
	rev::BYTE EncodeRegister(rev::WORD flags);

	bool DecodeSib(RiverCodeGen &cg, unsigned char *&px86);

	virtual bool DecodeFromx86(RiverCodeGen &cg, unsigned char *&px86, rev::BYTE &extra, rev::WORD flags);
	virtual bool EncodeTox86(unsigned char *&px86, rev::BYTE extra, rev::BYTE family, rev::WORD modifiers);
};


#endif
