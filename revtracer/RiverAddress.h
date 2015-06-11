#ifndef _RIVER_ADDRESS_H
#define _RIVER_ADDRESS_H

//#include "river.h"
//#include "CodeGen.h"		

class RiverCodeGen;

struct RiverAddress {
	BYTE type; 						/* 0x00 - combination of RIVER_ADDR* flags */
	BYTE scaleAndSegment;			/* 0x01 - scale 1, 2, 4, or 8 */
	BYTE modRM; 					/* 0x02 - original modrm byte */
	BYTE sib;   					/* 0x03 - original sib byte */
	union RiverRegister base;		/* 0x04 - base register */
	union RiverRegister index;		/* 0x08 - index register */
	union {
		BYTE d8;
		DWORD d32;
	} disp;							/* 0x0C - displacement */

	inline BYTE GetScale() const {
		return 1 << (scaleAndSegment & 0x3);
	}

	inline BYTE GetScaleBits() const {
		return scaleAndSegment & 0x03;
	}

	inline void CopyScale(BYTE scale) {
		scaleAndSegment &= 0xFC;
		scaleAndSegment |= scale;
	}

	inline bool SetScale(BYTE scale) {
		static const BYTE sTbl[] = { 0xFF, 0, 1, 0xFF, 2, 0xFF, 0xFF, 0xFF, 4 };
		BYTE sb = sTbl[scale];
		if (0xFF == sb) {
			return false;
		}
		CopyScale(sb);
	}

	inline BYTE HasSegment() const {
		return scaleAndSegment & 0xFC;
	}

	inline BYTE GetSegment() const {
		return scaleAndSegment >> 2;
	}

	inline void SetSegment(BYTE segment) {
		scaleAndSegment &= 0x03;
		scaleAndSegment |= segment << 2;
	}

	void DecodeFlags(WORD flags);

	virtual bool DecodeFromx86(RiverCodeGen &cg, unsigned char *&px86, BYTE &extra, WORD flags) = 0;
	virtual bool EncodeTox86(unsigned char *&px86, BYTE extra, WORD flags) = 0;
};

struct RiverAddress16 : public RiverAddress {
	bool DecodeSib(RiverCodeGen &cg, unsigned char *&px86);

	virtual bool DecodeFromx86(RiverCodeGen &cg, unsigned char *&px86, BYTE &extra, WORD flags);
	virtual bool EncodeTox86(unsigned char *&px86, BYTE extra, WORD flags);
};

struct RiverAddress32 : public RiverAddress {
	void FixEsp();
	bool CleanAddr(WORD flags);
	
	BYTE DecodeRegister(BYTE id, WORD flags);
	BYTE EncodeRegister(WORD flags);

	bool DecodeSib(RiverCodeGen &cg, unsigned char *&px86);

	virtual bool DecodeFromx86(RiverCodeGen &cg, unsigned char *&px86, BYTE &extra, WORD flags);
	virtual bool EncodeTox86(unsigned char *&px86, BYTE extra, WORD flags);
};


#endif
