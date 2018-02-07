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
	int addrJump = (int)(ri.operands[0].asImm32);

	rev_memcpy(px86.cursor, jmpCode, sizeof(jmpCode));

	*(unsigned int *)(&(px86.cursor[0x1])) = addrJump;

	px86.cursor += sizeof(jmpCode);
	instrCounter += 1;
}

void AssembleLoopInstruction(const RiverInstruction &ri, RelocableCodeBuffer &px86, nodep::DWORD &instrCounter) {
	static const nodep::BYTE loopCode[] = {
		0x00, 0x00
	};

	nodep::BYTE addrLoop = ri.operands[0].asImm8;

	nodep::BYTE opcode = ri.opCode;
	rev_memcpy(px86.cursor, loopCode, sizeof(loopCode));

	*(nodep::BYTE *)(px86.cursor) = opcode;
	*(nodep::BYTE *)(&px86.cursor[1]) = addrLoop;

	px86.cursor += sizeof(loopCode);
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
			AssembleJmpInstruction(px86, instrCounter);
			break;
		case 0xF3:
			AssembleJmpInstruction(px86, instrCounter);
			px86.MarkRepFini();
			break;
		case 0xE9:
			AssembleKnownJmpInstruction(ri, px86, instrCounter);
			break;
		case 0xE0: case 0xE1: case 0xE2:
			AssembleLoopInstruction(ri, px86, instrCounter);
			break;
		default:
			DEBUG_BREAK;
	}
}
