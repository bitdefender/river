#include "RiverRepAssembler.h"

const unsigned jmpSize = 5;
const unsigned loopSize = 2;

void AssembleDebugBreak(RelocableCodeBuffer &px86, nodep::DWORD &instrCounter) {
	static const nodep::BYTE intCode = 0xcc;

	*px86.cursor = intCode;
	px86.cursor += 1;
	instrCounter += 1;
}

void AssembleRepinitInstruction(const RiverInstruction &ri, RelocableCodeBuffer &px86, nodep::DWORD &instrCounter) {
	static const nodep::BYTE repinitCode[] = {
		0xE9, 0x00, 0x00, 0x00, 0x00,        //jmp wrapin
		0xE9, 0x00, 0x00, 0x00, 0x00,        //jmp codein
		0x00, 0x00,                          //loop init
		0xE9, 0x00, 0x00, 0x00, 0x00		 //jmp codeout
	};

	rev_memcpy(px86.cursor, repinitCode, sizeof(repinitCode));

	unsigned wrapinOffset = jmpSize + loopSize + jmpSize +
		jmpSize + jmpSize + jmpSize; // actual code size is fixed RelocableCodeBuffer::MarkRepFini
	*(unsigned int *)(px86.cursor + 1) = wrapinOffset;

	unsigned codeinOffset = loopSize + jmpSize;
	*(unsigned int *)(px86.cursor + 6) = codeinOffset;

	nodep::BYTE loopOpcode = 0x0;
	if (ri.modifiers & RIVER_MODIFIER_REP) {
		loopOpcode = 0xE2;
	} else if (ri.modifiers & RIVER_MODIFIER_REPZ) {
		loopOpcode = 0xE1;
	} else if (ri.modifiers & RIVER_MODIFIER_REPNZ) {
		loopOpcode = 0xE0;
	} else {
		DEBUG_BREAK;
	}
	*(px86.cursor + 10) = loopOpcode;
	*(px86.cursor + 11) = (nodep::BYTE)(-1 * (loopSize + jmpSize));

	*(unsigned int *)(px86.cursor + 13) = jmpSize; //actual code size is fixed in RelocableCodeBuffer::MarkRepFini

	px86.cursor += sizeof(repinitCode);
	instrCounter += 4;
}

void AssembleFarloopInstruction(RelocableCodeBuffer &px86, nodep::DWORD &instrCounter) {
	static const nodep::BYTE farloopCode[] = {
		0xE9, 0x00, 0x00, 0x00, 0x00,	//jmp init
		0x8D, 0x49, 0x01,				//lea ecx, [ecx + 1]
		0xE2, 0x00						//loop farloop
	};

	rev_memcpy(px86.cursor, farloopCode, sizeof(farloopCode));

	unsigned initOffset = -1 * (jmpSize + jmpSize + jmpSize + jmpSize +
			loopSize + jmpSize); //actual code size is fixed by RelocableCodeBuffer::MarkRepFini
	*(nodep::DWORD *)(px86.cursor + 1) = initOffset;

	nodep::BYTE loopOffset = (nodep::BYTE)(-1 * (loopSize + 3 + jmpSize));
	*(px86.cursor + 9) = loopOffset;

	px86.cursor += sizeof(farloopCode);
	instrCounter += 3;
}

void AssembleRepfiniInstruction(RelocableCodeBuffer &px86, nodep::DWORD &instrCounter) {
	static const nodep::BYTE repfiniCode[] = {
		0xE9, 0x00, 0x00, 0x00, 0x00, //jmp loop
		0xE9, 0x00, 0x00, 0x00, 0x00  //jmp wrapout
	};

	rev_memcpy(px86.cursor, repfiniCode, sizeof(repfiniCode));

	unsigned jmpLoopOffset = -1 * (jmpSize + jmpSize + loopSize); //actual code size is fixed by RelocableCodeBuffer::MarkRepFini
	*(unsigned int *)(px86.cursor + 1) = jmpLoopOffset;

	unsigned jmpWrapoutOffset = jmpSize + 3 + loopSize;
	*(unsigned int *)(px86.cursor + 6) = jmpWrapoutOffset;

	px86.cursor += sizeof(repfiniCode);
	instrCounter+= 2;

	AssembleFarloopInstruction(px86, instrCounter);
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
			AssembleRepinitInstruction(ri, px86, instrCounter);
			break;
		case 0xF3:
			AssembleRepfiniInstruction(px86, instrCounter);
			px86.MarkRepFini();
			break;
		case 0xcc:
			AssembleDebugBreak(px86, instrCounter);
			break;
		default:
			DEBUG_BREAK;
	}
}
