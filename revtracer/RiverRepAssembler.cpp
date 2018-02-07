#include "RiverRepAssembler.h"

void AssembleJmpInstruction(RelocableCodeBuffer &px86, nodep::DWORD &instrCounter) {
	static const nodep::BYTE jmpCode[] = {
		0xE9, 0x00, 0x00, 0x00, 0x00
	};
	rev_memcpy(px86.cursor, jmpCode, sizeof(jmpCode));
	px86.cursor += sizeof(jmpCode);
	instrCounter += 1;
}

void AssembleKnownJmpInstruction(const RiverInstruction &ri, RelocableCodeBuffer &px86, nodep::DWORD &instrCounter) {
	static const nodep::BYTE jmpCode[] = {
		0xE9, 0x00, 0x00, 0x00, 0x00
	};
	int addrJump = 2/*loop init*/ + 5 /*jmp repfini*/;

	rev_memcpy(px86.cursor, jmpCode, sizeof(jmpCode));

	*(unsigned int *)(&(px86.cursor[0x1])) = addrJump;

	px86.cursor += sizeof(jmpCode);
	instrCounter += 1;
}

void AssembleLoopInstruction(const RiverInstruction &ri, RelocableCodeBuffer &px86, nodep::DWORD &instrCounter) {
	static const nodep::BYTE loopCode[] = {
		0x00, 0x00
	};

	nodep::BYTE addrLoop = -1 * (2 /*loop init*/ + 5 /*jmp code*/);

	nodep::BYTE opcode = 0x0;
	if (ri.modifiers & RIVER_MODIFIER_REP) {
		opcode = 0xE2;
	} else if (ri.modifiers & RIVER_MODIFIER_REPZ) {
		opcode = 0xE1;
	} else if (ri.modifiers & RIVER_MODIFIER_REPNZ) {
		opcode = 0xE0;
	} else {
		DEBUG_BREAK;
	}

	rev_memcpy(px86.cursor, loopCode, sizeof(loopCode));

	*(nodep::BYTE *)(px86.cursor) = opcode;
	*(nodep::BYTE *)(&px86.cursor[1]) = addrLoop;

	px86.cursor += sizeof(loopCode);
	instrCounter += 1;
}

void AssembleDebugBreak(const RiverInstruction &ri, RelocableCodeBuffer &px86, nodep::DWORD &instrCounter) {
	static const nodep::BYTE intCode = 0xcc;

	*px86.cursor = intCode;
	px86.cursor += 1;
	instrCounter += 1;
}

/* rep prefixed instructions are translated as described in RiverRepTranslator
 * instructions repinit and repfini are used to compute the actual code size.
 * The jump offsets are fixed afterwards
 */
bool RiverRepAssembler::Translate(const RiverInstruction &ri, RelocableCodeBuffer &px86, nodep::DWORD &pFlags, nodep::BYTE &currentFamily, nodep::BYTE &repReg, nodep::DWORD &instrCounter, nodep::BYTE outputType) {
	// handle repinit and repfini
	switch(ri.opCode) {
		case 0xF2:
			px86.MarkRepInit();
			AssembleKnownJmpInstruction(ri, px86, instrCounter);
			AssembleLoopInstruction(ri, px86, instrCounter);
			AssembleJmpInstruction(px86, instrCounter);
			break;
		case 0xF3:
			AssembleJmpInstruction(px86, instrCounter);
			px86.MarkRepFini();
			break;
		case 0xcc:
			AssembleDebugBreak(ri, px86, instrCounter);
			break;
		default:
			DEBUG_BREAK;
	}
}
