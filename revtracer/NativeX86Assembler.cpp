#include "NativeX86Assembler.h"

#include "mm.h"

extern DWORD dwSysHandler; // = 0; // &SysHandler
extern DWORD dwSysEndHandler; // = 0; // &SysEndHandler
extern DWORD dwBranchHandler; // = 0; // &BranchHandler

bool NativeX86Assembler::Translate(const RiverInstruction &ri, RelocableCodeBuffer &px86, DWORD &pFlags, BYTE &currentFamily, BYTE &repReg, DWORD &instrCounter) {
	DWORD dwTable = 0;

	if (ri.modifiers & RIVER_MODIFIER_EXT) {
		*px86.cursor = 0x0F;
		px86.cursor++;
		dwTable = 1;
	}

	(this->*assembleOpcodes[dwTable][ri.opCode])(ri, px86, pFlags, instrCounter);
	(this->*assembleOperands[dwTable][ri.opCode])(ri, px86);
	return true;
}

/* =========================================== */
/* Opcode assemblers                           */
/* =========================================== */


#include "X86AssemblerFuncs.h"

void NativeX86Assembler::AssembleUnkInstr(const RiverInstruction &ri, RelocableCodeBuffer &px86, DWORD &pFlags, DWORD &instrCounter) {
	::AssembleUnkInstr(ri, px86, pFlags, instrCounter);
}

void NativeX86Assembler::AssembleDefaultInstr(const RiverInstruction &ri, RelocableCodeBuffer &px86, DWORD &pFlags, DWORD &instrCounter) {
	::AssembleDefaultInstr(ri, px86, pFlags, instrCounter);
}

void NativeX86Assembler::AssemblePlusRegInstr(const RiverInstruction &ri, RelocableCodeBuffer &px86, DWORD &pFlags, DWORD &instrCounter) {
	::AssemblePlusRegInstr(ri, px86, pFlags, instrCounter);
}

void NativeX86Assembler::AssembleRelJMPInstr(const RiverInstruction &ri, RelocableCodeBuffer &px86, DWORD &pFlags, DWORD &instrCounter) {
	static const BYTE pBranchJMP[] = {
		0x87, 0x25, 0x00, 0x00, 0x00, 0x00,			// 0x00 - xchg esp, large ds:<dwVirtualStack>
		0x9C, 										// 0x06 - pushf
		0x60,										// 0x07 - pusha
		0x68, 0x46, 0x02, 0x00, 0x00,				// 0x08 - push 0x00000246 - NEW FLAGS
		0x9D,										// 0x0D - popf
		0x68, 0x00, 0x00, 0x00, 0x00,				// 0x0E - push <jump_addr>
		0x68, 0x00, 0x00, 0x00, 0x00,				// 0x13 - push <execution_environment>
		0xFF, 0x15, 0x00, 0x00, 0x00, 0x00,			// 0x18 - call <dwBranchHandler>
		0x61,										// 0x1E - popa
		0x9D,										// 0x1F - popf
		0x87, 0x25, 0x00, 0x00, 0x00, 0x00,			// 0x20 - xchg esp, large ds:<dwVirtualStack>
		0xFF, 0x25, 0x00, 0x00, 0x00, 0x00			// 0x26 - jmp large dword ptr ds:<jumpbuff>	
	};

	int addrJump = (int)(ri.operands[1].asImm32);

	switch (RIVER_OPSIZE(ri.opTypes[0])) {
	case RIVER_OPSIZE_8:
		addrJump += (char)ri.operands[0].asImm8;
		break;
	case RIVER_OPSIZE_16:
		addrJump += (short)ri.operands[0].asImm16;
		break;
	case RIVER_OPSIZE_32:
		addrJump += (int)ri.operands[0].asImm32;
		break;
	}

	memcpy(px86.cursor, pBranchJMP, sizeof(pBranchJMP));
	*(unsigned int *)(&(px86.cursor[0x02])) = (unsigned int)&runtime->virtualStack;
	*(unsigned int *)(&(px86.cursor[0x0F])) = addrJump;
	*(unsigned int *)(&(px86.cursor[0x14])) = (unsigned int)runtime;
	*(unsigned int *)(&(px86.cursor[0x1A])) = (unsigned int)&dwBranchHandler;
	*(unsigned int *)(&(px86.cursor[0x22])) = (unsigned int)&runtime->virtualStack;
	*(unsigned int *)(&(px86.cursor[0x28])) = (unsigned int)&runtime->jumpBuff;

	px86.cursor += sizeof(pBranchJMP);
	pFlags |= RIVER_FLAG_BRANCH;
	instrCounter += 12;
}

void NativeX86Assembler::AssembleJMPInstr(const RiverInstruction &ri, RelocableCodeBuffer &px86, DWORD &pFlags, DWORD &instrCounter) {
	static const BYTE pBranchJMP[] = {
		0x87, 0x25, 0x00, 0x00, 0x00, 0x00,			// 0x00 - xchg esp, large ds:<dwVirtualStack>
		0x9C, 										// 0x06 - pushf
		0x60,										// 0x07 - pusha
		0x68, 0x46, 0x02, 0x00, 0x00,				// 0x08 - push 0x00000246 - NEW FLAGS
		0x9D,										// 0x0D - popf
		0x68, 0x00, 0x00, 0x00, 0x00,				// 0x0E - push <jump_addr>
		0x68, 0x00, 0x00, 0x00, 0x00,				// 0x13 - push <execution_environment>
		0xFF, 0x15, 0x00, 0x00, 0x00, 0x00,			// 0x18 - call <dwBranchHandler>
		0x61,										// 0x1E - popa
		0x9D,										// 0x1F - popf
		0x87, 0x25, 0x00, 0x00, 0x00, 0x00,			// 0x20 - xchg esp, large ds:<dwVirtualStack>
		0xFF, 0x25, 0x00, 0x00, 0x00, 0x00			// 0x26 - jmp large dword ptr ds:<jumpbuff>	
	};

	int addrJump = (int)(ri.operands[0].asImm32);

	memcpy(px86.cursor, pBranchJMP, sizeof(pBranchJMP));
	*(unsigned int *)(&(px86.cursor[0x02])) = (unsigned int)&runtime->virtualStack;
	*(unsigned int *)(&(px86.cursor[0x0F])) = addrJump;
	*(unsigned int *)(&(px86.cursor[0x14])) = (unsigned int)runtime;
	*(unsigned int *)(&(px86.cursor[0x1A])) = (unsigned int)&dwBranchHandler;
	*(unsigned int *)(&(px86.cursor[0x22])) = (unsigned int)&runtime->virtualStack;
	*(unsigned int *)(&(px86.cursor[0x28])) = (unsigned int)&runtime->jumpBuff;

	px86.cursor += sizeof(pBranchJMP);
	pFlags |= RIVER_FLAG_BRANCH;
	instrCounter += 12;
}

void NativeX86Assembler::AssembleLeaveForSyscall(
	const RiverInstruction &ri,
	RelocableCodeBuffer &px86,
	DWORD &pFlags,
	DWORD &instrCounter,
	NativeX86Assembler::AssembleOpcodeFunc opcodeFunc,
	NativeX86Assembler::AssembleOperandsFunc operandsFunc
) {
	static const BYTE pBranchSyscall[] = {
		0x87, 0x25, 0x00, 0x00, 0x00, 0x00,			// 0x00 - xchg esp, large ds:<dwVirtualStack>
		0x9C, 										// 0x06 - pushf
		0x60,										// 0x07 - pusha
		0x68, 0x46, 0x02, 0x00, 0x00,				// 0x08 - push 0x00000246 - NEW FLAGS
		0x9D,										// 0x0D - popf
		0x68, 0x00, 0x00, 0x00, 0x00,				// 0x0E - push <execution_environment>
		0xFF, 0x15, 0x00, 0x00, 0x00, 0x00,			// 0x13 - call <dwSysHandler>
		0x61,										// 0x19 - popa
		0x9D,										// 0x1A - popf
		0x87, 0x25, 0x00, 0x00, 0x00, 0x00			// 0x1B - xchg esp, large ds:<dwVirtualStack>
	};

	memcpy(px86.cursor, pBranchSyscall, sizeof(pBranchSyscall));
	*(unsigned int *)(&(px86.cursor[0x02])) = (unsigned int)&runtime->virtualStack;
	*(unsigned int *)(&(px86.cursor[0x0F])) = (unsigned int)runtime;
	*(unsigned int *)(&(px86.cursor[0x15])) = (unsigned int)&dwSysHandler;
	*(unsigned int *)(&(px86.cursor[0x1D])) = (unsigned int)&runtime->virtualStack;
	px86.cursor += sizeof(pBranchSyscall);
	instrCounter += 10;

	// assemble actual syscall
	GeneratePrefixes(ri, px86.cursor);

	(*this.*opcodeFunc)(ri, px86, pFlags, instrCounter);
	(*this.*operandsFunc)(ri, px86);

	// call $a
	static const BYTE pBranchSysret[] = {
		0x87, 0x25, 0x00, 0x00, 0x00, 0x00,			// 0x00 - xchg esp, large ds:<dwVirtualStack>
		0x9C, 										// 0x06 - pushf
		0x60,										// 0x07 - pusha
		0x68, 0x46, 0x02, 0x00, 0x00,				// 0x08 - push 0x00000246 - NEW FLAGS
		0x9D,										// 0x0D - popf
		0x68, 0x00, 0x00, 0x00, 0x00,				// 0x0E - push <jump_addr>
		0x68, 0x00, 0x00, 0x00, 0x00,				// 0x13 - push <execution_environment>
		0xFF, 0x15, 0x00, 0x00, 0x00, 0x00,			// 0x18 - call <dwBranchHandler>
		0x61,										// 0x1E - popa
		0x9D,										// 0x1F - popf
		0x87, 0x25, 0x00, 0x00, 0x00, 0x00,			// 0x20 - xchg esp, large ds:<dwVirtualStack>
		0xFF, 0x25, 0x00, 0x00, 0x00, 0x00			// 0x26 - jmp large dword ptr ds:<jumpbuff>	
	};

	int addrJump = (int)(ri.operands[1].asImm32);

	memcpy(px86.cursor, pBranchSysret, sizeof(pBranchSysret));
	*(unsigned int *)(&(px86.cursor[0x02])) = (unsigned int)&runtime->virtualStack;
	*(unsigned int *)(&(px86.cursor[0x0F])) = addrJump;
	*(unsigned int *)(&(px86.cursor[0x14])) = (unsigned int)runtime;
	*(unsigned int *)(&(px86.cursor[0x1A])) = (unsigned int)&dwBranchHandler;
	*(unsigned int *)(&(px86.cursor[0x22])) = (unsigned int)&runtime->virtualStack;
	*(unsigned int *)(&(px86.cursor[0x28])) = (unsigned int)&runtime->jumpBuff;

	px86.cursor += sizeof(pBranchSysret);
	pFlags |= RIVER_FLAG_BRANCH;
	instrCounter += 12;
}

void NativeX86Assembler::AssembleRelJmpCondInstr(const RiverInstruction &ri, RelocableCodeBuffer &px86, DWORD &pFlags, DWORD &instrCounter) {
	const BYTE pBranchJCC[] = {
		0x87, 0x25, 0x00, 0x00, 0x00, 0x00,			// 0x00 - xchg esp, large ds:<dwVirtualStack>
		0x9C, 										// 0x06 - pushf
		0x60,										// 0x07 - pusha
		0x68, 0x46, 0x02, 0x00, 0x00,				// 0x08 - push 0x00000246 - NEW FLAGS
		0x9D,										// 0x0D - popf
		0x68, 0x00, 0x00, 0x00, 0x00,				// 0x0E - push <original_address>
		0x68, 0x00, 0x00, 0x00, 0x00,				// 0x13 - push <execution_environment>
		0xFF, 0x15, 0x00, 0x00, 0x00, 0x00,			// 0x18 - call <branch_handler>
		0x61,										// 0x1E - popa
		0x9D,										// 0x1F - popf
		0x87, 0x25, 0x00, 0x00, 0x00, 0x00,			// 0x20 - xchg esp, large ds:<dwVirtualStack>
		0xFF, 0x25, 0x00, 0x00, 0x00, 0x00			// 0x26 - jmp large dword ptr ds:<jumpbuff>
	};

	int addrFallThrough = (int)(ri.operands[1].asImm32);
	int addrJump = addrFallThrough;

	switch (RIVER_OPSIZE(ri.opTypes[0])) {
	case RIVER_OPSIZE_8:
		addrJump += (char)ri.operands[0].asImm8;
		break;
	case RIVER_OPSIZE_16:
		addrJump += (short)ri.operands[0].asImm16;
		break;
	case RIVER_OPSIZE_32:
		addrJump += (int)ri.operands[0].asImm32;
		break;
	}

	/* copy the Jcc instruction */
	*px86.cursor = ri.opCode;
	px86.cursor++;

	switch (RIVER_OPSIZE(ri.opTypes[0])) {
	case RIVER_OPSIZE_8:
		*px86.cursor = sizeof(pBranchJCC);
		px86.cursor += 1;
		break;
	case RIVER_OPSIZE_16:
		*(WORD *)px86.cursor = sizeof(pBranchJCC);
		px86.cursor += 2;
		break;
	case RIVER_OPSIZE_32:
		*(DWORD *)px86.cursor = sizeof(pBranchJCC);
		px86.cursor += 4;
		break;
	}


	memcpy(px86.cursor, pBranchJCC, sizeof(pBranchJCC));

	*(unsigned int *)(&(px86.cursor[0x02])) = (unsigned int)&runtime->virtualStack;
	*(unsigned int *)(&(px86.cursor[0x0F])) = addrFallThrough;
	*(unsigned int *)(&(px86.cursor[0x14])) = (unsigned int)runtime;
	*(unsigned int *)(&(px86.cursor[0x1A])) = (unsigned int)&dwBranchHandler;
	*(unsigned int *)(&(px86.cursor[0x22])) = (unsigned int)&runtime->virtualStack;
	*(unsigned int *)(&(px86.cursor[0x28])) = (unsigned int)&runtime->jumpBuff;
	px86.cursor += sizeof(pBranchJCC);


	memcpy(px86.cursor, pBranchJCC, sizeof(pBranchJCC));

	*(unsigned int *)(&(px86.cursor[0x02])) = (unsigned int)&runtime->virtualStack;
	*(unsigned int *)(&(px86.cursor[0x0F])) = addrJump;
	*(unsigned int *)(&(px86.cursor[0x14])) = (unsigned int)runtime;
	*(unsigned int *)(&(px86.cursor[0x1A])) = (unsigned int)&dwBranchHandler;
	*(unsigned int *)(&(px86.cursor[0x22])) = (unsigned int)&runtime->virtualStack;
	*(unsigned int *)(&(px86.cursor[0x28])) = (unsigned int)&runtime->jumpBuff;
	px86.cursor += sizeof(pBranchJCC);

	pFlags |= RIVER_FLAG_BRANCH;
	instrCounter += 25;
}

void NativeX86Assembler::AssembleCallInstr(const RiverInstruction &ri, RelocableCodeBuffer &px86, DWORD &pFlags, DWORD &instrCounter) {
	const char pBranchCall[] = {
		0x68, 0x00, 0x00, 0x00, 0x00,				// 0x00 - push <retAddr>
		0x87, 0x25, 0x00, 0x00, 0x00, 0x00,			// 0x05 - xchg esp, large ds:<dwVirtualStack>
		0x9C, 										// 0x0B - pushf
		0x60,										// 0x0C - pusha
		0x68, 0x46, 0x02, 0x00, 0x00,				// 0x0D - push 0x00000246 - NEW FLAGS
		0x9D,										// 0x12 - popf
		0x68, 0x00, 0x00, 0x00, 0x00,				// 0x13 - push <jumpAddr>
		0x68, 0x00, 0x00, 0x00, 0x00,				// 0x18 - push <execution_environment>
		0xFF, 0x15, 0x00, 0x00, 0x00, 0x00,			// 0x1D - call <dwBranchHandler>

		0x61,										// 0x23 - popa
		0x9D,										// 0x24 - popf
		0x87, 0x25, 0x00, 0x00, 0x00, 0x00,			// 0x25 - xchg esp, large ds:<dwVirtualStack>
		0xFF, 0x25, 0x00, 0x00, 0x00, 0x00			// 0x2B - jmp large dword ptr ds:<jumpbuff>
	};

	int retAddr = (int)(ri.operands[1].asImm32);
	int addrJump = (int)(ri.operands[1].asImm32);

	switch (RIVER_OPSIZE(ri.opTypes[0])) {
	case RIVER_OPSIZE_8:
		addrJump += (char)ri.operands[0].asImm8;
		break;
	case RIVER_OPSIZE_16:
		addrJump += (short)ri.operands[0].asImm16;
		break;
	case RIVER_OPSIZE_32:
		addrJump += (int)ri.operands[0].asImm32;
		break;
	}

	memcpy(px86.cursor, pBranchCall, sizeof(pBranchCall));
	*(unsigned int *)(&(px86.cursor[0x01])) = retAddr;
	*(unsigned int *)(&(px86.cursor[0x07])) = (unsigned int)&runtime->virtualStack;
	*(unsigned int *)(&(px86.cursor[0x14])) = addrJump;
	*(unsigned int *)(&(px86.cursor[0x19])) = (unsigned int)runtime;
	*(unsigned int *)(&(px86.cursor[0x1F])) = (unsigned int)&dwBranchHandler;
	*(unsigned int *)(&(px86.cursor[0x27])) = (unsigned int)&runtime->virtualStack;
	*(unsigned int *)(&(px86.cursor[0x2D])) = (unsigned int)&runtime->jumpBuff;
	px86.cursor += sizeof(pBranchCall);

	pFlags |= RIVER_FLAG_BRANCH;
	instrCounter += 25;
}

void NativeX86Assembler::AssembleFFJumpInstr(const RiverInstruction &ri, RelocableCodeBuffer &px86, DWORD &pFlags, DWORD &instrCounter) {
	static const char pGetAddrCode[] = {
		0xA3, 0x00, 0x00, 0x00, 0x00,				// 0x00 - [<dwEaxSave>], eax
		0x8B										// 0x05 - mov ...
	};

	static const char pBranchFFCall[] = {
		0x87, 0x25, 0x00, 0x00, 0x00, 0x00,			// 0x00 - xchg esp, large [<dwVirtualStack>]
		0x9C, 										// 0x06 - pushf
		0x60,										// 0x07 - pusha
		0x68, 0x46, 0x02, 0x00, 0x00,				// 0x08 - push 0x00000246 - NEW FLAGS
		0x9D,										// 0x0D - popf
		0x50,                            			// 0x0E - push eax
		0xA1, 0x00, 0x00, 0x00, 0x00,				// 0x0F	- mov eax, [<dwEaxSave>]
		0x89, 0x44, 0x24, 0x20,						// 0x14 - mov [esp+0x20], eax // fix the eax value
		0x68, 0x00, 0x00, 0x00, 0x00,				// 0x18 - push <execution_environment>
		0xFF, 0x15, 0x00, 0x00, 0x00, 0x00,         // 0x1D - call [<dwBranchHandler>]
		0x61,										// 0x23 - popa
		0x9D,										// 0x24 - popf
		0x87, 0x25, 0x00, 0x00, 0x00, 0x00,			// 0x25 - xchg esp, large ds:<dwVirtualStack>
		0xFF, 0x25, 0x00, 0x00, 0x00, 0x00			// 0x2B - jmp large dword ptr ds:<jumpbuff>
	};


	memcpy(px86.cursor, pGetAddrCode, sizeof(pGetAddrCode));
	*(unsigned int *)(&(px86.cursor[1])) = (unsigned int)&runtime->returnRegister;
	px86.cursor += sizeof(pGetAddrCode);
	ri.operands[0].asAddress->EncodeTox86(px86.cursor, RIVER_REG_xAX, 0, 0); // use flags?

	memcpy(px86.cursor, pBranchFFCall, sizeof(pBranchFFCall));
	*(unsigned int *)(&(px86.cursor[0x02])) = (unsigned int)&runtime->virtualStack;
	*(unsigned int *)(&(px86.cursor[0x10])) = (unsigned int)&runtime->returnRegister;
	*(unsigned int *)(&(px86.cursor[0x19])) = (unsigned int)runtime;
	*(unsigned int *)(&(px86.cursor[0x1F])) = (unsigned int)&dwBranchHandler;
	*(unsigned int *)(&(px86.cursor[0x27])) = (unsigned int)&runtime->virtualStack;
	*(unsigned int *)(&(px86.cursor[0x2D])) = (unsigned int)&runtime->jumpBuff;
	px86.cursor += sizeof(pBranchFFCall);
	pFlags |= RIVER_FLAG_BRANCH;

	instrCounter += 16;
}

void NativeX86Assembler::AssembleSyscall2(const RiverInstruction &ri, RelocableCodeBuffer &px86, DWORD &pFlags, DWORD &instrCounter) {
	static const char pSaveEdxCode[] = {
		0xA3, 0x00, 0x00, 0x00, 0x00,					// 0x00 - mov [<eaxSave>], eax
		0x8B, 0x02,										// 0x05 - mov eax, [edx]
		0xA3, 0x00, 0x00, 0x00, 0x00,					// 0x07 - mov [<espSave>], eax
		0xA1, 0x00, 0x00, 0x00, 0x00,					// 0x0C - mov eax, [<eaxSave>]
		0xC7, 0x02, 0x00, 0x00, 0x00, 0x00,				// 0x11 - mov [edx], imm32
		0x0F, 0x34,										// 0x17 - syscall
		0xFF, 0x35, 0x00, 0x00, 0x00, 0x00				// 0x19 - push [<espSave>]
	};

	memcpy(px86.cursor, pSaveEdxCode, sizeof(pSaveEdxCode));
	*(unsigned int *)(&(px86.cursor[0x01])) = (unsigned int)&runtime->returnRegister;
	*(unsigned int *)(&(px86.cursor[0x08])) = (unsigned int)&runtime->jumpBuff;
	*(unsigned int *)(&(px86.cursor[0x0D])) = (unsigned int)&runtime->returnRegister;
	*(unsigned int *)(&(px86.cursor[0x13])) = ((unsigned int)px86.cursor) + 0x19;
	*(unsigned int *)(&(px86.cursor[0x1B])) = (unsigned int)&runtime->jumpBuff;

	px86.SetRelocation(&px86.cursor[0x13]);
	//needsRAFix = true;
	//rvAddress = &px86[0x13];

	px86.cursor += sizeof(pSaveEdxCode);
	instrCounter += 7;
}


void NativeX86Assembler::AssembleSyscall(const RiverInstruction &ri, RelocableCodeBuffer &px86, DWORD &pFlags, DWORD &instrCounter) {
	px86.cursor--;
	ClearPrefixes(ri, px86.cursor);
	AssembleLeaveForSyscall(ri, px86, pFlags, instrCounter, &NativeX86Assembler::AssembleSyscall2, &NativeX86Assembler::AssembleNoOp);
}

void NativeX86Assembler::AssembleFFCallInstr(const RiverInstruction &ri, RelocableCodeBuffer &px86, DWORD &pFlags, DWORD &instrCounter) {
	/*static const char pGetAddrCode[] = {
	0xA3, 0x00, 0x00, 0x00, 0x00,				// 0x00 - [<dwEaxSave>], eax
	//0x8B										// 0x05 - mov ...
	};*/

	static const char pBranchFFCall[] = {
		0x68, 0x00, 0x00, 0x00, 0x00,				// 0x00 - push <retAddr>
		0x87, 0x25, 0x00, 0x00, 0x00, 0x00,			// 0x05 - xchg esp, large [<dwVirtualStack>]
		0x9C, 										// 0x0B - pushf
		0x60,										// 0x0C - pusha
		0x68, 0x46, 0x02, 0x00, 0x00,				// 0x0D - push 0x00000246 - NEW FLAGS
		0x9D,										// 0x12 - popf
		0x50,                            			// 0x13 - push eax
		0xA1, 0x00, 0x00, 0x00, 0x00,				// 0x14	- mov eax, [<dwEaxSave>]
		0x89, 0x44, 0x24, 0x20,						// 0x19 - mov [esp+0x20], eax // fix the eax value
		0x68, 0x00, 0x00, 0x00, 0x00,				// 0x1D - push <execution_environment>
		0xFF, 0x15, 0x00, 0x00, 0x00, 0x00,         // 0x22 - call [<dwBranchHandler>]

		0x61,										// 0x28 - popa
		0x9D,										// 0x29 - popf
		0x87, 0x25, 0x00, 0x00, 0x00, 0x00,			// 0x2A - xchg esp, large ds:<dwVirtualStack>
		0xFF, 0x25, 0x00, 0x00, 0x00, 0x00			// 0x30 - jmp large dword ptr ds:<jumpbuff>
	};

	ClearPrefixes(ri, px86.cursor);

	if ((ri.modifiers & 0x07) == RIVER_MODIFIER_FSSEG) {
		AssembleLeaveForSyscall(ri, px86, pFlags, instrCounter, &NativeX86Assembler::AssembleDefaultInstr, &NativeX86Assembler::AssembleSubOpModRMOp);
		pFlags |= RIVER_FLAG_BRANCH;
		return;
	}

	int retAddr = (int)(ri.operands[1].asImm32);

	*px86.cursor = 0xA3; // xchg eax, [<eaxSave>] 
	++px86.cursor;
	*(unsigned int *)(px86.cursor) = (unsigned int)&runtime->returnRegister;
	px86.cursor += 4;

	GeneratePrefixes(ri, px86.cursor);
	*px86.cursor = 0x8B; // mov eax, [jmpAddr]
	++px86.cursor;
	ri.operands[0].asAddress->EncodeTox86(px86.cursor, RIVER_REG_xAX, 0, 0); // use flags?

	memcpy(px86.cursor, pBranchFFCall, sizeof(pBranchFFCall));

	*(unsigned int *)(&(px86.cursor[0x01])) = retAddr;
	*(unsigned int *)(&(px86.cursor[0x07])) = (unsigned int)&runtime->virtualStack;
	*(unsigned int *)(&(px86.cursor[0x15])) = (unsigned int)&runtime->returnRegister;
	*(unsigned int *)(&(px86.cursor[0x1E])) = (unsigned int)runtime;
	*(unsigned int *)(&(px86.cursor[0x24])) = (unsigned int)&dwBranchHandler;
	*(unsigned int *)(&(px86.cursor[0x2C])) = (unsigned int)&runtime->virtualStack;
	*(unsigned int *)(&(px86.cursor[0x32])) = (unsigned int)&runtime->jumpBuff;
	px86.cursor += sizeof(pBranchFFCall);
	pFlags |= RIVER_FLAG_BRANCH;

	instrCounter += 17;
}

void NativeX86Assembler::AssembleRetnInstr(const RiverInstruction &ri, RelocableCodeBuffer &px86, DWORD &pFlags, DWORD &instrCounter) {
	//RetImm - copy the value
	//Retn - 0
	//RetFar - 4
	const BYTE pBranchRet[] = {
		0xA3, 0x00, 0x00, 0x00, 0x00,				// 0x00 - mov [<dwEaxSave>], eax
		0x58,										// 0x05 - pop eax
		0x87, 0x25, 0x00, 0x00, 0x00, 0x00,			// 0x06 - xchg esp, large ds:<dwVirtualStack>
		0x9C,			 							// 0x0C - pushf
		0x60,										// 0x0D - pusha
		0x68, 0x46, 0x02, 0x00, 0x00,				// 0x0E - push 0x00000246 - NEW FLAGS
		0x9D,										// 0x13 - popf
		0x50,										// 0x14 - push eax
		0xA1, 0x00, 0x00, 0x00, 0x00,				// 0x15	- mov eax, [<dwEaxSave>]
		0x89, 0x44, 0x24, 0x20,						// 0x1A - mov [esp+0x20], eax // fix the eax value
		0x68, 0x00, 0x00, 0x00, 0x00,				// 0x1E - push <execution_environment>
		0xFF, 0x15, 0x00, 0x00, 0x00, 0x00,			// 0x23 - call <branch_handler>

		0x61,										// 0x29 - popa
		0x9D,										// 0x2A - popf
		0x87, 0x25, 0x00, 0x00, 0x00, 0x00,			// 0x2B - xchg esp, large ds:<dwVirtualStack>
		0x8D, 0xA4, 0x24, 0x00, 0x00, 0x00, 0x00,   // 0x31 - lea esp, [esp + <pI>] // probably sub esp, xxx
		0xFF, 0x25, 0x00, 0x00, 0x00, 0x00			// 0x38 - jmp large dword ptr ds:<jumpbuff>
	};

	unsigned short stackSpace = 0;
	memcpy(px86.cursor, pBranchRet, sizeof(pBranchRet));

	*(unsigned int *)(&(px86.cursor[0x01])) = (unsigned int)&runtime->returnRegister;
	*(unsigned int *)(&(px86.cursor[0x08])) = (unsigned int)&runtime->virtualStack;
	*(unsigned int *)(&(px86.cursor[0x16])) = (unsigned int)&runtime->returnRegister;
	*(unsigned int *)(&(px86.cursor[0x1F])) = (unsigned int)runtime;
	*(unsigned int *)(&(px86.cursor[0x25])) = (unsigned int)&dwBranchHandler;
	*(unsigned int *)(&(px86.cursor[0x2D])) = (unsigned int)&runtime->virtualStack;
	*(unsigned int *)(&(px86.cursor[0x34])) = stackSpace;
	*(unsigned int *)(&(px86.cursor[0x3A])) = (unsigned int)&runtime->jumpBuff;

	px86.cursor += sizeof(pBranchRet);
	pFlags |= RIVER_FLAG_BRANCH;
	instrCounter += 17;
}

void NativeX86Assembler::AssembleRetnImmInstr(const RiverInstruction &ri, RelocableCodeBuffer &px86, DWORD &pFlags, DWORD &instrCounter) {
	//RetImm - copy the value
	//Retn - 0
	//RetFar - 4
	const BYTE pBranchRet[] = {
		0xA3, 0x00, 0x00, 0x00, 0x00,				// 0x00 - mov [<dwEaxSave>], eax
		0x58,										// 0x05 - pop eax
		0x87, 0x25, 0x00, 0x00, 0x00, 0x00,			// 0x06 - xchg esp, large ds:<dwVirtualStack>
		0x9C,			 							// 0x0C - pushf
		0x60,										// 0x0D - pusha
		0x68, 0x46, 0x02, 0x00, 0x00,				// 0x0E - push 0x00000246 - NEW FLAGS
		0x9D,										// 0x13 - popf
		0x50,										// 0x14 - push eax
		0xA1, 0x00, 0x00, 0x00, 0x00,				// 0x15	- mov eax, [<dwEaxSave>]
		0x89, 0x44, 0x24, 0x20,						// 0x1A - mov [esp+0x20], eax // fix the eax value
		0x68, 0x00, 0x00, 0x00, 0x00,				// 0x1E - push <execution_environment>
		0xFF, 0x15, 0x00, 0x00, 0x00, 0x00,			// 0x23 - call <branch_handler>

		0x61,										// 0x29 - popa
		0x9D,										// 0x2A - popf
		0x87, 0x25, 0x00, 0x00, 0x00, 0x00,			// 0x2B - xchg esp, large ds:<dwVirtualStack>
		0x8D, 0xA4, 0x24, 0x00, 0x00, 0x00, 0x00,   // 0x31 - lea esp, [esp + <pI>] // probably sub esp, xxx
		0xFF, 0x25, 0x00, 0x00, 0x00, 0x00			// 0x38 - jmp large dword ptr ds:<jumpbuff>
	};

	unsigned short stackSpace = ri.operands[0].asImm16;
	memcpy(px86.cursor, pBranchRet, sizeof(pBranchRet));

	*(unsigned int *)(&(px86.cursor[0x01])) = (unsigned int)&runtime->returnRegister;
	*(unsigned int *)(&(px86.cursor[0x08])) = (unsigned int)&runtime->virtualStack;
	*(unsigned int *)(&(px86.cursor[0x16])) = (unsigned int)&runtime->returnRegister;
	*(unsigned int *)(&(px86.cursor[0x1F])) = (unsigned int)runtime;
	*(unsigned int *)(&(px86.cursor[0x25])) = (unsigned int)&dwBranchHandler;
	*(unsigned int *)(&(px86.cursor[0x2D])) = (unsigned int)&runtime->virtualStack;

	*(unsigned int *)(&(px86.cursor[0x34])) = stackSpace;
	*(unsigned int *)(&(px86.cursor[0x3A])) = (unsigned int)&runtime->jumpBuff;

	px86.cursor += sizeof(pBranchRet);
	pFlags |= RIVER_FLAG_BRANCH;
	instrCounter += 17;
}


/* =========================================== */
/* Operand assemblers                          */
/* =========================================== */

void NativeX86Assembler::AssembleUnknownOp(const RiverInstruction &ri, RelocableCodeBuffer &px86) {
	::AssembleUnknownOp(ri, px86);
}

void NativeX86Assembler::AssembleNoOp(const RiverInstruction &ri, RelocableCodeBuffer &px86) {
	::AssembleNoOp(ri, px86);
}

void NativeX86Assembler::AssembleModRMImm8Op(const RiverInstruction &ri, RelocableCodeBuffer &px86) {
	::AssembleModRMImm8Op(ri, px86);
}

void NativeX86Assembler::AssembleModRMImm32Op(const RiverInstruction &ri, RelocableCodeBuffer &px86) {
	::AssembleModRMImm32Op(ri, px86);
}

void NativeX86Assembler::AssembleRegModRMOp(const RiverInstruction &ri, RelocableCodeBuffer &px86) {
	::AssembleRegModRMOp(ri, px86);
}

void NativeX86Assembler::AssembleModRMRegOp(const RiverInstruction &ri, RelocableCodeBuffer &px86) {
	::AssembleModRMRegOp(ri, px86);
}

void NativeX86Assembler::AssembleSubOpModRMOp(const RiverInstruction &ri, RelocableCodeBuffer &px86) {
	::AssembleSubOpModRMOp(ri, px86);
}

void NativeX86Assembler::AssembleSubOpModRMImm8Op(const RiverInstruction &ri, RelocableCodeBuffer &px86) {
	::AssembleSubOpModRMImm8Op(ri, px86);
}

void NativeX86Assembler::AssembleSubOpModRMImm32Op(const RiverInstruction &ri, RelocableCodeBuffer &px86) {
	::AssembleSubOpModRMImm32Op(ri, px86);
}

void NativeX86Assembler::AssembleRegModRMImm32Op(const RiverInstruction &ri, RelocableCodeBuffer &px86) {
	::AssembleRegModRMImm32Op(ri, px86);
}

void NativeX86Assembler::AssembleRegModRMImm8Op(const RiverInstruction &ri, RelocableCodeBuffer &px86) {
	::AssembleRegModRMImm8Op(ri, px86);
}

void NativeX86Assembler::AssembleModRMRegImm8Op(const RiverInstruction &ri, RelocableCodeBuffer &px86) {
	::AssembleModRMRegImm8Op(ri, px86);
}


/* =========================================== */
/* Additional assembly tables                  */
/* =========================================== */

NativeX86Assembler::AssembleOpcodeFunc NativeX86Assembler::assemble0xF6Instr[8] = {
	/*0x00*/ &NativeX86Assembler::AssembleDefaultInstr, &NativeX86Assembler::AssembleDefaultInstr, &NativeX86Assembler::AssembleDefaultInstr, &NativeX86Assembler::AssembleDefaultInstr,
	/*0x04*/ &NativeX86Assembler::AssembleUnkInstr, &NativeX86Assembler::AssembleUnkInstr, &NativeX86Assembler::AssembleUnkInstr, &NativeX86Assembler::AssembleUnkInstr
};

NativeX86Assembler::AssembleOperandsFunc NativeX86Assembler::assemble0xF6Op[8] = {
	/*0x00*/ &NativeX86Assembler::AssembleSubOpModRMImm8Op, &NativeX86Assembler::AssembleSubOpModRMImm8Op, &NativeX86Assembler::AssembleModRMOp<3>, &NativeX86Assembler::AssembleModRMOp<4>,
	/*0x04*/ &NativeX86Assembler::AssembleUnknownOp, &NativeX86Assembler::AssembleUnknownOp, &NativeX86Assembler::AssembleUnknownOp, &NativeX86Assembler::AssembleUnknownOp,
};

NativeX86Assembler::AssembleOpcodeFunc NativeX86Assembler::assemble0xF7Instr[8] = {
	/*0x00*/ &NativeX86Assembler::AssembleDefaultInstr, &NativeX86Assembler::AssembleDefaultInstr, &NativeX86Assembler::AssembleDefaultInstr, &NativeX86Assembler::AssembleDefaultInstr,
	/*0x04*/ &NativeX86Assembler::AssembleDefaultInstr, &NativeX86Assembler::AssembleDefaultInstr, &NativeX86Assembler::AssembleDefaultInstr, &NativeX86Assembler::AssembleDefaultInstr
};

NativeX86Assembler::AssembleOperandsFunc NativeX86Assembler::assemble0xF7Op[8] = {
	/*0x00*/ &NativeX86Assembler::AssembleSubOpModRMImm32Op, &NativeX86Assembler::AssembleSubOpModRMImm32Op, &NativeX86Assembler::AssembleModRMOp<2>, &NativeX86Assembler::AssembleModRMOp<3>,
	/*0x04*/ &NativeX86Assembler::AssembleModRMOp<4>, &NativeX86Assembler::AssembleModRMOp<5>, &NativeX86Assembler::AssembleModRMOp<6>, &NativeX86Assembler::AssembleModRMOp<7>,
};

NativeX86Assembler::AssembleOpcodeFunc NativeX86Assembler::assemble0xFFInstr[8] = {
	/*0x00*/ &NativeX86Assembler::AssembleDefaultInstr, &NativeX86Assembler::AssembleDefaultInstr, &NativeX86Assembler::AssembleFFCallInstr, &NativeX86Assembler::AssembleUnkInstr,
	/*0x04*/ &NativeX86Assembler::AssembleFFJumpInstr, &NativeX86Assembler::AssembleUnkInstr, &NativeX86Assembler::AssembleDefaultInstr, &NativeX86Assembler::AssembleUnkInstr
};

NativeX86Assembler::AssembleOperandsFunc NativeX86Assembler::assemble0xFFOp[8] = {
	/*0x00*/ &NativeX86Assembler::AssembleSubOpModRMOp, &NativeX86Assembler::AssembleSubOpModRMOp, &NativeX86Assembler::AssembleNoOp, &NativeX86Assembler::AssembleUnknownOp,
	/*0x04*/ &NativeX86Assembler::AssembleNoOp, &NativeX86Assembler::AssembleUnknownOp, &NativeX86Assembler::AssembleModRMOp<6>, &NativeX86Assembler::AssembleUnknownOp,
};

NativeX86Assembler::AssembleOpcodeFunc NativeX86Assembler::assemble0x0FC7Instr[8] = {
	/*0x00*/ &NativeX86Assembler::AssembleUnkInstr, &NativeX86Assembler::AssembleUnkInstr, &NativeX86Assembler::AssembleUnkInstr, &NativeX86Assembler::AssembleUnkInstr,
	/*0x04*/ &NativeX86Assembler::AssembleUnkInstr, &NativeX86Assembler::AssembleUnkInstr, &NativeX86Assembler::AssembleUnkInstr, &NativeX86Assembler::AssembleUnkInstr
};

NativeX86Assembler::AssembleOperandsFunc NativeX86Assembler::assemble0x0FC7Op[8] = {
	/*0x00*/ &NativeX86Assembler::AssembleUnknownOp, &NativeX86Assembler::AssembleUnknownOp, &NativeX86Assembler::AssembleUnknownOp, &NativeX86Assembler::AssembleUnknownOp,
	/*0x04*/ &NativeX86Assembler::AssembleUnknownOp, &NativeX86Assembler::AssembleUnknownOp, &NativeX86Assembler::AssembleUnknownOp, &NativeX86Assembler::AssembleUnknownOp,
};

/* =========================================== */
/* Assembly tables                             */
/* =========================================== */

NativeX86Assembler::AssembleOpcodeFunc NativeX86Assembler::assembleOpcodes[2][0x100] = {
	{
		/*0x00*/ &NativeX86Assembler::AssembleDefaultInstr, &NativeX86Assembler::AssembleDefaultInstr, &NativeX86Assembler::AssembleDefaultInstr, &NativeX86Assembler::AssembleDefaultInstr,
		/*0x04*/ &NativeX86Assembler::AssembleDefaultInstr, &NativeX86Assembler::AssembleDefaultInstr, &NativeX86Assembler::AssembleDefaultInstr, &NativeX86Assembler::AssembleDefaultInstr,
		/*0x08*/ &NativeX86Assembler::AssembleDefaultInstr, &NativeX86Assembler::AssembleDefaultInstr, &NativeX86Assembler::AssembleDefaultInstr, &NativeX86Assembler::AssembleDefaultInstr,
		/*0x0C*/ &NativeX86Assembler::AssembleDefaultInstr, &NativeX86Assembler::AssembleDefaultInstr, &NativeX86Assembler::AssembleUnkInstr, &NativeX86Assembler::AssembleUnkInstr,

		/*0x10*/ &NativeX86Assembler::AssembleDefaultInstr, &NativeX86Assembler::AssembleDefaultInstr, &NativeX86Assembler::AssembleDefaultInstr, &NativeX86Assembler::AssembleDefaultInstr,
		/*0x14*/ &NativeX86Assembler::AssembleDefaultInstr, &NativeX86Assembler::AssembleDefaultInstr, &NativeX86Assembler::AssembleDefaultInstr, &NativeX86Assembler::AssembleDefaultInstr,
		/*0x18*/ &NativeX86Assembler::AssembleDefaultInstr, &NativeX86Assembler::AssembleDefaultInstr, &NativeX86Assembler::AssembleDefaultInstr, &NativeX86Assembler::AssembleDefaultInstr,
		/*0x1C*/ &NativeX86Assembler::AssembleDefaultInstr, &NativeX86Assembler::AssembleDefaultInstr, &NativeX86Assembler::AssembleUnkInstr, &NativeX86Assembler::AssembleUnkInstr,

		/*0x20*/ &NativeX86Assembler::AssembleDefaultInstr, &NativeX86Assembler::AssembleDefaultInstr, &NativeX86Assembler::AssembleDefaultInstr, &NativeX86Assembler::AssembleDefaultInstr,
		/*0x24*/ &NativeX86Assembler::AssembleDefaultInstr, &NativeX86Assembler::AssembleDefaultInstr, &NativeX86Assembler::AssembleUnkInstr, &NativeX86Assembler::AssembleUnkInstr,
		/*0x28*/ &NativeX86Assembler::AssembleDefaultInstr, &NativeX86Assembler::AssembleDefaultInstr, &NativeX86Assembler::AssembleDefaultInstr, &NativeX86Assembler::AssembleDefaultInstr,
		/*0x2C*/ &NativeX86Assembler::AssembleDefaultInstr, &NativeX86Assembler::AssembleDefaultInstr, &NativeX86Assembler::AssembleUnkInstr, &NativeX86Assembler::AssembleUnkInstr,

		/*0x30*/ &NativeX86Assembler::AssembleDefaultInstr, &NativeX86Assembler::AssembleDefaultInstr, &NativeX86Assembler::AssembleDefaultInstr, &NativeX86Assembler::AssembleDefaultInstr,
		/*0x34*/ &NativeX86Assembler::AssembleDefaultInstr, &NativeX86Assembler::AssembleDefaultInstr, &NativeX86Assembler::AssembleUnkInstr, &NativeX86Assembler::AssembleUnkInstr,
		/*0x38*/ &NativeX86Assembler::AssembleDefaultInstr, &NativeX86Assembler::AssembleDefaultInstr, &NativeX86Assembler::AssembleDefaultInstr, &NativeX86Assembler::AssembleDefaultInstr,
		/*0x3C*/ &NativeX86Assembler::AssembleDefaultInstr, &NativeX86Assembler::AssembleDefaultInstr, &NativeX86Assembler::AssembleUnkInstr, &NativeX86Assembler::AssembleUnkInstr,

		/*0x40*/ &NativeX86Assembler::AssemblePlusRegInstr, &NativeX86Assembler::AssemblePlusRegInstr, &NativeX86Assembler::AssemblePlusRegInstr, &NativeX86Assembler::AssemblePlusRegInstr,
		/*0x44*/ &NativeX86Assembler::AssemblePlusRegInstr, &NativeX86Assembler::AssemblePlusRegInstr, &NativeX86Assembler::AssemblePlusRegInstr, &NativeX86Assembler::AssemblePlusRegInstr,
		/*0x48*/ &NativeX86Assembler::AssemblePlusRegInstr, &NativeX86Assembler::AssemblePlusRegInstr, &NativeX86Assembler::AssemblePlusRegInstr, &NativeX86Assembler::AssemblePlusRegInstr,
		/*0x4C*/ &NativeX86Assembler::AssemblePlusRegInstr, &NativeX86Assembler::AssemblePlusRegInstr, &NativeX86Assembler::AssemblePlusRegInstr, &NativeX86Assembler::AssemblePlusRegInstr,

		/*0x50*/ &NativeX86Assembler::AssemblePlusRegInstr, &NativeX86Assembler::AssemblePlusRegInstr, &NativeX86Assembler::AssemblePlusRegInstr, &NativeX86Assembler::AssemblePlusRegInstr,
		/*0x54*/ &NativeX86Assembler::AssemblePlusRegInstr, &NativeX86Assembler::AssemblePlusRegInstr, &NativeX86Assembler::AssemblePlusRegInstr, &NativeX86Assembler::AssemblePlusRegInstr,
		/*0x58*/ &NativeX86Assembler::AssemblePlusRegInstr, &NativeX86Assembler::AssemblePlusRegInstr, &NativeX86Assembler::AssemblePlusRegInstr, &NativeX86Assembler::AssemblePlusRegInstr,
		/*0x5C*/ &NativeX86Assembler::AssemblePlusRegInstr, &NativeX86Assembler::AssemblePlusRegInstr, &NativeX86Assembler::AssemblePlusRegInstr, &NativeX86Assembler::AssemblePlusRegInstr,

		/*0x60*/ &NativeX86Assembler::AssembleUnkInstr, &NativeX86Assembler::AssembleUnkInstr, &NativeX86Assembler::AssembleUnkInstr, &NativeX86Assembler::AssembleUnkInstr,
		/*0x64*/ &NativeX86Assembler::AssembleUnkInstr, &NativeX86Assembler::AssembleUnkInstr, &NativeX86Assembler::AssembleUnkInstr, &NativeX86Assembler::AssembleUnkInstr,
		/*0x68*/ &NativeX86Assembler::AssembleDefaultInstr, &NativeX86Assembler::AssembleDefaultInstr, &NativeX86Assembler::AssembleDefaultInstr, &NativeX86Assembler::AssembleDefaultInstr,
		/*0x6C*/ &NativeX86Assembler::AssembleUnkInstr, &NativeX86Assembler::AssembleUnkInstr, &NativeX86Assembler::AssembleUnkInstr, &NativeX86Assembler::AssembleUnkInstr,

		/*0x70*/ &NativeX86Assembler::AssembleRelJmpCondInstr, &NativeX86Assembler::AssembleRelJmpCondInstr, &NativeX86Assembler::AssembleRelJmpCondInstr, &NativeX86Assembler::AssembleRelJmpCondInstr,
		/*0x74*/ &NativeX86Assembler::AssembleRelJmpCondInstr, &NativeX86Assembler::AssembleRelJmpCondInstr, &NativeX86Assembler::AssembleRelJmpCondInstr, &NativeX86Assembler::AssembleRelJmpCondInstr,
		/*0x78*/ &NativeX86Assembler::AssembleRelJmpCondInstr, &NativeX86Assembler::AssembleRelJmpCondInstr, &NativeX86Assembler::AssembleRelJmpCondInstr, &NativeX86Assembler::AssembleRelJmpCondInstr,
		/*0x7C*/ &NativeX86Assembler::AssembleRelJmpCondInstr, &NativeX86Assembler::AssembleRelJmpCondInstr, &NativeX86Assembler::AssembleRelJmpCondInstr, &NativeX86Assembler::AssembleRelJmpCondInstr,

		/*0x80*/ &NativeX86Assembler::AssembleDefaultInstr, &NativeX86Assembler::AssembleDefaultInstr, &NativeX86Assembler::AssembleUnkInstr, &NativeX86Assembler::AssembleDefaultInstr,
		/*0x84*/ &NativeX86Assembler::AssembleDefaultInstr, &NativeX86Assembler::AssembleDefaultInstr, &NativeX86Assembler::AssembleDefaultInstr, &NativeX86Assembler::AssembleDefaultInstr,
		/*0x88*/ &NativeX86Assembler::AssembleDefaultInstr, &NativeX86Assembler::AssembleDefaultInstr, &NativeX86Assembler::AssembleDefaultInstr, &NativeX86Assembler::AssembleDefaultInstr,
		/*0x8C*/ &NativeX86Assembler::AssembleUnkInstr, &NativeX86Assembler::AssembleDefaultInstr, &NativeX86Assembler::AssembleUnkInstr, &NativeX86Assembler::AssembleDefaultInstr,

		/*0x90*/ &NativeX86Assembler::AssemblePlusRegInstr, &NativeX86Assembler::AssemblePlusRegInstr, &NativeX86Assembler::AssemblePlusRegInstr, &NativeX86Assembler::AssemblePlusRegInstr,
		/*0x94*/ &NativeX86Assembler::AssemblePlusRegInstr, &NativeX86Assembler::AssemblePlusRegInstr, &NativeX86Assembler::AssemblePlusRegInstr, &NativeX86Assembler::AssemblePlusRegInstr,
		/*0x98*/ &NativeX86Assembler::AssembleUnkInstr, &NativeX86Assembler::AssembleDefaultInstr, &NativeX86Assembler::AssembleUnkInstr, &NativeX86Assembler::AssembleUnkInstr,
		/*0x9C*/ &NativeX86Assembler::AssembleDefaultInstr, &NativeX86Assembler::AssembleDefaultInstr, &NativeX86Assembler::AssembleUnkInstr, &NativeX86Assembler::AssembleUnkInstr,

		/*0xA0*/ &NativeX86Assembler::AssembleDefaultInstr, &NativeX86Assembler::AssembleDefaultInstr, &NativeX86Assembler::AssembleDefaultInstr, &NativeX86Assembler::AssembleDefaultInstr,
		/*0xA4*/ &NativeX86Assembler::AssembleDefaultInstr, &NativeX86Assembler::AssembleDefaultInstr, &NativeX86Assembler::AssembleDefaultInstr, &NativeX86Assembler::AssembleDefaultInstr,
		/*0xA8*/ &NativeX86Assembler::AssembleDefaultInstr, &NativeX86Assembler::AssembleDefaultInstr, &NativeX86Assembler::AssembleDefaultInstr, &NativeX86Assembler::AssembleDefaultInstr,
		/*0xAC*/ &NativeX86Assembler::AssembleUnkInstr, &NativeX86Assembler::AssembleUnkInstr, &NativeX86Assembler::AssembleDefaultInstr, &NativeX86Assembler::AssembleDefaultInstr,

		/*0xB0*/ &NativeX86Assembler::AssemblePlusRegInstr, &NativeX86Assembler::AssemblePlusRegInstr, &NativeX86Assembler::AssemblePlusRegInstr, &NativeX86Assembler::AssemblePlusRegInstr,
		/*0xB4*/ &NativeX86Assembler::AssemblePlusRegInstr, &NativeX86Assembler::AssemblePlusRegInstr, &NativeX86Assembler::AssemblePlusRegInstr, &NativeX86Assembler::AssemblePlusRegInstr,
		/*0xB8*/ &NativeX86Assembler::AssemblePlusRegInstr, &NativeX86Assembler::AssemblePlusRegInstr, &NativeX86Assembler::AssemblePlusRegInstr, &NativeX86Assembler::AssemblePlusRegInstr,
		/*0xBC*/ &NativeX86Assembler::AssemblePlusRegInstr, &NativeX86Assembler::AssemblePlusRegInstr, &NativeX86Assembler::AssemblePlusRegInstr, &NativeX86Assembler::AssemblePlusRegInstr,

		/*0xC0*/ &NativeX86Assembler::AssembleDefaultInstr, &NativeX86Assembler::AssembleDefaultInstr, &NativeX86Assembler::AssembleRetnImmInstr, &NativeX86Assembler::AssembleRetnInstr,
		/*0xC4*/ &NativeX86Assembler::AssembleUnkInstr, &NativeX86Assembler::AssembleUnkInstr, &NativeX86Assembler::AssembleDefaultInstr, &NativeX86Assembler::AssembleDefaultInstr,
		/*0xC8*/ &NativeX86Assembler::AssembleUnkInstr, &NativeX86Assembler::AssembleDefaultInstr, &NativeX86Assembler::AssembleUnkInstr, &NativeX86Assembler::AssembleUnkInstr,
		/*0xCC*/ &NativeX86Assembler::AssembleUnkInstr, &NativeX86Assembler::AssembleUnkInstr, &NativeX86Assembler::AssembleUnkInstr, &NativeX86Assembler::AssembleUnkInstr,

		/*0xD0*/ &NativeX86Assembler::AssembleDefaultInstr, &NativeX86Assembler::AssembleDefaultInstr, &NativeX86Assembler::AssembleDefaultInstr, &NativeX86Assembler::AssembleDefaultInstr,
		/*0xD4*/ &NativeX86Assembler::AssembleUnkInstr, &NativeX86Assembler::AssembleUnkInstr, &NativeX86Assembler::AssembleUnkInstr, &NativeX86Assembler::AssembleUnkInstr,
		/*0xD8*/ &NativeX86Assembler::AssembleUnkInstr, &NativeX86Assembler::AssembleUnkInstr, &NativeX86Assembler::AssembleUnkInstr, &NativeX86Assembler::AssembleUnkInstr,
		/*0xDC*/ &NativeX86Assembler::AssembleUnkInstr, &NativeX86Assembler::AssembleUnkInstr, &NativeX86Assembler::AssembleUnkInstr, &NativeX86Assembler::AssembleUnkInstr,

		/*0xE0*/ &NativeX86Assembler::AssembleUnkInstr, &NativeX86Assembler::AssembleUnkInstr, &NativeX86Assembler::AssembleUnkInstr, &NativeX86Assembler::AssembleUnkInstr,
		/*0xE4*/ &NativeX86Assembler::AssembleUnkInstr, &NativeX86Assembler::AssembleUnkInstr, &NativeX86Assembler::AssembleUnkInstr, &NativeX86Assembler::AssembleUnkInstr,
		/*0xE8*/ &NativeX86Assembler::AssembleCallInstr, &NativeX86Assembler::AssembleRelJMPInstr, &NativeX86Assembler::AssembleJMPInstr, &NativeX86Assembler::AssembleRelJMPInstr,
		/*0xEC*/ &NativeX86Assembler::AssembleUnkInstr, &NativeX86Assembler::AssembleUnkInstr, &NativeX86Assembler::AssembleUnkInstr, &NativeX86Assembler::AssembleUnkInstr,

		/*0xF0*/ &NativeX86Assembler::AssembleUnkInstr, &NativeX86Assembler::AssembleUnkInstr, &NativeX86Assembler::AssembleUnkInstr, &NativeX86Assembler::AssembleUnkInstr,
		/*0xF4*/ &NativeX86Assembler::AssembleUnkInstr, &NativeX86Assembler::AssembleUnkInstr, &NativeX86Assembler::AssembleSubOpInstr<assemble0xF6Instr>, &NativeX86Assembler::AssembleSubOpInstr<assemble0xF7Instr>,
		/*0xF8*/ &NativeX86Assembler::AssembleUnkInstr, &NativeX86Assembler::AssembleUnkInstr, &NativeX86Assembler::AssembleUnkInstr, &NativeX86Assembler::AssembleUnkInstr,
		/*0xFC*/ &NativeX86Assembler::AssembleDefaultInstr, &NativeX86Assembler::AssembleDefaultInstr, &NativeX86Assembler::AssembleDefaultInstr, &NativeX86Assembler::AssembleSubOpInstr<assemble0xFFInstr>
	}, {
		/*0x00*/ &NativeX86Assembler::AssembleUnkInstr, &NativeX86Assembler::AssembleUnkInstr, &NativeX86Assembler::AssembleDefaultInstr, &NativeX86Assembler::AssembleDefaultInstr,
		/*0x04*/ &NativeX86Assembler::AssembleUnkInstr, &NativeX86Assembler::AssembleUnkInstr, &NativeX86Assembler::AssembleUnkInstr, &NativeX86Assembler::AssembleUnkInstr,
		/*0x08*/ &NativeX86Assembler::AssembleUnkInstr, &NativeX86Assembler::AssembleUnkInstr, &NativeX86Assembler::AssembleUnkInstr, &NativeX86Assembler::AssembleUnkInstr,
		/*0x0C*/ &NativeX86Assembler::AssembleUnkInstr, &NativeX86Assembler::AssembleUnkInstr, &NativeX86Assembler::AssembleUnkInstr, &NativeX86Assembler::AssembleUnkInstr,

		/*0x10*/ &NativeX86Assembler::AssembleUnkInstr, &NativeX86Assembler::AssembleUnkInstr, &NativeX86Assembler::AssembleUnkInstr, &NativeX86Assembler::AssembleUnkInstr,
		/*0x14*/ &NativeX86Assembler::AssembleUnkInstr, &NativeX86Assembler::AssembleUnkInstr, &NativeX86Assembler::AssembleUnkInstr, &NativeX86Assembler::AssembleUnkInstr,
		/*0x18*/ &NativeX86Assembler::AssembleUnkInstr, &NativeX86Assembler::AssembleUnkInstr, &NativeX86Assembler::AssembleUnkInstr, &NativeX86Assembler::AssembleUnkInstr,
		/*0x1C*/ &NativeX86Assembler::AssembleUnkInstr, &NativeX86Assembler::AssembleUnkInstr, &NativeX86Assembler::AssembleUnkInstr, &NativeX86Assembler::AssembleUnkInstr,

		/*0x20*/ &NativeX86Assembler::AssembleUnkInstr, &NativeX86Assembler::AssembleUnkInstr, &NativeX86Assembler::AssembleUnkInstr, &NativeX86Assembler::AssembleUnkInstr,
		/*0x24*/ &NativeX86Assembler::AssembleUnkInstr, &NativeX86Assembler::AssembleUnkInstr, &NativeX86Assembler::AssembleUnkInstr, &NativeX86Assembler::AssembleUnkInstr,
		/*0x28*/ &NativeX86Assembler::AssembleUnkInstr, &NativeX86Assembler::AssembleUnkInstr, &NativeX86Assembler::AssembleUnkInstr, &NativeX86Assembler::AssembleUnkInstr,
		/*0x2C*/ &NativeX86Assembler::AssembleUnkInstr, &NativeX86Assembler::AssembleUnkInstr, &NativeX86Assembler::AssembleUnkInstr, &NativeX86Assembler::AssembleUnkInstr,

		/*0x30*/ &NativeX86Assembler::AssembleUnkInstr, &NativeX86Assembler::AssembleDefaultInstr, &NativeX86Assembler::AssembleUnkInstr, &NativeX86Assembler::AssembleUnkInstr,
		/*0x34*/ &NativeX86Assembler::AssembleSyscall, &NativeX86Assembler::AssembleUnkInstr, &NativeX86Assembler::AssembleUnkInstr, &NativeX86Assembler::AssembleUnkInstr,
		/*0x38*/ &NativeX86Assembler::AssembleUnkInstr, &NativeX86Assembler::AssembleUnkInstr, &NativeX86Assembler::AssembleUnkInstr, &NativeX86Assembler::AssembleUnkInstr,
		/*0x3C*/ &NativeX86Assembler::AssembleUnkInstr, &NativeX86Assembler::AssembleUnkInstr, &NativeX86Assembler::AssembleUnkInstr, &NativeX86Assembler::AssembleUnkInstr,

		/*0x40*/ &NativeX86Assembler::AssembleDefaultInstr, &NativeX86Assembler::AssembleDefaultInstr, &NativeX86Assembler::AssembleDefaultInstr, &NativeX86Assembler::AssembleDefaultInstr,
		/*0x44*/ &NativeX86Assembler::AssembleDefaultInstr, &NativeX86Assembler::AssembleDefaultInstr, &NativeX86Assembler::AssembleDefaultInstr, &NativeX86Assembler::AssembleDefaultInstr,
		/*0x48*/ &NativeX86Assembler::AssembleDefaultInstr, &NativeX86Assembler::AssembleDefaultInstr, &NativeX86Assembler::AssembleDefaultInstr, &NativeX86Assembler::AssembleDefaultInstr,
		/*0x4C*/ &NativeX86Assembler::AssembleDefaultInstr, &NativeX86Assembler::AssembleDefaultInstr, &NativeX86Assembler::AssembleDefaultInstr, &NativeX86Assembler::AssembleDefaultInstr,

		/*0x50*/ &NativeX86Assembler::AssembleUnkInstr, &NativeX86Assembler::AssembleUnkInstr, &NativeX86Assembler::AssembleUnkInstr, &NativeX86Assembler::AssembleUnkInstr,
		/*0x54*/ &NativeX86Assembler::AssembleUnkInstr, &NativeX86Assembler::AssembleUnkInstr, &NativeX86Assembler::AssembleUnkInstr, &NativeX86Assembler::AssembleUnkInstr,
		/*0x58*/ &NativeX86Assembler::AssembleUnkInstr, &NativeX86Assembler::AssembleUnkInstr, &NativeX86Assembler::AssembleUnkInstr, &NativeX86Assembler::AssembleUnkInstr,
		/*0x5C*/ &NativeX86Assembler::AssembleUnkInstr, &NativeX86Assembler::AssembleUnkInstr, &NativeX86Assembler::AssembleUnkInstr, &NativeX86Assembler::AssembleUnkInstr,

		/*0x60*/ &NativeX86Assembler::AssembleUnkInstr, &NativeX86Assembler::AssembleUnkInstr, &NativeX86Assembler::AssembleUnkInstr, &NativeX86Assembler::AssembleUnkInstr,
		/*0x64*/ &NativeX86Assembler::AssembleUnkInstr, &NativeX86Assembler::AssembleUnkInstr, &NativeX86Assembler::AssembleUnkInstr, &NativeX86Assembler::AssembleUnkInstr,
		/*0x68*/ &NativeX86Assembler::AssembleUnkInstr, &NativeX86Assembler::AssembleUnkInstr, &NativeX86Assembler::AssembleUnkInstr, &NativeX86Assembler::AssembleUnkInstr,
		/*0x6C*/ &NativeX86Assembler::AssembleUnkInstr, &NativeX86Assembler::AssembleUnkInstr, &NativeX86Assembler::AssembleUnkInstr, &NativeX86Assembler::AssembleUnkInstr,

		/*0x70*/ &NativeX86Assembler::AssembleUnkInstr, &NativeX86Assembler::AssembleUnkInstr, &NativeX86Assembler::AssembleUnkInstr, &NativeX86Assembler::AssembleUnkInstr,
		/*0x74*/ &NativeX86Assembler::AssembleUnkInstr, &NativeX86Assembler::AssembleUnkInstr, &NativeX86Assembler::AssembleUnkInstr, &NativeX86Assembler::AssembleUnkInstr,
		/*0x78*/ &NativeX86Assembler::AssembleUnkInstr, &NativeX86Assembler::AssembleUnkInstr, &NativeX86Assembler::AssembleUnkInstr, &NativeX86Assembler::AssembleUnkInstr,
		/*0x7C*/ &NativeX86Assembler::AssembleUnkInstr, &NativeX86Assembler::AssembleUnkInstr, &NativeX86Assembler::AssembleUnkInstr, &NativeX86Assembler::AssembleUnkInstr,

		/*0x80*/ &NativeX86Assembler::AssembleRelJmpCondInstr, &NativeX86Assembler::AssembleRelJmpCondInstr, &NativeX86Assembler::AssembleRelJmpCondInstr, &NativeX86Assembler::AssembleRelJmpCondInstr,
		/*0x84*/ &NativeX86Assembler::AssembleRelJmpCondInstr, &NativeX86Assembler::AssembleRelJmpCondInstr, &NativeX86Assembler::AssembleRelJmpCondInstr, &NativeX86Assembler::AssembleRelJmpCondInstr,
		/*0x88*/ &NativeX86Assembler::AssembleRelJmpCondInstr, &NativeX86Assembler::AssembleRelJmpCondInstr, &NativeX86Assembler::AssembleRelJmpCondInstr, &NativeX86Assembler::AssembleRelJmpCondInstr,
		/*0x8C*/ &NativeX86Assembler::AssembleRelJmpCondInstr, &NativeX86Assembler::AssembleRelJmpCondInstr, &NativeX86Assembler::AssembleRelJmpCondInstr, &NativeX86Assembler::AssembleRelJmpCondInstr,

		/*0x90*/ &NativeX86Assembler::AssembleDefaultInstr, &NativeX86Assembler::AssembleDefaultInstr, &NativeX86Assembler::AssembleDefaultInstr, &NativeX86Assembler::AssembleDefaultInstr,
		/*0x94*/ &NativeX86Assembler::AssembleDefaultInstr, &NativeX86Assembler::AssembleDefaultInstr, &NativeX86Assembler::AssembleDefaultInstr, &NativeX86Assembler::AssembleDefaultInstr,
		/*0x98*/ &NativeX86Assembler::AssembleDefaultInstr, &NativeX86Assembler::AssembleDefaultInstr, &NativeX86Assembler::AssembleDefaultInstr, &NativeX86Assembler::AssembleDefaultInstr,
		/*0x9C*/ &NativeX86Assembler::AssembleDefaultInstr, &NativeX86Assembler::AssembleDefaultInstr, &NativeX86Assembler::AssembleDefaultInstr, &NativeX86Assembler::AssembleDefaultInstr,

		/*0xA0*/ &NativeX86Assembler::AssembleUnkInstr, &NativeX86Assembler::AssembleUnkInstr, &NativeX86Assembler::AssembleDefaultInstr, &NativeX86Assembler::AssembleUnkInstr,
		/*0xA4*/ &NativeX86Assembler::AssembleDefaultInstr, &NativeX86Assembler::AssembleDefaultInstr, &NativeX86Assembler::AssembleUnkInstr, &NativeX86Assembler::AssembleUnkInstr,
		/*0xA8*/ &NativeX86Assembler::AssembleUnkInstr, &NativeX86Assembler::AssembleUnkInstr, &NativeX86Assembler::AssembleUnkInstr, &NativeX86Assembler::AssembleUnkInstr,
		/*0xAC*/ &NativeX86Assembler::AssembleDefaultInstr, &NativeX86Assembler::AssembleDefaultInstr, &NativeX86Assembler::AssembleUnkInstr, &NativeX86Assembler::AssembleDefaultInstr,

		/*0xB0*/ &NativeX86Assembler::AssembleDefaultInstr, &NativeX86Assembler::AssembleDefaultInstr, &NativeX86Assembler::AssembleUnkInstr, &NativeX86Assembler::AssembleUnkInstr,
		/*0xB4*/ &NativeX86Assembler::AssembleUnkInstr, &NativeX86Assembler::AssembleUnkInstr, &NativeX86Assembler::AssembleDefaultInstr, &NativeX86Assembler::AssembleDefaultInstr,
		/*0xB8*/ &NativeX86Assembler::AssembleUnkInstr, &NativeX86Assembler::AssembleUnkInstr, &NativeX86Assembler::AssembleDefaultInstr, &NativeX86Assembler::AssembleUnkInstr,
		/*0xBC*/ &NativeX86Assembler::AssembleDefaultInstr, &NativeX86Assembler::AssembleDefaultInstr, &NativeX86Assembler::AssembleDefaultInstr, &NativeX86Assembler::AssembleDefaultInstr,

		/*0xC0*/ &NativeX86Assembler::AssembleDefaultInstr, &NativeX86Assembler::AssembleDefaultInstr, &NativeX86Assembler::AssembleUnkInstr, &NativeX86Assembler::AssembleUnkInstr,
		/*0xC4*/ &NativeX86Assembler::AssembleUnkInstr, &NativeX86Assembler::AssembleUnkInstr, &NativeX86Assembler::AssembleUnkInstr, &NativeX86Assembler::AssembleSubOpInstr<NativeX86Assembler::assemble0x0FC7Instr>,
		/*0xC8*/ &NativeX86Assembler::AssembleUnkInstr, &NativeX86Assembler::AssembleUnkInstr, &NativeX86Assembler::AssembleUnkInstr, &NativeX86Assembler::AssembleUnkInstr,
		/*0xCC*/ &NativeX86Assembler::AssembleUnkInstr, &NativeX86Assembler::AssembleUnkInstr, &NativeX86Assembler::AssembleUnkInstr, &NativeX86Assembler::AssembleUnkInstr,

		/*0xD0*/ &NativeX86Assembler::AssembleUnkInstr, &NativeX86Assembler::AssembleUnkInstr, &NativeX86Assembler::AssembleUnkInstr, &NativeX86Assembler::AssembleUnkInstr,
		/*0xD4*/ &NativeX86Assembler::AssembleUnkInstr, &NativeX86Assembler::AssembleUnkInstr, &NativeX86Assembler::AssembleUnkInstr, &NativeX86Assembler::AssembleUnkInstr,
		/*0xD8*/ &NativeX86Assembler::AssembleUnkInstr, &NativeX86Assembler::AssembleUnkInstr, &NativeX86Assembler::AssembleUnkInstr, &NativeX86Assembler::AssembleUnkInstr,
		/*0xDC*/ &NativeX86Assembler::AssembleUnkInstr, &NativeX86Assembler::AssembleUnkInstr, &NativeX86Assembler::AssembleUnkInstr, &NativeX86Assembler::AssembleUnkInstr,

		/*0xE0*/ &NativeX86Assembler::AssembleUnkInstr, &NativeX86Assembler::AssembleUnkInstr, &NativeX86Assembler::AssembleUnkInstr, &NativeX86Assembler::AssembleUnkInstr,
		/*0xE4*/ &NativeX86Assembler::AssembleUnkInstr, &NativeX86Assembler::AssembleUnkInstr, &NativeX86Assembler::AssembleUnkInstr, &NativeX86Assembler::AssembleUnkInstr,
		/*0xE8*/ &NativeX86Assembler::AssembleUnkInstr, &NativeX86Assembler::AssembleUnkInstr, &NativeX86Assembler::AssembleUnkInstr, &NativeX86Assembler::AssembleUnkInstr,
		/*0xEC*/ &NativeX86Assembler::AssembleUnkInstr, &NativeX86Assembler::AssembleUnkInstr, &NativeX86Assembler::AssembleUnkInstr, &NativeX86Assembler::AssembleUnkInstr,

		/*0xF0*/ &NativeX86Assembler::AssembleUnkInstr, &NativeX86Assembler::AssembleUnkInstr, &NativeX86Assembler::AssembleUnkInstr, &NativeX86Assembler::AssembleUnkInstr,
		/*0xF4*/ &NativeX86Assembler::AssembleUnkInstr, &NativeX86Assembler::AssembleUnkInstr, &NativeX86Assembler::AssembleUnkInstr, &NativeX86Assembler::AssembleUnkInstr,
		/*0xF8*/ &NativeX86Assembler::AssembleUnkInstr, &NativeX86Assembler::AssembleUnkInstr, &NativeX86Assembler::AssembleUnkInstr, &NativeX86Assembler::AssembleUnkInstr,
		/*0xFC*/ &NativeX86Assembler::AssembleUnkInstr, &NativeX86Assembler::AssembleUnkInstr, &NativeX86Assembler::AssembleUnkInstr, &NativeX86Assembler::AssembleUnkInstr
	}
};

NativeX86Assembler::AssembleOperandsFunc NativeX86Assembler::assembleOperands[2][0x100] = {
	{
		/*0x00*/ &NativeX86Assembler::AssembleModRMRegOp, &NativeX86Assembler::AssembleModRMRegOp, &NativeX86Assembler::AssembleRegModRMOp, &NativeX86Assembler::AssembleRegModRMOp,
		/*0x04*/ &NativeX86Assembler::AssembleImmOp<1, RIVER_OPSIZE_8>, &NativeX86Assembler::AssembleImmOp<1, RIVER_OPSIZE_32>, &NativeX86Assembler::AssembleNoOp, &NativeX86Assembler::AssembleNoOp,
		/*0x08*/ &NativeX86Assembler::AssembleModRMRegOp, &NativeX86Assembler::AssembleModRMRegOp, &NativeX86Assembler::AssembleRegModRMOp, &NativeX86Assembler::AssembleRegModRMOp,
		/*0x0C*/ &NativeX86Assembler::AssembleImmOp<1, RIVER_OPSIZE_8>, &NativeX86Assembler::AssembleImmOp<1, RIVER_OPSIZE_32>, &NativeX86Assembler::AssembleUnknownOp, &NativeX86Assembler::AssembleUnknownOp,

		/*0x10*/ &NativeX86Assembler::AssembleModRMRegOp, &NativeX86Assembler::AssembleModRMRegOp, &NativeX86Assembler::AssembleRegModRMOp, &NativeX86Assembler::AssembleRegModRMOp,
		/*0x14*/ &NativeX86Assembler::AssembleImmOp<1, RIVER_OPSIZE_8>, &NativeX86Assembler::AssembleImmOp<1, RIVER_OPSIZE_32>, &NativeX86Assembler::AssembleNoOp, &NativeX86Assembler::AssembleNoOp,
		/*0x18*/ &NativeX86Assembler::AssembleModRMRegOp, &NativeX86Assembler::AssembleModRMRegOp, &NativeX86Assembler::AssembleRegModRMOp, &NativeX86Assembler::AssembleRegModRMOp,
		/*0x1C*/ &NativeX86Assembler::AssembleImmOp<1, RIVER_OPSIZE_8>, &NativeX86Assembler::AssembleImmOp<1, RIVER_OPSIZE_32>, &NativeX86Assembler::AssembleUnknownOp, &NativeX86Assembler::AssembleUnknownOp,

		/*0x20*/ &NativeX86Assembler::AssembleModRMRegOp, &NativeX86Assembler::AssembleModRMRegOp, &NativeX86Assembler::AssembleRegModRMOp, &NativeX86Assembler::AssembleRegModRMOp,
		/*0x24*/ &NativeX86Assembler::AssembleImmOp<1, RIVER_OPSIZE_8>, &NativeX86Assembler::AssembleImmOp<1, RIVER_OPSIZE_32>, &NativeX86Assembler::AssembleUnknownOp, &NativeX86Assembler::AssembleUnknownOp,
		/*0x28*/ &NativeX86Assembler::AssembleModRMRegOp, &NativeX86Assembler::AssembleModRMRegOp, &NativeX86Assembler::AssembleRegModRMOp, &NativeX86Assembler::AssembleRegModRMOp,
		/*0x2C*/ &NativeX86Assembler::AssembleImmOp<1, RIVER_OPSIZE_8>, &NativeX86Assembler::AssembleImmOp<1, RIVER_OPSIZE_32>, &NativeX86Assembler::AssembleUnknownOp, &NativeX86Assembler::AssembleUnknownOp,

		/*0x30*/ &NativeX86Assembler::AssembleModRMRegOp, &NativeX86Assembler::AssembleModRMRegOp, &NativeX86Assembler::AssembleRegModRMOp, &NativeX86Assembler::AssembleRegModRMOp,
		/*0x34*/ &NativeX86Assembler::AssembleImmOp<1, RIVER_OPSIZE_8>, &NativeX86Assembler::AssembleImmOp<1, RIVER_OPSIZE_32>, &NativeX86Assembler::AssembleUnknownOp, &NativeX86Assembler::AssembleUnknownOp,
		/*0x38*/ &NativeX86Assembler::AssembleModRMRegOp, &NativeX86Assembler::AssembleModRMRegOp, &NativeX86Assembler::AssembleRegModRMOp, &NativeX86Assembler::AssembleRegModRMOp,
		/*0x3C*/ &NativeX86Assembler::AssembleImmOp<1, RIVER_OPSIZE_8>, &NativeX86Assembler::AssembleImmOp<1, RIVER_OPSIZE_32>, &NativeX86Assembler::AssembleUnknownOp, &NativeX86Assembler::AssembleUnknownOp,

		/*0x40*/ &NativeX86Assembler::AssembleNoOp, &NativeX86Assembler::AssembleNoOp, &NativeX86Assembler::AssembleNoOp, &NativeX86Assembler::AssembleNoOp,
		/*0x44*/ &NativeX86Assembler::AssembleNoOp, &NativeX86Assembler::AssembleNoOp, &NativeX86Assembler::AssembleNoOp, &NativeX86Assembler::AssembleNoOp,
		/*0x48*/ &NativeX86Assembler::AssembleNoOp, &NativeX86Assembler::AssembleNoOp, &NativeX86Assembler::AssembleNoOp, &NativeX86Assembler::AssembleNoOp,
		/*0x4C*/ &NativeX86Assembler::AssembleNoOp, &NativeX86Assembler::AssembleNoOp, &NativeX86Assembler::AssembleNoOp, &NativeX86Assembler::AssembleNoOp,

		/*0x50*/ &NativeX86Assembler::AssembleNoOp, &NativeX86Assembler::AssembleNoOp, &NativeX86Assembler::AssembleNoOp, &NativeX86Assembler::AssembleNoOp,
		/*0x54*/ &NativeX86Assembler::AssembleNoOp, &NativeX86Assembler::AssembleNoOp, &NativeX86Assembler::AssembleNoOp, &NativeX86Assembler::AssembleNoOp,
		/*0x58*/ &NativeX86Assembler::AssembleNoOp, &NativeX86Assembler::AssembleNoOp, &NativeX86Assembler::AssembleNoOp, &NativeX86Assembler::AssembleNoOp,
		/*0x5C*/ &NativeX86Assembler::AssembleNoOp, &NativeX86Assembler::AssembleNoOp, &NativeX86Assembler::AssembleNoOp, &NativeX86Assembler::AssembleNoOp,

		/*0x60*/ &NativeX86Assembler::AssembleUnknownOp, &NativeX86Assembler::AssembleUnknownOp, &NativeX86Assembler::AssembleUnknownOp, &NativeX86Assembler::AssembleUnknownOp,
		/*0x64*/ &NativeX86Assembler::AssembleUnknownOp, &NativeX86Assembler::AssembleUnknownOp, &NativeX86Assembler::AssembleUnknownOp, &NativeX86Assembler::AssembleUnknownOp,
		/*0x68*/ &NativeX86Assembler::AssembleImmOp<0, RIVER_OPSIZE_32>, &NativeX86Assembler::AssembleRegModRMImm32Op, &NativeX86Assembler::AssembleImmOp<0, RIVER_OPSIZE_8>, &NativeX86Assembler::AssembleRegModRMImm8Op,
		/*0x6C*/ &NativeX86Assembler::AssembleUnknownOp, &NativeX86Assembler::AssembleUnknownOp, &NativeX86Assembler::AssembleUnknownOp, &NativeX86Assembler::AssembleUnknownOp,

		/*0x70*/ &NativeX86Assembler::AssembleNoOp, &NativeX86Assembler::AssembleNoOp, &NativeX86Assembler::AssembleNoOp, &NativeX86Assembler::AssembleNoOp,
		/*0x74*/ &NativeX86Assembler::AssembleNoOp, &NativeX86Assembler::AssembleNoOp, &NativeX86Assembler::AssembleNoOp, &NativeX86Assembler::AssembleNoOp,
		/*0x78*/ &NativeX86Assembler::AssembleNoOp, &NativeX86Assembler::AssembleNoOp, &NativeX86Assembler::AssembleNoOp, &NativeX86Assembler::AssembleNoOp,
		/*0x7C*/ &NativeX86Assembler::AssembleNoOp, &NativeX86Assembler::AssembleNoOp, &NativeX86Assembler::AssembleNoOp, &NativeX86Assembler::AssembleNoOp,

		/*0x80*/ &NativeX86Assembler::AssembleSubOpModRMImm8Op, &NativeX86Assembler::AssembleSubOpModRMImm32Op, &NativeX86Assembler::AssembleUnknownOp, &NativeX86Assembler::AssembleSubOpModRMImm8Op,
		/*0x84*/ &NativeX86Assembler::AssembleRegModRMOp, &NativeX86Assembler::AssembleRegModRMOp, &NativeX86Assembler::AssembleRegModRMOp, &NativeX86Assembler::AssembleRegModRMOp,
		/*0x88*/ &NativeX86Assembler::AssembleModRMRegOp, &NativeX86Assembler::AssembleModRMRegOp, &NativeX86Assembler::AssembleRegModRMOp, &NativeX86Assembler::AssembleRegModRMOp,
		/*0x8C*/ &NativeX86Assembler::AssembleUnknownOp, &NativeX86Assembler::AssembleRegModRMOp, &NativeX86Assembler::AssembleUnknownOp, &NativeX86Assembler::AssembleModRMOp<0>,

		/*0x90*/ &NativeX86Assembler::AssembleNoOp, &NativeX86Assembler::AssembleNoOp, &NativeX86Assembler::AssembleNoOp, &NativeX86Assembler::AssembleNoOp,
		/*0x94*/ &NativeX86Assembler::AssembleNoOp, &NativeX86Assembler::AssembleNoOp, &NativeX86Assembler::AssembleNoOp, &NativeX86Assembler::AssembleNoOp,
		/*0x98*/ &NativeX86Assembler::AssembleUnknownOp, &NativeX86Assembler::AssembleNoOp, &NativeX86Assembler::AssembleUnknownOp, &NativeX86Assembler::AssembleUnknownOp,
		/*0x9C*/ &NativeX86Assembler::AssembleNoOp, &NativeX86Assembler::AssembleNoOp, &NativeX86Assembler::AssembleUnknownOp, &NativeX86Assembler::AssembleUnknownOp,

		/*0xA0*/ &NativeX86Assembler::AssembleMoffs8<1>, &NativeX86Assembler::AssembleMoffs32<1>, &NativeX86Assembler::AssembleMoffs8<0>, &NativeX86Assembler::AssembleMoffs32<0>,
		/*0xA4*/ &NativeX86Assembler::AssembleNoOp, &NativeX86Assembler::AssembleNoOp, &NativeX86Assembler::AssembleNoOp, &NativeX86Assembler::AssembleNoOp,
		/*0xA8*/ &NativeX86Assembler::AssembleImmOp<1, RIVER_OPSIZE_8>, &NativeX86Assembler::AssembleImmOp<1, RIVER_OPSIZE_32>, &NativeX86Assembler::AssembleNoOp, &NativeX86Assembler::AssembleNoOp,
		/*0xAC*/ &NativeX86Assembler::AssembleUnknownOp, &NativeX86Assembler::AssembleUnknownOp, &NativeX86Assembler::AssembleNoOp, &NativeX86Assembler::AssembleNoOp,

		/*0xB0*/ &NativeX86Assembler::AssembleImmOp<1, RIVER_OPSIZE_8>, &NativeX86Assembler::AssembleImmOp<1, RIVER_OPSIZE_8>, &NativeX86Assembler::AssembleImmOp<1, RIVER_OPSIZE_8>, &NativeX86Assembler::AssembleImmOp<1, RIVER_OPSIZE_8>,
		/*0xB4*/ &NativeX86Assembler::AssembleImmOp<1, RIVER_OPSIZE_8>, &NativeX86Assembler::AssembleImmOp<1, RIVER_OPSIZE_8>, &NativeX86Assembler::AssembleImmOp<1, RIVER_OPSIZE_8>, &NativeX86Assembler::AssembleImmOp<1, RIVER_OPSIZE_8>,
		/*0xB8*/ &NativeX86Assembler::AssembleImmOp<1, RIVER_OPSIZE_32>, &NativeX86Assembler::AssembleImmOp<1, RIVER_OPSIZE_32>, &NativeX86Assembler::AssembleImmOp<1, RIVER_OPSIZE_32>, &NativeX86Assembler::AssembleImmOp<1, RIVER_OPSIZE_32>,
		/*0xBC*/ &NativeX86Assembler::AssembleImmOp<1, RIVER_OPSIZE_32>, &NativeX86Assembler::AssembleImmOp<1, RIVER_OPSIZE_32>, &NativeX86Assembler::AssembleImmOp<1, RIVER_OPSIZE_32>, &NativeX86Assembler::AssembleImmOp<1, RIVER_OPSIZE_32>,

		/*0xC0*/ &NativeX86Assembler::AssembleSubOpModRMImm8Op, &NativeX86Assembler::AssembleSubOpModRMImm8Op, &NativeX86Assembler::AssembleNoOp, &NativeX86Assembler::AssembleNoOp,
		/*0xC4*/ &NativeX86Assembler::AssembleUnknownOp, &NativeX86Assembler::AssembleUnknownOp, &NativeX86Assembler::AssembleModRMImm8Op, &NativeX86Assembler::AssembleModRMImm32Op,
		/*0xC8*/ &NativeX86Assembler::AssembleUnknownOp, &NativeX86Assembler::AssembleNoOp, &NativeX86Assembler::AssembleUnknownOp, &NativeX86Assembler::AssembleUnknownOp,
		/*0xCC*/ &NativeX86Assembler::AssembleUnknownOp, &NativeX86Assembler::AssembleUnknownOp, &NativeX86Assembler::AssembleUnknownOp, &NativeX86Assembler::AssembleUnknownOp,

		/*0xD0*/ &NativeX86Assembler::AssembleSubOpModRMOp, &NativeX86Assembler::AssembleSubOpModRMOp, &NativeX86Assembler::AssembleSubOpModRMOp, &NativeX86Assembler::AssembleSubOpModRMOp,
		/*0xD4*/ &NativeX86Assembler::AssembleUnknownOp, &NativeX86Assembler::AssembleUnknownOp, &NativeX86Assembler::AssembleUnknownOp, &NativeX86Assembler::AssembleUnknownOp,
		/*0xD8*/ &NativeX86Assembler::AssembleUnknownOp, &NativeX86Assembler::AssembleUnknownOp, &NativeX86Assembler::AssembleUnknownOp, &NativeX86Assembler::AssembleUnknownOp,
		/*0xDC*/ &NativeX86Assembler::AssembleUnknownOp, &NativeX86Assembler::AssembleUnknownOp, &NativeX86Assembler::AssembleUnknownOp, &NativeX86Assembler::AssembleUnknownOp,

		/*0xE0*/ &NativeX86Assembler::AssembleUnknownOp, &NativeX86Assembler::AssembleUnknownOp, &NativeX86Assembler::AssembleUnknownOp, &NativeX86Assembler::AssembleUnknownOp,
		/*0xE4*/ &NativeX86Assembler::AssembleUnknownOp, &NativeX86Assembler::AssembleUnknownOp, &NativeX86Assembler::AssembleUnknownOp, &NativeX86Assembler::AssembleUnknownOp,
		/*0xE8*/ &NativeX86Assembler::AssembleNoOp, &NativeX86Assembler::AssembleNoOp, &NativeX86Assembler::AssembleNoOp, &NativeX86Assembler::AssembleNoOp,
		/*0xEC*/ &NativeX86Assembler::AssembleUnknownOp, &NativeX86Assembler::AssembleUnknownOp, &NativeX86Assembler::AssembleUnknownOp, &NativeX86Assembler::AssembleUnknownOp,

		/*0xF0*/ &NativeX86Assembler::AssembleUnknownOp, &NativeX86Assembler::AssembleUnknownOp, &NativeX86Assembler::AssembleUnknownOp, &NativeX86Assembler::AssembleUnknownOp,
		/*0xF4*/ &NativeX86Assembler::AssembleUnknownOp, &NativeX86Assembler::AssembleUnknownOp, &NativeX86Assembler::AssembleSubOpOp<assemble0xF6Op>, &NativeX86Assembler::AssembleSubOpOp<assemble0xF7Op>,
		/*0xF8*/ &NativeX86Assembler::AssembleUnknownOp, &NativeX86Assembler::AssembleUnknownOp, &NativeX86Assembler::AssembleUnknownOp, &NativeX86Assembler::AssembleUnknownOp,
		/*0xFC*/ &NativeX86Assembler::AssembleNoOp, &NativeX86Assembler::AssembleNoOp, &NativeX86Assembler::AssembleSubOpModRMOp, &NativeX86Assembler::AssembleSubOpOp<assemble0xFFOp>
	}, {
		/*0x00*/ &NativeX86Assembler::AssembleUnknownOp, &NativeX86Assembler::AssembleUnknownOp, &NativeX86Assembler::AssembleRegModRMOp, &NativeX86Assembler::AssembleRegModRMOp,
		/*0x04*/ &NativeX86Assembler::AssembleUnknownOp, &NativeX86Assembler::AssembleUnknownOp, &NativeX86Assembler::AssembleUnknownOp, &NativeX86Assembler::AssembleUnknownOp,
		/*0x08*/ &NativeX86Assembler::AssembleUnknownOp, &NativeX86Assembler::AssembleUnknownOp, &NativeX86Assembler::AssembleUnknownOp, &NativeX86Assembler::AssembleUnknownOp,
		/*0x0C*/ &NativeX86Assembler::AssembleUnknownOp, &NativeX86Assembler::AssembleUnknownOp, &NativeX86Assembler::AssembleUnknownOp, &NativeX86Assembler::AssembleUnknownOp,

		/*0x10*/ &NativeX86Assembler::AssembleUnknownOp, &NativeX86Assembler::AssembleUnknownOp, &NativeX86Assembler::AssembleUnknownOp, &NativeX86Assembler::AssembleUnknownOp,
		/*0x14*/ &NativeX86Assembler::AssembleUnknownOp, &NativeX86Assembler::AssembleUnknownOp, &NativeX86Assembler::AssembleUnknownOp, &NativeX86Assembler::AssembleUnknownOp,
		/*0x18*/ &NativeX86Assembler::AssembleUnknownOp, &NativeX86Assembler::AssembleUnknownOp, &NativeX86Assembler::AssembleUnknownOp, &NativeX86Assembler::AssembleUnknownOp,
		/*0x1C*/ &NativeX86Assembler::AssembleUnknownOp, &NativeX86Assembler::AssembleUnknownOp, &NativeX86Assembler::AssembleUnknownOp, &NativeX86Assembler::AssembleUnknownOp,

		/*0x20*/ &NativeX86Assembler::AssembleUnknownOp, &NativeX86Assembler::AssembleUnknownOp, &NativeX86Assembler::AssembleUnknownOp, &NativeX86Assembler::AssembleUnknownOp,
		/*0x24*/ &NativeX86Assembler::AssembleUnknownOp, &NativeX86Assembler::AssembleUnknownOp, &NativeX86Assembler::AssembleUnknownOp, &NativeX86Assembler::AssembleUnknownOp,
		/*0x28*/ &NativeX86Assembler::AssembleUnknownOp, &NativeX86Assembler::AssembleUnknownOp, &NativeX86Assembler::AssembleUnknownOp, &NativeX86Assembler::AssembleUnknownOp,
		/*0x2C*/ &NativeX86Assembler::AssembleUnknownOp, &NativeX86Assembler::AssembleUnknownOp, &NativeX86Assembler::AssembleUnknownOp, &NativeX86Assembler::AssembleUnknownOp,

		/*0x30*/ &NativeX86Assembler::AssembleUnknownOp, &NativeX86Assembler::AssembleNoOp, &NativeX86Assembler::AssembleUnknownOp, &NativeX86Assembler::AssembleUnknownOp,
		/*0x34*/ &NativeX86Assembler::AssembleNoOp, &NativeX86Assembler::AssembleUnknownOp, &NativeX86Assembler::AssembleUnknownOp, &NativeX86Assembler::AssembleUnknownOp,
		/*0x38*/ &NativeX86Assembler::AssembleUnknownOp, &NativeX86Assembler::AssembleUnknownOp, &NativeX86Assembler::AssembleUnknownOp, &NativeX86Assembler::AssembleUnknownOp,
		/*0x3C*/ &NativeX86Assembler::AssembleUnknownOp, &NativeX86Assembler::AssembleUnknownOp, &NativeX86Assembler::AssembleUnknownOp, &NativeX86Assembler::AssembleUnknownOp,

		/*0x40*/ &NativeX86Assembler::AssembleRegModRMOp, &NativeX86Assembler::AssembleRegModRMOp, &NativeX86Assembler::AssembleRegModRMOp, &NativeX86Assembler::AssembleRegModRMOp,
		/*0x44*/ &NativeX86Assembler::AssembleRegModRMOp, &NativeX86Assembler::AssembleRegModRMOp, &NativeX86Assembler::AssembleRegModRMOp, &NativeX86Assembler::AssembleRegModRMOp,
		/*0x48*/ &NativeX86Assembler::AssembleRegModRMOp, &NativeX86Assembler::AssembleRegModRMOp, &NativeX86Assembler::AssembleRegModRMOp, &NativeX86Assembler::AssembleRegModRMOp,
		/*0x4C*/ &NativeX86Assembler::AssembleRegModRMOp, &NativeX86Assembler::AssembleRegModRMOp, &NativeX86Assembler::AssembleRegModRMOp, &NativeX86Assembler::AssembleRegModRMOp,

		/*0x50*/ &NativeX86Assembler::AssembleUnknownOp, &NativeX86Assembler::AssembleUnknownOp, &NativeX86Assembler::AssembleUnknownOp, &NativeX86Assembler::AssembleUnknownOp,
		/*0x54*/ &NativeX86Assembler::AssembleUnknownOp, &NativeX86Assembler::AssembleUnknownOp, &NativeX86Assembler::AssembleUnknownOp, &NativeX86Assembler::AssembleUnknownOp,
		/*0x58*/ &NativeX86Assembler::AssembleUnknownOp, &NativeX86Assembler::AssembleUnknownOp, &NativeX86Assembler::AssembleUnknownOp, &NativeX86Assembler::AssembleUnknownOp,
		/*0x5C*/ &NativeX86Assembler::AssembleUnknownOp, &NativeX86Assembler::AssembleUnknownOp, &NativeX86Assembler::AssembleUnknownOp, &NativeX86Assembler::AssembleUnknownOp,

		/*0x60*/ &NativeX86Assembler::AssembleUnknownOp, &NativeX86Assembler::AssembleUnknownOp, &NativeX86Assembler::AssembleUnknownOp, &NativeX86Assembler::AssembleUnknownOp,
		/*0x64*/ &NativeX86Assembler::AssembleUnknownOp, &NativeX86Assembler::AssembleUnknownOp, &NativeX86Assembler::AssembleUnknownOp, &NativeX86Assembler::AssembleUnknownOp,
		/*0x68*/ &NativeX86Assembler::AssembleUnknownOp, &NativeX86Assembler::AssembleUnknownOp, &NativeX86Assembler::AssembleUnknownOp, &NativeX86Assembler::AssembleUnknownOp,
		/*0x6C*/ &NativeX86Assembler::AssembleUnknownOp, &NativeX86Assembler::AssembleUnknownOp, &NativeX86Assembler::AssembleUnknownOp, &NativeX86Assembler::AssembleUnknownOp,

		/*0x70*/ &NativeX86Assembler::AssembleUnknownOp, &NativeX86Assembler::AssembleUnknownOp, &NativeX86Assembler::AssembleUnknownOp, &NativeX86Assembler::AssembleUnknownOp,
		/*0x74*/ &NativeX86Assembler::AssembleUnknownOp, &NativeX86Assembler::AssembleUnknownOp, &NativeX86Assembler::AssembleUnknownOp, &NativeX86Assembler::AssembleUnknownOp,
		/*0x78*/ &NativeX86Assembler::AssembleUnknownOp, &NativeX86Assembler::AssembleUnknownOp, &NativeX86Assembler::AssembleUnknownOp, &NativeX86Assembler::AssembleUnknownOp,
		/*0x7C*/ &NativeX86Assembler::AssembleUnknownOp, &NativeX86Assembler::AssembleUnknownOp, &NativeX86Assembler::AssembleUnknownOp, &NativeX86Assembler::AssembleUnknownOp,

		/*0x80*/ &NativeX86Assembler::AssembleNoOp, &NativeX86Assembler::AssembleNoOp, &NativeX86Assembler::AssembleNoOp, &NativeX86Assembler::AssembleNoOp,
		/*0x84*/ &NativeX86Assembler::AssembleNoOp, &NativeX86Assembler::AssembleNoOp, &NativeX86Assembler::AssembleNoOp, &NativeX86Assembler::AssembleNoOp,
		/*0x88*/ &NativeX86Assembler::AssembleNoOp, &NativeX86Assembler::AssembleNoOp, &NativeX86Assembler::AssembleNoOp, &NativeX86Assembler::AssembleNoOp,
		/*0x8C*/ &NativeX86Assembler::AssembleNoOp, &NativeX86Assembler::AssembleNoOp, &NativeX86Assembler::AssembleNoOp, &NativeX86Assembler::AssembleNoOp,

		/*0x90*/ &NativeX86Assembler::AssembleModRMOp<0>, &NativeX86Assembler::AssembleModRMOp<0>, &NativeX86Assembler::AssembleModRMOp<0>, &NativeX86Assembler::AssembleModRMOp<0>,
		/*0x94*/ &NativeX86Assembler::AssembleModRMOp<0>, &NativeX86Assembler::AssembleModRMOp<0>, &NativeX86Assembler::AssembleModRMOp<0>, &NativeX86Assembler::AssembleModRMOp<0>,
		/*0x98*/ &NativeX86Assembler::AssembleModRMOp<0>, &NativeX86Assembler::AssembleModRMOp<0>, &NativeX86Assembler::AssembleModRMOp<0>, &NativeX86Assembler::AssembleModRMOp<0>,
		/*0x9C*/ &NativeX86Assembler::AssembleModRMOp<0>, &NativeX86Assembler::AssembleModRMOp<0>, &NativeX86Assembler::AssembleModRMOp<0>, &NativeX86Assembler::AssembleModRMOp<0>,

		/*0xA0*/ &NativeX86Assembler::AssembleUnknownOp, &NativeX86Assembler::AssembleUnknownOp, &NativeX86Assembler::AssembleNoOp, &NativeX86Assembler::AssembleUnknownOp,
		/*0xA4*/ &NativeX86Assembler::AssembleModRMRegImm8Op, &NativeX86Assembler::AssembleModRMRegOp, &NativeX86Assembler::AssembleUnknownOp, &NativeX86Assembler::AssembleUnknownOp,
		/*0xA8*/ &NativeX86Assembler::AssembleUnknownOp, &NativeX86Assembler::AssembleUnknownOp, &NativeX86Assembler::AssembleUnknownOp, &NativeX86Assembler::AssembleUnknownOp,
		/*0xAC*/ &NativeX86Assembler::AssembleModRMRegImm8Op, &NativeX86Assembler::AssembleModRMRegOp, &NativeX86Assembler::AssembleUnknownOp, &NativeX86Assembler::AssembleRegModRMOp,

		/*0xB0*/ &NativeX86Assembler::AssembleModRMRegOp, &NativeX86Assembler::AssembleModRMRegOp, &NativeX86Assembler::AssembleUnknownOp, &NativeX86Assembler::AssembleUnknownOp,
		/*0xB4*/ &NativeX86Assembler::AssembleUnknownOp, &NativeX86Assembler::AssembleUnknownOp, &NativeX86Assembler::AssembleRegModRMOp, &NativeX86Assembler::AssembleRegModRMOp,
		/*0xB8*/ &NativeX86Assembler::AssembleUnknownOp, &NativeX86Assembler::AssembleUnknownOp, &NativeX86Assembler::AssembleSubOpModRMImm8Op, &NativeX86Assembler::AssembleUnknownOp,
		/*0xBC*/ &NativeX86Assembler::AssembleRegModRMOp, &NativeX86Assembler::AssembleRegModRMOp, &NativeX86Assembler::AssembleRegModRMOp, &NativeX86Assembler::AssembleRegModRMOp,

		/*0xC0*/ &NativeX86Assembler::AssembleModRMRegOp, &NativeX86Assembler::AssembleModRMRegOp, &NativeX86Assembler::AssembleUnknownOp, &NativeX86Assembler::AssembleUnknownOp,
		/*0xC4*/ &NativeX86Assembler::AssembleUnknownOp, &NativeX86Assembler::AssembleUnknownOp, &NativeX86Assembler::AssembleUnknownOp, &NativeX86Assembler::AssembleSubOpOp<NativeX86Assembler::assemble0x0FC7Op>,
		/*0xC8*/ &NativeX86Assembler::AssembleUnknownOp, &NativeX86Assembler::AssembleUnknownOp, &NativeX86Assembler::AssembleUnknownOp, &NativeX86Assembler::AssembleUnknownOp,
		/*0xCC*/ &NativeX86Assembler::AssembleUnknownOp, &NativeX86Assembler::AssembleUnknownOp, &NativeX86Assembler::AssembleUnknownOp, &NativeX86Assembler::AssembleUnknownOp,

		/*0xD0*/ &NativeX86Assembler::AssembleUnknownOp, &NativeX86Assembler::AssembleUnknownOp, &NativeX86Assembler::AssembleUnknownOp, &NativeX86Assembler::AssembleUnknownOp,
		/*0xD4*/ &NativeX86Assembler::AssembleUnknownOp, &NativeX86Assembler::AssembleUnknownOp, &NativeX86Assembler::AssembleUnknownOp, &NativeX86Assembler::AssembleUnknownOp,
		/*0xD8*/ &NativeX86Assembler::AssembleUnknownOp, &NativeX86Assembler::AssembleUnknownOp, &NativeX86Assembler::AssembleUnknownOp, &NativeX86Assembler::AssembleUnknownOp,
		/*0xDC*/ &NativeX86Assembler::AssembleUnknownOp, &NativeX86Assembler::AssembleUnknownOp, &NativeX86Assembler::AssembleUnknownOp, &NativeX86Assembler::AssembleUnknownOp,

		/*0xE0*/ &NativeX86Assembler::AssembleUnknownOp, &NativeX86Assembler::AssembleUnknownOp, &NativeX86Assembler::AssembleUnknownOp, &NativeX86Assembler::AssembleUnknownOp,
		/*0xE4*/ &NativeX86Assembler::AssembleUnknownOp, &NativeX86Assembler::AssembleUnknownOp, &NativeX86Assembler::AssembleUnknownOp, &NativeX86Assembler::AssembleUnknownOp,
		/*0xE8*/ &NativeX86Assembler::AssembleUnknownOp, &NativeX86Assembler::AssembleUnknownOp, &NativeX86Assembler::AssembleUnknownOp, &NativeX86Assembler::AssembleUnknownOp,
		/*0xEC*/ &NativeX86Assembler::AssembleUnknownOp, &NativeX86Assembler::AssembleUnknownOp, &NativeX86Assembler::AssembleUnknownOp, &NativeX86Assembler::AssembleUnknownOp,

		/*0xF0*/ &NativeX86Assembler::AssembleUnknownOp, &NativeX86Assembler::AssembleUnknownOp, &NativeX86Assembler::AssembleUnknownOp, &NativeX86Assembler::AssembleUnknownOp,
		/*0xF4*/ &NativeX86Assembler::AssembleUnknownOp, &NativeX86Assembler::AssembleUnknownOp, &NativeX86Assembler::AssembleUnknownOp, &NativeX86Assembler::AssembleUnknownOp,
		/*0xF8*/ &NativeX86Assembler::AssembleUnknownOp, &NativeX86Assembler::AssembleUnknownOp, &NativeX86Assembler::AssembleUnknownOp, &NativeX86Assembler::AssembleUnknownOp,
		/*0xFC*/ &NativeX86Assembler::AssembleUnknownOp, &NativeX86Assembler::AssembleUnknownOp, &NativeX86Assembler::AssembleUnknownOp, &NativeX86Assembler::AssembleUnknownOp
	}
};
