#include "TrackingX86Assembler.h"

#include "X86AssemblerFuncs.h"
#include "mm.h"

extern DWORD dwAddressTrackHandler;
extern DWORD dwAddressMarkHandler;

void TrackingX86Assembler::AssembleTrackFlag(DWORD testFlags, RelocableCodeBuffer &px86, DWORD &pFlags, DWORD &instrCounter) {
	const BYTE trackFlagInstr[] = { 0x0B, 0x3D, 0x00, 0x00, 0x00, 0x00 };

	for (BYTE c = 0, m = 1; m < RIVER_SPEC_FLAG_EXT; ++c, m <<= 1) {
		if (m & testFlags) {
			memcpy(px86.cursor, trackFlagInstr, sizeof(trackFlagInstr));
			*(DWORD *)(&px86.cursor[0x02]) = (DWORD)&runtime->taintedFlags[c];
			px86.cursor += sizeof(trackFlagInstr);
			instrCounter++;
		}
	}
}

void TrackingX86Assembler::AssembleMarkFlag(DWORD modFlags, RelocableCodeBuffer &px86, DWORD &pFlags, DWORD &instrCounter) {
	// mov [offset], edx 0x89, 0x15, 0x00, 0x00, 0x00, 0x00
	// TODO: implement register renaming and overlapping registers
	const BYTE markFlagInstr[] = { 0x89, 0x3D, 0x00, 0x00, 0x00, 0x00 };

	for (BYTE c = 0, m = 1; m < RIVER_SPEC_FLAG_EXT; ++c, m <<= 1) {
		if (m & modFlags) {
			memcpy(px86.cursor, markFlagInstr, sizeof(markFlagInstr));
			*(DWORD *)(&px86.cursor[0x02]) = (DWORD)&runtime->taintedFlags[c];
			px86.cursor += sizeof(markFlagInstr);
			instrCounter++;
		}
	}
}

void TrackingX86Assembler::AssembleTrackRegister(const RiverRegister &reg, RelocableCodeBuffer &px86, DWORD &pFlags, DWORD &instrCounter) {
	// or edx, [offset] 0x0B, 0x15, 0x00, 0x00, 0x00, 0x00
	// TODO: implement register renaming, and overlapping registers
	const BYTE trackRegInstr[] = { 0x0B, 0x3D, 0x00, 0x00, 0x00, 0x00 };

	memcpy(px86.cursor, trackRegInstr, sizeof(trackRegInstr));
	*(DWORD *)(&px86.cursor[0x02]) = (DWORD)&runtime->taintedRegisters[GetFundamentalRegister(reg.name)];
	px86.cursor += sizeof(trackRegInstr);
	instrCounter++;
}

void TrackingX86Assembler::AssembleMarkRegister(const RiverRegister &reg, RelocableCodeBuffer &px86, DWORD &pFlags, DWORD &instrCounter) {
	// mov [offset], edx 0x89, 0x15, 0x00, 0x00, 0x00, 0x00
	// TODO: implement register renaming and overlapping registers
	const BYTE markRegInstr[] = { 0x89, 0x3D, 0x00, 0x00, 0x00, 0x00 };

	memcpy(px86.cursor, markRegInstr, sizeof(markRegInstr));
	*(DWORD *)(&px86.cursor[0x02]) = (DWORD)&runtime->taintedRegisters[GetFundamentalRegister(reg.name)];
	px86.cursor += sizeof(markRegInstr);
	instrCounter++;
}

void TrackingX86Assembler::AssembleTrackMemory(const RiverAddress *addr, RelocableCodeBuffer &px86, DWORD &pFlags, DWORD &instrCounter) {
	const BYTE trackMemInstr[] = {
		0xFF, 0x36,										// 0x00 - push [esi] - effective address
		0xFF, 0x35, 0x00, 0x00, 0x00, 0x00,				// 0x02 - push [address_hash]
		0xFF, 0x15, 0x00, 0x00, 0x00, 0x00,				// 0x08 - call [dwAddressTrackHandler]
		0x09, 0xC7										// 0x0E - or edi, eax
	};

	memcpy(px86.cursor, trackMemInstr, sizeof(trackMemInstr));
	*(DWORD *)(&px86.cursor[0x04]) = (DWORD)&runtime->taintedAddresses;
	*(DWORD *)(&px86.cursor[0x0A]) = (DWORD)&dwAddressTrackHandler;
	px86.cursor += sizeof(trackMemInstr);
	instrCounter += 4;
}

void TrackingX86Assembler::AssembleMarkMemory(const RiverAddress *addr, RelocableCodeBuffer &px86, DWORD &pFlags, DWORD &instrCounter) {
	const BYTE markMemInstr[] = { 
		0x57,											// 0x00 - push edi
		0xFF, 0x36,										// 0x01 - push [esi] - effective address
		0xFF, 0x35, 0x00, 0x00, 0x00, 0x00,				// 0x03 - push [address_hash]
		0xFF, 0x15, 0x00, 0x00, 0x00, 0x00				// 0x09 - call [dwAddressMarkHandler]
	};

	memcpy(px86.cursor, markMemInstr, sizeof(markMemInstr));
	*(DWORD *)(&px86.cursor[0x05]) = (DWORD)&runtime->taintedAddresses;
	*(DWORD *)(&px86.cursor[0x0B]) = (DWORD)&dwAddressMarkHandler;
	px86.cursor += sizeof(markMemInstr);
	instrCounter += 4;
}

void TrackingX86Assembler::AssembleAdjustESI(BYTE count, RelocableCodeBuffer &px86, DWORD &pFlags, DWORD &instrCounter) {
	const BYTE adjustESI[] = {
		0x8D, 0x76, 0x00
	};

	memcpy(px86.cursor, adjustESI, sizeof(adjustESI));
	*(BYTE *)(&px86.cursor[0x02]) = (BYTE) (~(count << 2) + 1);
	px86.cursor += sizeof(adjustESI);
	instrCounter += 4;
}

void TrackingX86Assembler::AssembleSetZero(BYTE reg, RelocableCodeBuffer &px86, DWORD &pFlags, DWORD &instrCounter) {
	reg &= 0x07;
	px86.cursor[0] = 0x31;
	px86.cursor[1] = 0xC0 | (reg << 3) | reg;
	px86.cursor += 2;
	instrCounter += 1;
}

bool TrackingX86Assembler::Translate(const RiverInstruction &ri, RelocableCodeBuffer &px86, DWORD &pFlags, BYTE &currentFamily, BYTE &repReg, DWORD &instrCounter) {
	if (0 == (RIVER_SPEC_FLAG_EXT & ri.modifiers)) {
		switch (ri.opCode) {
			case 0x50 :
				AssembleTrackRegister(ri.operands[0].asRegister, px86, pFlags, instrCounter);
				break;

			case 0x58 :
				AssembleMarkRegister(ri.operands[0].asRegister, px86, pFlags, instrCounter);
				break;

			case 0x8F:
				AssembleMarkMemory(ri.operands[0].asAddress, px86, pFlags, instrCounter);
				break;

			case 0x9C:
				AssembleTrackFlag(ri.operands[0].asImm8, px86, pFlags, instrCounter);
				break;

			case 0x9D:
				AssembleMarkFlag(ri.operands[0].asImm8, px86, pFlags, instrCounter);
				break;

			case 0xB8: // used as tracking initialization (mov eax, 0)
				if (0 == ri.operands[1].asImm32) {
					AssembleSetZero(ri.operands[0].asRegister.name, px86, pFlags, instrCounter);
				} else {
					::AssemblePlusRegInstr(ri, px86, pFlags, instrCounter);
					::AssembleImmOp<1, RIVER_OPSIZE_32>(ri, px86);
				}
				break;

			case 0xC3:
				AssembleAdjustESI(ri.operands[0].asImm8, px86, pFlags, instrCounter);
				break;

			case 0xFF:
				if (6 == ri.subOpCode) {
					AssembleTrackMemory(ri.operands[0].asAddress, px86, pFlags, instrCounter);
				} else {
					__asm int 3;
				}
				break;

			default :
				__asm int 3;
		}

		return true;
	} else {
		return true;
	}
}
