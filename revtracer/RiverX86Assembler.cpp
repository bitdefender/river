#include <x86intrin.h>

#include "RiverX86Assembler.h"
#include "X86AssemblerFuncs.h"

#include "mm.h"

using namespace rev;

extern const nodep::BYTE specAssemblerTbl[2][0x100];

#define X86_OPZISE_PREFIX			0x66

#define X86_LOCK_PREFIX				0xF0
#define X86_REPNZ_PREFIX			0xF2
#define X86_REPZ_PREFIX				0xF3
#define X86_REP_PREFIX				0xF3

#define X86_ESSEG_PREFIX			0x26
#define X86_CSSEG_PREFIX			0x2E
#define X86_SSSEG_PREFIX			0x36
#define X86_DSSEG_PREFIX			0x3E
#define X86_FSSEG_PREFIX			0x64
#define X86_GSSEG_PREFIX			0x65

#define FLAG_GENERATE_RIVER_xSP		0x0F
/*#define FLAG_GENERATE_RIVER_xSP_xAX	0x01
#define FLAG_GENERATE_RIVER_xSP_xCX	0x02
#define FLAG_GENERATE_RIVER_xSP_xDX	0x04
#define FLAG_GENERATE_RIVER_xSP_xBX	0x08*/

#define FLAG_SKIP_METAOP			0x10
#define FLAG_GENERATE_RIVER			0x20


extern nodep::DWORD dwSysHandler; // = 0; // &SysHandler
extern nodep::DWORD dwSysEndHandler; // = 0; // &SysEndHandler
extern nodep::DWORD dwBranchHandler; // = 0; // &BranchHandler

void RiverX86Assembler::SwitchToRiver(nodep::BYTE *&px86, nodep::DWORD &instrCounter) {
	static const unsigned char code[] = { 0x87, 0x25, 0x00, 0x00, 0x00, 0x00 };			// 0x00 - xchg esp, large ds:<dwVirtualStack>}

	rev_memcpy(px86, code, sizeof(code));
	*(unsigned int *)(&(px86[0x02])) = (unsigned int)&runtime->execBuff;

	px86 += sizeof(code);
	instrCounter++;
}

void RiverX86Assembler::SwitchToRiverEsp(nodep::BYTE *&px86, nodep::DWORD &instrCounter, nodep::BYTE repReg) {
	static const unsigned char code[] = { 0x87, 0x05, 0x00, 0x00, 0x00, 0x00 };			// 0x00 - xchg eax, large ds:<dwVirtualStack>}

	rev_memcpy(px86, code, sizeof(code));
	px86[0x01] |= (repReg & 0x03); // choose the acording replacement register
	*(unsigned int *)(&(px86[0x02])) = (unsigned int)&runtime->execBuff;

	px86 += sizeof(code);
	instrCounter++;
}

bool RiverX86Assembler::Translate(const RiverInstruction &ri, RelocableCodeBuffer &px86, nodep::DWORD &pFlags, nodep::BYTE &currentFamily, nodep::BYTE &repReg, nodep::DWORD &instrCounter, nodep::BYTE outputType) {
	nodep::DWORD dwTable = 0;

	if (ri.modifiers & RIVER_MODIFIER_EXT) {
		*px86.cursor = 0x0F;
		px86.cursor++;
		dwTable = 1;
	}

	/*
	(this->*assembleOpcodes[dwTable][ri.opCode])(ri, px86, pFlags, instrCounter);
	(this->*assembleOperands[dwTable][ri.opCode])(ri, px86);
	*/

	if (1 == dwTable) {
		DEBUG_BREAK;
	} else {
		switch (ri.opCode) {
			case 0x50:
			case 0x58:
				AssemblePlusRegInstr(ri, px86, pFlags, instrCounter);
				AssembleNoOp(ri, px86);
				break;

			case 0x84:
				AssembleRiverAddSubInstr(ri, px86, pFlags, instrCounter);
				AssembleRiverAddSubOp(ri, px86);
				break;

			case 0x8F:
				::AssembleDefaultInstr(ri, px86, pFlags, instrCounter);
				::AssembleModRMOp(0, ri, px86, 0);
				break;

			case 0x9C:
			case 0x9D:
				::AssembleDefaultInstr(ri, px86, pFlags, instrCounter);
				::AssembleNoOp(ri, px86);
				break;

			case 0xFF:
				if (6 == ri.subOpCode) {
					AssembleDefaultInstr(ri, px86, pFlags, instrCounter);
					AssembleModRMOp(0, ri, px86, 6);
				} else {
					DEBUG_BREAK;
				}
				break;
			default :
				DEBUG_BREAK;
		}
	}
	return true;
}


void RiverX86Assembler::AssembleRiverAddSubInstr(const RiverInstruction &ri, RelocableCodeBuffer &px86, nodep::DWORD &pFlags, nodep::DWORD &instrCounter) {
	*px86.cursor = 0x8d; // add and sub are converted to lea
	px86.cursor++;
	instrCounter++;
}

void RiverX86Assembler::AssembleRiverAddSubOp(const RiverInstruction &ri, RelocableCodeBuffer &px86) {
	RiverInstruction rTmp;
	RiverAddress32 addr;

	addr.type = RIVER_ADDR_DIRTY | RIVER_ADDR_BASE | RIVER_ADDR_DISP8;
	addr.base.versioned = ri.operands[0].asRegister.versioned;

	addr.disp.d8 = ri.operands[1].asImm8;

	if (ri.subOpCode == 5) {
		addr.disp.d8 = ~addr.disp.d8 + 1;
	}

	addr.modRM |= ri.operands[0].asRegister.name << 3;

	rTmp.opTypes[0] = ri.opTypes[0];
	rTmp.operands[0].asRegister.versioned = ri.operands[0].asRegister.versioned;

	rTmp.opTypes[1] = RIVER_OPTYPE_MEM; // no size specified;
	rTmp.operands[1].asAddress = &addr;


	AssembleRegModRMOp(rTmp, px86);
}

void RiverX86Assembler::AssembleRegModRMOp(const RiverInstruction &ri, RelocableCodeBuffer &px86) {
	AssembleModRMOp(1, ri, px86, ri.operands[0].asRegister.name);
}

void RiverX86Assembler::AssembleModRMOp(unsigned int opIdx, const RiverInstruction &ri, RelocableCodeBuffer &px86, nodep::BYTE extra) {
	ri.operands[opIdx].asAddress->EncodeTox86(px86.cursor, extra, ri.family, ri.modifiers);
}
