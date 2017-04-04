#include "TrackingX86Assembler.h"

#include "X86AssemblerFuncs.h"
#include "mm.h"

using namespace rev;

extern nodep::DWORD dwAddressTrackHandler;
extern nodep::DWORD dwAddressMarkHandler;
extern nodep::DWORD dwSymbolicHandler;

void TrackingX86Assembler::AssembleTrackFlag(nodep::DWORD testFlags, RelocableCodeBuffer &px86, nodep::DWORD &pFlags, nodep::DWORD &instrCounter) {
	const nodep::BYTE trackFlagInstr[] = { 0x0B, 0x3D, 0x00, 0x00, 0x00, 0x00 };

	for (nodep::BYTE c = 0, m = 1; m < RIVER_SPEC_FLAG_EXT; ++c, m <<= 1) {
		if (m & testFlags) {
			rev_memcpy(px86.cursor, trackFlagInstr, sizeof(trackFlagInstr));
			*(nodep::DWORD *)(&px86.cursor[0x02]) = (nodep::DWORD)&runtime->taintedFlags[c];
			px86.cursor += sizeof(trackFlagInstr);
			instrCounter++;
		}
	}
}

void TrackingX86Assembler::AssembleMarkFlag(nodep::DWORD modFlags, RelocableCodeBuffer &px86, nodep::DWORD &pFlags, nodep::DWORD &instrCounter) {
	// mov [offset], edx 0x89, 0x15, 0x00, 0x00, 0x00, 0x00
	// TODO: implement register renaming and overlapping registers
	const nodep::BYTE markFlagInstr[] = { 0x89, 0x3D, 0x00, 0x00, 0x00, 0x00 };

	for (nodep::BYTE c = 0, m = 1; m < RIVER_SPEC_FLAG_EXT; ++c, m <<= 1) {
		if (m & modFlags) {
			rev_memcpy(px86.cursor, markFlagInstr, sizeof(markFlagInstr));
			*(nodep::DWORD *)(&px86.cursor[0x02]) = (nodep::DWORD)&runtime->taintedFlags[c];
			px86.cursor += sizeof(markFlagInstr);
			instrCounter++;
		}
	}
}

void TrackingX86Assembler::AssembleTrackRegister(const RiverRegister &reg, RelocableCodeBuffer &px86, nodep::DWORD &pFlags, nodep::DWORD &instrCounter) {
	// or edi, [offset] 0x0B, 0x15, 0x00, 0x00, 0x00, 0x00
	// TODO: implement register renaming, and overlapping registers
	const nodep::BYTE trackRegInstr[] = { 0x0B, 0x3D, 0x00, 0x00, 0x00, 0x00 };

	rev_memcpy(px86.cursor, trackRegInstr, sizeof(trackRegInstr));
	*(nodep::DWORD *)(&px86.cursor[0x02]) = (nodep::DWORD)&runtime->taintedRegisters[GetFundamentalRegister(reg.name)];
	px86.cursor += sizeof(trackRegInstr);
	instrCounter++;
}

void TrackingX86Assembler::AssembleMarkRegister(const RiverRegister &reg, RelocableCodeBuffer &px86, nodep::DWORD &pFlags, nodep::DWORD &instrCounter) {
	// mov [offset], edx 0x89, 0x15, 0x00, 0x00, 0x00, 0x00
	// TODO: implement register renaming and overlapping registers
	const nodep::BYTE markRegInstr[] = { 0x89, 0x3D, 0x00, 0x00, 0x00, 0x00 };

	rev_memcpy(px86.cursor, markRegInstr, sizeof(markRegInstr));
	*(nodep::DWORD *)(&px86.cursor[0x02]) = (nodep::DWORD)&runtime->taintedRegisters[GetFundamentalRegister(reg.name)];
	px86.cursor += sizeof(markRegInstr);
	instrCounter++;
}

void TrackingX86Assembler::AssembleTrackMemory(const RiverAddress *addr, nodep::BYTE offset, RelocableCodeBuffer &px86, nodep::DWORD &pFlags, nodep::DWORD &instrCounter) {
	const nodep::BYTE trackMemInstr[] = {
		0xFF, 0x76, 0x00,								// 0x00 - push [esi + 0x00] - effective address
		0xFF, 0x35, 0x00, 0x00, 0x00, 0x00,				// 0x03 - push [address_hash]
		0xFF, 0x15, 0x00, 0x00, 0x00, 0x00,				// 0x09 - call [dwAddressTrackHandler]
		0x09, 0xC7										// 0x0F - or edi, eax
	};

	const nodep::BYTE trackSegInstr[] = {
		0xFF, 0x76, 0x00								// 0x00 - push [esi + 0x00] - segment index
	};

	const nodep::BYTE trackImmInstr[] = {
		0x6A, 0x00										// 0x00 - push 0 - no segment
	};

	if (addr->HasSegment()) {
		rev_memcpy(px86.cursor, trackSegInstr, sizeof(trackSegInstr));
		px86.cursor[0x02] = ~((offset - 1) << 2) + 1;
		px86.cursor += sizeof(trackSegInstr);
	} else {
		rev_memcpy(px86.cursor, trackImmInstr, sizeof(trackImmInstr));
		px86.cursor += sizeof(trackImmInstr);
	}

	rev_memcpy(px86.cursor, trackMemInstr, sizeof(trackMemInstr));
	px86.cursor[0x02] = ~(offset << 2) + 1;
	*(nodep::DWORD *)(&px86.cursor[0x05]) = (nodep::DWORD)&runtime->taintedAddresses;
	*(nodep::DWORD *)(&px86.cursor[0x0B]) = (nodep::DWORD)&dwAddressTrackHandler;
	px86.cursor += sizeof(trackMemInstr);
	instrCounter += 5;
}

void TrackingX86Assembler::AssembleTrackAddress(const RiverAddress *addr, RelocableCodeBuffer &px86, nodep::DWORD &pFlags, nodep::DWORD &instrCounter) {
	if ((0 == addr->type) || (addr->type & RIVER_ADDR_BASE)) {
		AssembleTrackRegister(addr->base, px86, pFlags, instrCounter);
	}

	if (addr->type & RIVER_ADDR_INDEX) {
		AssembleTrackRegister(addr->index, px86, pFlags, instrCounter);
	}
}

void TrackingX86Assembler::AssembleMarkMemory(const RiverAddress *addr, nodep::BYTE offset, RelocableCodeBuffer &px86, nodep::DWORD &pFlags, nodep::DWORD &instrCounter) {
	const nodep::BYTE markMemInstr[] = { 
		0x57,											// 0x00 - push edi
		0xFF, 0x76,	0x00,								// 0x01 - push [esi - offset] - effective address
		0xFF, 0x35, 0x00, 0x00, 0x00, 0x00,				// 0x04 - push [address_hash]
		0xFF, 0x15, 0x00, 0x00, 0x00, 0x00				// 0x0A - call [dwAddressMarkHandler]
	};

	const nodep::BYTE markSegInstr[] = {
		0xFF, 0x76, 0x00								// 0x00 - push [esi + 0x00] - segment index
	};

	const nodep::BYTE markImmInstr[] = {
		0x6A, 0x00										// 0x00 - push 0 - no segment
	};

	if (addr->HasSegment()) {
		rev_memcpy(px86.cursor, markSegInstr, sizeof(markSegInstr));
		px86.cursor[0x02] = ~((offset - 1) << 2) + 1;
		px86.cursor += sizeof(markSegInstr);
	}
	else {
		rev_memcpy(px86.cursor, markImmInstr, sizeof(markImmInstr));
		px86.cursor += sizeof(markImmInstr);
	}

	rev_memcpy(px86.cursor, markMemInstr, sizeof(markMemInstr));
	*(nodep::BYTE *)(&px86.cursor[0x03]) = (nodep::BYTE)(~(offset << 2) + 1);
	*(nodep::DWORD *)(&px86.cursor[0x06]) = (nodep::DWORD)&runtime->taintedAddresses;
	*(nodep::DWORD *)(&px86.cursor[0x0C]) = (nodep::DWORD)&dwAddressMarkHandler;
	px86.cursor += sizeof(markMemInstr);
	instrCounter += 4;
}

void TrackingX86Assembler::AssembleAdjustESI(nodep::BYTE count, RelocableCodeBuffer &px86, nodep::DWORD &pFlags, nodep::DWORD &instrCounter) {
	const nodep::BYTE adjustESI[] = {
		0x8D, 0x76, 0x00
	};

	rev_memcpy(px86.cursor, adjustESI, sizeof(adjustESI));
	*(nodep::BYTE *)(&px86.cursor[0x02]) = (nodep::BYTE) (~(count << 2) + 1);
	px86.cursor += sizeof(adjustESI);
	instrCounter += 4;
}

void TrackingX86Assembler::AssembleSetZero(nodep::BYTE reg, RelocableCodeBuffer &px86, nodep::DWORD &pFlags, nodep::DWORD &instrCounter) {
	reg &= 0x07;
	px86.cursor[0] = 0x31;
	px86.cursor[1] = 0xC0 | (reg << 3) | reg;
	px86.cursor += 2;
	instrCounter += 1;
}

nodep::DWORD TrackingX86Assembler::GetOperandTrackSize(const RiverInstruction &ri, nodep::BYTE idx) {
	switch (RIVER_OPTYPE(ri.opTypes[idx])) {
		case RIVER_OPTYPE_IMM :
			return 0;
		case RIVER_OPTYPE_REG :
			return 1;
		case RIVER_OPTYPE_MEM :
			if (ri.specifiers & RIVER_SPEC_IGNORES_MEMORY) {
				return 1;
			}
			return 2;
		default :
			DEBUG_BREAK;
			return 0;
	}
}

void TrackingX86Assembler::AssembleUnmark(RelocableCodeBuffer &px86, nodep::DWORD &pFlags, nodep::DWORD &instrCounter) {
	const nodep::BYTE unmarkMem[] = {
		0x56,											// 0x00 - push esi
		0x50,											// 0x01 - push eax
		0x57,											// 0x02 - push edi
		0xFF, 0x35, 0x00, 0x00, 0x00, 0x00,				// 0x03 - push [address_hash]
		0xFF, 0x15, 0x00, 0x00, 0x00, 0x00				// 0x09 - call [dwAddressTrackHandler]
	};

	rev_memcpy(px86.cursor, unmarkMem, sizeof(unmarkMem));
	*(nodep::DWORD *)(&px86.cursor[0x05]) = (nodep::DWORD)&runtime->taintedAddresses;
	*(nodep::DWORD *)(&px86.cursor[0x0B]) = (nodep::DWORD)&dwAddressMarkHandler;
	px86.cursor += sizeof(unmarkMem);
	instrCounter += 3;
}

void TrackingX86Assembler::AssembleSymbolicCall(nodep::DWORD address, nodep::BYTE index, RelocableCodeBuffer &px86, nodep::DWORD &pFlags, nodep::DWORD &instrCounter) {
	const nodep::BYTE symCall[] = {
		0x74, 0x11,										// 0x00 - jz <over_this_block>
		0x68, 0x00, 0x00, 0x00, 0x00,					// 0x02 - push instructionAddress
		0x56,											// 0x07 - push esi - data offset
		0x68, 0x00, 0x00, 0x00, 0x00,					// 0x08	- push runtimeContext (or env)
		0xFF, 0x15, 0x00, 0x00, 0x00, 0x00				// 0x0D - call [dwSymbolicHandler]
	};

	rev_memcpy(px86.cursor, symCall, sizeof(symCall));
	*(nodep::DWORD *)(&px86.cursor[0x03]) = address;
	*(nodep::DWORD *)(&px86.cursor[0x09]) = (nodep::DWORD)runtime;
	*(nodep::DWORD *)(&px86.cursor[0x0F]) = (nodep::DWORD)&revtracerImports.symbolicHandler;
	px86.cursor += sizeof(symCall);
	instrCounter += 5;
}

void TrackingX86Assembler::SetTranslationFlags(nodep::DWORD dwFlags) {
	dwTranslationFlags = dwFlags;
}

bool TrackingX86Assembler::Translate(const RiverInstruction &ri, RelocableCodeBuffer &px86, nodep::DWORD &pFlags, nodep::BYTE &currentFamily, nodep::BYTE &repReg, nodep::DWORD &instrCounter, nodep::BYTE outputType) {
	if (0 == (RIVER_SPEC_FLAG_EXT & ri.modifiers)) {
		switch (ri.opCode) {
			case 0x50 :
				AssembleTrackRegister(ri.operands[0].asRegister, px86, pFlags, instrCounter);
				break;

			case 0x58 :
				if (0 == (TRACER_FEATURE_ADVANCED_TRACKING & dwTranslationFlags)) {
					AssembleMarkRegister(ri.operands[0].asRegister, px86, pFlags, instrCounter);
				}
				break;

			case 0x8D :
				AssembleTrackAddress(ri.operands[0].asAddress, px86, pFlags, instrCounter);
				break;

			case 0x8F :
				if (ASSEMBLER_DIR_BACKWARD & outputType) {
					AssembleUnmark(px86, pFlags, instrCounter);
				}
				else {
					if (0 == (TRACER_FEATURE_ADVANCED_TRACKING & dwTranslationFlags)) {
						AssembleMarkMemory(ri.operands[0].asAddress, ri.operands[1].asImm8, px86, pFlags, instrCounter);
					}
				}
				break;

			case 0x9C :
				AssembleTrackFlag(ri.operands[0].asImm8, px86, pFlags, instrCounter);
				break;

			case 0x9D :
				if (0 == (TRACER_FEATURE_ADVANCED_TRACKING & dwTranslationFlags)) {
					AssembleMarkFlag(ri.operands[0].asImm8, px86, pFlags, instrCounter);
				}
				break;

			case 0xB8 : // used as tracking initialization (mov eax, 0)
				if (0 == ri.operands[1].asImm32) {
					AssembleSetZero(ri.operands[0].asRegister.name, px86, pFlags, instrCounter);
				} else {
					::AssemblePlusRegInstr(ri, px86, pFlags, instrCounter);
					::AssembleImmOp<1, RIVER_OPSIZE_32>(ri, px86);
				}
				break;

			case 0xC3 :
				AssembleAdjustESI(ri.operands[0].asImm8, px86, pFlags, instrCounter);
				break;

			case 0xFF :
				if (6 == ri.subOpCode) {
					AssembleTrackMemory(ri.operands[0].asAddress, ri.operands[1].asImm8, px86, pFlags, instrCounter);
				} else {
					DEBUG_BREAK;
				}
				break;

			case 0xE8 :
				AssembleSymbolicCall(ri.operands[0].asImm32, ri.operands[1].asImm8, px86, pFlags, instrCounter);
				break;
			default :
				DEBUG_BREAK;
		}

		return true;
	} else {
		return true;
	}
}
