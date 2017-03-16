#ifndef _RIVER_ADDRESS_H
#define _RIVER_ADDRESS_H

//#include "river.h"
//#include "CodeGen.h"		

class RiverCodeGen;

struct RiverAddress {
	nodep::BYTE type; 						/* 0x00 - combination of RIVER_ADDR* flags */
	nodep::BYTE scaleAndSegment;			/* 0x01 - scale 1, 2, 4, or 8 */
	nodep::BYTE modRM; 					/* 0x02 - original modrm byte */
	nodep::BYTE sib;   					/* 0x03 - original sib byte */
	union RiverRegister base;		/* 0x04 - base register */
	union RiverRegister index;		/* 0x08 - index register */
	union {
		nodep::BYTE d8;
		nodep::DWORD d32;
	} disp;							/* 0x0C - displacement */

	inline nodep::BYTE GetScale() const {
		return 1 << (scaleAndSegment & 0x3);
	}

	inline nodep::BYTE GetScaleBits() const {
		return scaleAndSegment & 0x03;
	}

	inline void CopyScale(nodep::BYTE scale) {
		scaleAndSegment &= 0xFC;
		scaleAndSegment |= scale;
	}

	inline bool SetScale(nodep::BYTE scale) {
		static const nodep::BYTE sTbl[] = { 0xFF, 0, 1, 0xFF, 2, 0xFF, 0xFF, 0xFF, 4 };
		nodep::BYTE sb = sTbl[scale];
		if (0xFF == sb) {
			return false;
		}
		CopyScale(sb);
	}

	inline nodep::BYTE HasSegment() const {
		return scaleAndSegment & 0xFC;
	}

	inline nodep::BYTE GetSegment() const {
		return scaleAndSegment >> 2;
	}

	inline void SetSegment(nodep::BYTE segment) {
		scaleAndSegment &= 0x03;
		scaleAndSegment |= segment << 2;
	}

	nodep::BYTE GetUnusedRegisters() const;

	void DecodeFlags(nodep::WORD flags);

	virtual bool DecodeFromx86(RiverCodeGen &cg, unsigned char *&px86, nodep::BYTE &extra, nodep::WORD flags) = 0;
	virtual bool EncodeTox86(unsigned char *&px86, nodep::BYTE extra, nodep::BYTE family, nodep::WORD modifiers) = 0;
};

struct RiverAddress16 : public RiverAddress {
	bool DecodeSib(RiverCodeGen &cg, unsigned char *&px86);

	virtual bool DecodeFromx86(RiverCodeGen &cg, unsigned char *&px86, nodep::BYTE &extra, nodep::WORD flags);
	virtual bool EncodeTox86(unsigned char *&px86, nodep::BYTE extra, nodep::BYTE family, nodep::WORD modifiers);
};

struct RiverAddress32 : public RiverAddress {
	void FixEsp();
	bool CleanAddr(nodep::WORD flags);
	
	nodep::BYTE DecodeRegister(nodep::BYTE id, nodep::WORD flags);
	nodep::BYTE EncodeRegister(nodep::WORD flags);

	bool DecodeSib(RiverCodeGen &cg, unsigned char *&px86);

	virtual bool DecodeFromx86(RiverCodeGen &cg, unsigned char *&px86, nodep::BYTE &extra, nodep::WORD flags);
	virtual bool EncodeTox86(unsigned char *&px86, nodep::BYTE extra, nodep::BYTE family, nodep::WORD modifiers);
};


#endif
