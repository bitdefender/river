struct RiverAddress {
	nodep::BYTE type;
	nodep::BYTE scaleAndSegment;
	nodep::BYTE modRM;
	nodep::BYTE sib;
	union RiverRegister base;
	union RiverRegister index;
	union {
		nodep::BYTE d8;
		nodep::DWORD d32;
	} disp;

	inline nodep::BYTE GetScale() const {
		return 1 << (scaleAndSegment & 0x3);
	}

	inline nodep::BYTE GetScaleBits() const {
		return scaleAndSegment & 0x03;
	}

	inline nodep::BYTE HasSegment() const {
		return scaleAndSegment & 0xFC;
	}

	inline nodep::BYTE GetSegment() const {
		return scaleAndSegment >> 2;
	}
	...
};
