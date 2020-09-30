#include "cb.h"
#include "modrm32.h"
#include "codetbl.h"
#include "extern.h"

extern unsigned int dwSysHandler; // = 0; // &SysHandler
extern unsigned int dwSysEndHandler; // = 0; // &SysEndHandler
extern unsigned int dwBranchHandler; // = 0; // &BranchHandler


const char pSysOut[] = {
	0x87, 0x25, 0x00, 0x00, 0x00, 0x00,			// 0x00 - xchg esp, large ds:<dwVirtualStack>
	0x9C, 										// 0x06 - pushf
	0x60,										// 0x07 - pusha
	0x68, 0x46, 0x02, 0x00, 0x00,				// 0x08 - push 0x00000246 - NEW FLAGS
	0x9D,										// 0x0D - popf
	0x68, 0x00, 0x00, 0x00, 0x00,				// 0x0E - push <execution_environment>
	0xFF, 0x15, 0x00, 0x00, 0x00, 0x00,         // 0x13 - call <dwSysEndHandler>
	0x58,										// 0x19 - pop eax // pop the execution environment off the stack
	0x61,                             			// 0x1A - popa
	0x9D,                                       // 0x1B - popf
	0x87, 0x25, 0x00, 0x00, 0x00, 0x00          // 0x1C - xchg esp, large ds:<dwVirtualStack>
};

unsigned int AddSysEndPrefix(struct _exec_env *pEnv, char *pCurr) {
	memcpy(pCurr, pSysOut, sizeof(pSysOut));

	*(unsigned int *)(&pCurr[0x02]) = (unsigned int)&pEnv->virtualStack;
	*(unsigned int *)(&pCurr[0x0F]) = (unsigned int)pEnv;
	*(unsigned int *)(&pCurr[0x15]) = (unsigned int)&dwSysEndHandler;
	*(unsigned int *)(&pCurr[0x1E]) = (unsigned int)&pEnv->virtualStack;

	return sizeof(pSysOut);
}


unsigned int __stdcall HandleSingle(struct _exec_env *pEnv, struct _cb_info *pCB, unsigned int *dwFlags, char *pI, char *pD, unsigned int *dwWritten) {
	pD[0] = pI[0];
	return *dwWritten = 1;
}

unsigned int __stdcall HandleDouble(struct _exec_env *pEnv, struct _cb_info *pCB, unsigned int *dwFlags, char *pI, char *pD, unsigned int *dwWritten) {
	pD[0] = pI[0];
	pD[1] = pI[1];
	*dwWritten = 2;
	return 0;
}

unsigned int __stdcall FuncImm8(struct _exec_env *pEnv, struct _cb_info *pCB, unsigned int *dwFlags, char *pI, char *pD, unsigned int *dwWritten) {
	pD[0] = pI[0];
	pD[1] = pI[1];

	return *dwWritten = 2;
}

unsigned int __stdcall FuncImm32(struct _exec_env *pEnv, struct _cb_info *pCB, unsigned int *dwFlags, char *pI, char *pD, unsigned int *dwWritten) {
	pD[0] = pI[0];
	pD[1] = pI[1];
	pD[2] = pI[2];
	
	if ((*dwFlags & FLAG_O16) == 0) {
		pD[3] = pI[3];
		pD[4] = pI[4];
		return *dwWritten = 5;
	}

	return *dwWritten = 3;
}

unsigned int __stdcall FuncExt(struct _exec_env *pEnv, struct _cb_info *pCB, unsigned int *dwFlags, char *pI, char *pD, unsigned int *dwWritten) {
	pD[0] = pI[0];
	*dwFlags |= FLAG_EXT | FLAG_PFX;
	return *dwWritten = 1;
}

unsigned int __stdcall FuncSeg(struct _exec_env *pEnv, struct _cb_info *pCB, unsigned int *dwFlags, char *pI, char *pD, unsigned int *dwWritten) {
	pD[0] = pI[0];
	*dwFlags |= FLAG_SEG | FLAG_PFX;
	return *dwWritten = 1;
}

unsigned int __stdcall FuncLock(struct _exec_env *pEnv, struct _cb_info *pCB, unsigned int *dwFlags, char *pI, char *pD, unsigned int *dwWritten) {
	pD[0] = pI[0];
	*dwFlags |= FLAG_LOCK | FLAG_PFX;
	return *dwWritten = 1;
}

unsigned int __stdcall FuncRep(struct _exec_env *pEnv, struct _cb_info *pCB, unsigned int *dwFlags, char *pI, char *pD, unsigned int *dwWritten) {
	pD[0] = pI[0];
	*dwFlags |= FLAG_REP | FLAG_PFX;
	return *dwWritten = 1;
}


unsigned int __stdcall FuncO16(struct _exec_env *pEnv, struct _cb_info *pCB, unsigned int *dwFlags, char *pI, char *pD, unsigned int *dwWritten) {
	pD[0] = pI[0];

	/*if ((*dwFlags & FLAG_O16) == FLAG_O16) {
		
	}*/
	*dwFlags ^= FLAG_O16;
	*dwFlags |= FLAG_PFX;

	return *dwWritten = 1;
}

unsigned int __stdcall FuncA16(struct _exec_env *pEnv, struct _cb_info *pCB, unsigned int *dwFlags, char *pI, char *pD, unsigned int *dwWritten) {
	pD[0] = pI[0];

	/*if ((*dwFlags & FLAG_O16) == FLAG_O16) {
		
	}*/
	*dwFlags ^= FLAG_A16;
	*dwFlags |= FLAG_PFX;

	return *dwWritten = 1;
}

unsigned int __stdcall FuncModrm(struct _exec_env *pEnv, struct _cb_info *pCB, unsigned int *dwFlags, char *pI, char *pD, unsigned int *dwWritten) {
	unsigned int size;

	pD[0] = pI[0];
	pD[1] = pI[1];

	size = GetModrmSize(*dwFlags, pI);
	if (size) {
		memcpy(pD + 2, pI + 2, size);
	}

	return *dwWritten = size + 2;
}

unsigned int __stdcall ModrmImm8(struct _exec_env *pEnv, struct _cb_info *pCB, unsigned int *dwFlags, char *pI, char *pD, unsigned int *dwWritten) {
	unsigned int size;

	pD[0] = pI[0];
	pD[1] = pI[1];

	size = GetModrmSize(*dwFlags, pI);
	if (size) {
		memcpy(pD + 2, pI + 2, size);
	}

	pD[2 + size] = pI[2 + size];
	return *dwWritten = 2 + size + 1;

}

unsigned int __stdcall ModrmImm32(struct _exec_env *pEnv, struct _cb_info *pCB, unsigned int *dwFlags, char *pI, char *pD, unsigned int *dwWritten) {
	unsigned int size;

	pD[0] = pI[0];
	pD[1] = pI[1];

	size = GetModrmSize(*dwFlags, pI);
	if (size) {
		memcpy(pD + 2, pI + 2, size);
	}

	
	pI += 2 + size;
	pD += 2 + size;

	pD[0] = pI[0];
	pD[1] = pI[1];
	if ((*dwFlags & FLAG_O16) == 0) {
		pD[2] = pI[2];
		pD[3] = pI[3];
		return *dwWritten = 2 + size + 4;		
	}

	return *dwWritten = 2 + size + 2;

}

const char pBranchJCC[] = {
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

unsigned int __stdcall FuncJCC_B(struct _exec_env *pEnv, struct _cb_info *pCB, unsigned int *dwFlags, char *pI, char *pD, unsigned int *dwWritten) {
	char *pCurr = pD;

	unsigned int addrFallThrough = (unsigned int)(pI + 2);
	unsigned int addrJump = (unsigned int)(pI + pI[1] + 2);

	pD[0] = pI[0];
	pD[1] = sizeof(pBranchJCC);

	pCurr += 2;
	memcpy(pCurr, pBranchJCC, sizeof(pBranchJCC));

	*(unsigned int *)(&pCurr[0x02]) = (unsigned int)&pEnv->virtualStack;
	*(unsigned int *)(&pCurr[0x0F]) = addrFallThrough;
	*(unsigned int *)(&pCurr[0x14]) = (unsigned int)pEnv;
	*(unsigned int *)(&pCurr[0x1A]) = (unsigned int)&dwBranchHandler;
	*(unsigned int *)(&pCurr[0x22]) = (unsigned int)&pEnv->virtualStack;
	*(unsigned int *)(&pCurr[0x28]) = (unsigned int)&pEnv->jumpBuff;
	pCurr += sizeof(pBranchJCC);

	
	memcpy(pCurr, pBranchJCC, sizeof(pBranchJCC));

	*(unsigned int *)(&pCurr[0x02]) = (unsigned int)&pEnv->virtualStack;
	*(unsigned int *)(&pCurr[0x0F]) = addrJump;
	*(unsigned int *)(&pCurr[0x14]) = (unsigned int)pEnv;
	*(unsigned int *)(&pCurr[0x1A]) = (unsigned int)&dwBranchHandler;
	*(unsigned int *)(&pCurr[0x22]) = (unsigned int)&pEnv->virtualStack;
	*(unsigned int *)(&pCurr[0x28]) = (unsigned int)&pEnv->jumpBuff;
	pCurr += sizeof(pBranchJCC);

	*dwWritten = pCurr - pD;
	*dwFlags |= FLAG_BRANCH;
	return 2;
}


unsigned int __stdcall FuncJCC_D(struct _exec_env *pEnv, struct _cb_info *pCB, unsigned int *dwFlags, char *pI, char *pD, unsigned int *dwWritten) {
	char *pCurr = pD;

	unsigned int addrFallThrough = (unsigned int)(pI + 5);
	unsigned int addrJump = (unsigned int)(pI + *(unsigned int *)&pI[1] + 5);

	pD[0] = pI[0];
	*(unsigned int *)(&pD[1]) = sizeof(pBranchJCC);

	pCurr += 5;
	memcpy(pCurr, pBranchJCC, sizeof(pBranchJCC));

	*(unsigned int *)(&pCurr[0x02]) = (unsigned int)&pEnv->virtualStack;
	*(unsigned int *)(&pCurr[0x0F]) = addrFallThrough;
	*(unsigned int *)(&pCurr[0x14]) = (unsigned int)pEnv;
	*(unsigned int *)(&pCurr[0x1A]) = (unsigned int)&dwBranchHandler;
	*(unsigned int *)(&pCurr[0x22]) = (unsigned int)&pEnv->virtualStack;
	*(unsigned int *)(&pCurr[0x28]) = (unsigned int)&pEnv->jumpBuff;
	pCurr += sizeof(pBranchJCC);

	
	memcpy(pCurr, pBranchJCC, sizeof(pBranchJCC));

	*(unsigned int *)(&pCurr[0x02]) = (unsigned int)&pEnv->virtualStack;
	*(unsigned int *)(&pCurr[0x0F]) = addrJump;
	*(unsigned int *)(&pCurr[0x14]) = (unsigned int)pEnv;
	*(unsigned int *)(&pCurr[0x1A]) = (unsigned int)&dwBranchHandler;
	*(unsigned int *)(&pCurr[0x22]) = (unsigned int)&pEnv->virtualStack;
	*(unsigned int *)(&pCurr[0x28]) = (unsigned int)&pEnv->jumpBuff;
	pCurr += sizeof(pBranchJCC);

	*dwWritten = pCurr - pD;
	*dwFlags |= FLAG_BRANCH;
	return 5;
}

const char pBranchJMP[] = {
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

unsigned int __stdcall FuncJMP_B(struct _exec_env *pEnv, struct _cb_info *pCB, unsigned int *dwFlags, char *pI, char *pD, unsigned int *dwWritten) {
	unsigned int addrJump = (unsigned int)(pI + pI[1] + 2);

	memcpy(pD, pBranchJMP, sizeof(pBranchJMP));
	*(unsigned int *)(&pD[0x02]) = (unsigned int)&pEnv->virtualStack;
	*(unsigned int *)(&pD[0x0F]) = addrJump;
	*(unsigned int *)(&pD[0x14]) = (unsigned int)pEnv;
	*(unsigned int *)(&pD[0x1A]) = (unsigned int)&dwBranchHandler;
	*(unsigned int *)(&pD[0x22]) = (unsigned int)&pEnv->virtualStack;
	*(unsigned int *)(&pD[0x28]) = (unsigned int)&pEnv->jumpBuff;

	*dwWritten = sizeof(pBranchJMP);
	*dwFlags |= FLAG_BRANCH;
	return 2;
}

unsigned int __stdcall FuncJMP_D(struct _exec_env *pEnv, struct _cb_info *pCB, unsigned int *dwFlags, char *pI, char *pD, unsigned int *dwWritten) {
	unsigned int addrJump = (unsigned int)(pI + *(unsigned int *)(&pI[1]) + 5);

	memcpy(pD, pBranchJMP, sizeof(pBranchJMP));
	*(unsigned int *)(&pD[0x02]) = (unsigned int)&pEnv->virtualStack;
	*(unsigned int *)(&pD[0x0F]) = addrJump;
	*(unsigned int *)(&pD[0x14]) = (unsigned int)pEnv;
	*(unsigned int *)(&pD[0x1A]) = (unsigned int)&dwBranchHandler;
	*(unsigned int *)(&pD[0x22]) = (unsigned int)&pEnv->virtualStack;
	*(unsigned int *)(&pD[0x28]) = (unsigned int)&pEnv->jumpBuff;

	*dwWritten = sizeof(pBranchJMP);
	*dwFlags |= FLAG_BRANCH;
	return 5;
}

unsigned int __stdcall FarPointer(struct _exec_env *pEnv, struct _cb_info *pCB, unsigned int *dwFlags, char *pI, char *pD, unsigned int *dwWritten) {
	pD[0] = pI[0];
	pD[1] = pI[1];
	pD[2] = pI[2];
	pD[3] = pI[3];
	pD[4] = pI[4];

	if ((*dwFlags & FLAG_A16) == 0) {
		pD[5] = pI[5];
		pD[6] = pI[6];
		return *dwWritten = 7;
	}

	return *dwWritten = 5;
}

unsigned int __stdcall FuncAddr32(struct _exec_env *pEnv, struct _cb_info *pCB, unsigned int *dwFlags, char *pI, char *pD, unsigned int *dwWritten) {
	pD[0] = pI[0];
	pD[1] = pI[1];
	pD[2] = pI[2];

	if ((*dwFlags & FLAG_A16) == 0) {
		pD[3] = pI[3];
		pD[4] = pI[4];
		return *dwWritten = 5;
	}

	return *dwWritten = 3;
}

//RetImm - copy the value
//Retn - 0
//RetFar - 4
const char pBranchRet[] = {
	0xA3, 0x00, 0x00, 0x00, 0x00,				// 0x00 - mov [<dwEaxSave>], eax
	0x58,										// 0x05 - pop eax
	0x87, 0x25, 0x00, 0x00, 0x00, 0x00,			// 0x06 - xchg esp, large ds:<dwVirtualStack>
	0x9C,			 							// 0x0C - pushf
	0x60,										// 0x0D - pusha
	0x68, 0x46, 0x02, 0x00, 0x00,				// 0x0E - push 0x00000246 - NEW FLAGS
	0x9D,										// 0x13 - popf
	0x50,										// 0x14 - push eax
	0x68, 0x00, 0x00, 0x00, 0x00,				// 0x15 - push <execution_environment>
	0xFF, 0x15, 0x00, 0x00, 0x00, 0x00,			// 0x1A - call <branch_handler>

	0x61,										// 0x20 - popa
	0x9D,										// 0x21 - popf
	0x87, 0x25, 0x00, 0x00, 0x00, 0x00,			// 0x22 - xchg esp, large ds:<dwVirtualStack>
	0xA1, 0x00, 0x00, 0x00, 0x00,				// 0x28 - mov eax, large ds:<dwEaxSave>
	0x8D, 0xA4, 0x24, 0x00, 0x00, 0x00, 0x00,   // 0x2D - lea esp, [esp + <pI>] // probably sub esp, xxx
	0xFF, 0x25, 0x00, 0x00, 0x00, 0x00			// 0x34 - jmp large dword ptr ds:<jumpbuff>
};

unsigned int __stdcall FuncCRetImm(struct _exec_env *pEnv, struct _cb_info *pCB, unsigned int *dwFlags, char *pI, char *pD, unsigned int *dwWritten) {
	unsigned short stackSpace = *(unsigned short *)(&pI[1]);
	memcpy(pD, pBranchRet, sizeof(pBranchRet));	

	*(unsigned int *)(&pD[0x01]) = (unsigned int)&pEnv->returnRegister;
	*(unsigned int *)(&pD[0x08]) = (unsigned int)&pEnv->virtualStack;
	*(unsigned int *)(&pD[0x16]) = (unsigned int)pEnv;
	*(unsigned int *)(&pD[0x1C]) = (unsigned int)&dwBranchHandler;
	*(unsigned int *)(&pD[0x24]) = (unsigned int)&pEnv->virtualStack;
	*(unsigned int *)(&pD[0x29]) = (unsigned int)&pEnv->returnRegister;
	*(unsigned int *)(&pD[0x30]) = stackSpace;
	*(unsigned int *)(&pD[0x36]) = (unsigned int)&pEnv->jumpBuff;

	*dwWritten = sizeof(pBranchRet);
	*dwFlags |= FLAG_BRANCH;
	return 3;
}

unsigned int __stdcall FuncCRetn(struct _exec_env *pEnv, struct _cb_info *pCB, unsigned int *dwFlags, char *pI, char *pD, unsigned int *dwWritten) {
	unsigned short stackSpace = 0;
	memcpy(pD, pBranchRet, sizeof(pBranchRet));	

	*(unsigned int *)(&pD[0x01]) = (unsigned int)&pEnv->returnRegister;
	*(unsigned int *)(&pD[0x08]) = (unsigned int)&pEnv->virtualStack;
	*(unsigned int *)(&pD[0x16]) = (unsigned int)pEnv;
	*(unsigned int *)(&pD[0x1C]) = (unsigned int)&dwBranchHandler;
	*(unsigned int *)(&pD[0x24]) = (unsigned int)&pEnv->virtualStack;
	*(unsigned int *)(&pD[0x29]) = (unsigned int)&pEnv->returnRegister;
	*(unsigned int *)(&pD[0x30]) = stackSpace;
	*(unsigned int *)(&pD[0x36]) = (unsigned int)&pEnv->jumpBuff;

	*dwWritten = sizeof(pBranchRet);
	*dwFlags |= FLAG_BRANCH;
	return 1;
}

unsigned int __stdcall FuncCRetFar(struct _exec_env *pEnv, struct _cb_info *pCB, unsigned int *dwFlags, char *pI, char *pD, unsigned int *dwWritten) {
	unsigned short stackSpace = 4;
	memcpy(pD, pBranchRet, sizeof(pBranchRet));	

	*(unsigned int *)(&pD[0x01]) = (unsigned int)&pEnv->returnRegister;
	*(unsigned int *)(&pD[0x08]) = (unsigned int)&pEnv->virtualStack;
	*(unsigned int *)(&pD[0x16]) = (unsigned int)pEnv;
	*(unsigned int *)(&pD[0x1C]) = (unsigned int)&dwBranchHandler;
	*(unsigned int *)(&pD[0x24]) = (unsigned int)&pEnv->virtualStack;
	*(unsigned int *)(&pD[0x29]) = (unsigned int)&pEnv->returnRegister;
	*(unsigned int *)(&pD[0x30]) = stackSpace;
	*(unsigned int *)(&pD[0x36]) = (unsigned int)&pEnv->jumpBuff;

	*dwWritten = sizeof(pBranchRet);
	*dwFlags |= FLAG_BRANCH;
	return 3;
}

unsigned int __stdcall FuncEnter(struct _exec_env *pEnv, struct _cb_info *pCB, unsigned int *dwFlags, char *pI, char *pD, unsigned int *dwWritten) {
	pD[0] = pI[0];
	pD[1] = pI[1];
	pD[2] = pI[2];
	pD[3] = pI[3];
	return *dwWritten = 4;
}

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

unsigned int __stdcall FuncCall(struct _exec_env *pEnv, struct _cb_info *pCB, unsigned int *dwFlags, char *pI, char *pD, unsigned int *dwWritten) {
	unsigned int jumpAddr = (unsigned int)(pI + *(unsigned int *)(&pI[1]) + 5);
	unsigned int retAddr = (unsigned int)(&pI[5]);

	memcpy(pD, pBranchCall, sizeof(pBranchCall));
	*(unsigned int *)(&pD[0x01]) = retAddr;
	*(unsigned int *)(&pD[0x07]) = (unsigned int)&pEnv->virtualStack;
	*(unsigned int *)(&pD[0x14]) = jumpAddr;
	*(unsigned int *)(&pD[0x19]) = (unsigned int)pEnv;
	*(unsigned int *)(&pD[0x1F]) = (unsigned int)&dwBranchHandler;
	*(unsigned int *)(&pD[0x27]) = (unsigned int)&pEnv->virtualStack;
	*(unsigned int *)(&pD[0x2D]) = (unsigned int)&pEnv->jumpBuff;

	*dwWritten = sizeof(pBranchCall);
	*dwFlags |= FLAG_BRANCH;
	return 5;
}


unsigned int __stdcall DefaultBranch(struct _exec_env *pEnv, struct _cb_info *pCB, unsigned int *dwFlags, char *pI, char *pD, unsigned int *dwWritten) {
	DbgPrint("DefaultBranch: %02X.\n", pI);
	return 0;
}

unsigned int __stdcall DefaultFunc(struct _exec_env *pEnv, struct _cb_info *pCB, unsigned int *dwFlags, char *pI, char *pD, unsigned int *dwWritten) {
	DbgPrint("DefaultFunc: %02X.\n", pI);
	return 0;
}

unsigned int __stdcall FuncVirtualPC(struct _exec_env *pEnv, struct _cb_info *pCB, unsigned int *dwFlags, char *pI, char *pD, unsigned int *dwWritten) {
	pD[0] = pI[0];
	pD[1] = pI[1];
	pD[2] = pI[2];

	return *dwWritten = 3;
}

unsigned int __stdcall FuncF6(struct _exec_env *pEnv, struct _cb_info *pCB, unsigned int *dwFlags, char *pI, char *pD, unsigned int *dwWritten) {
	unsigned int size, test;

	pD[0] = pI[0];
	pD[1] = pI[1];

	size = GetModrmSize(*dwFlags, pI);
	if (size) {
		memcpy(pD + 2, pI + 2, size);
	}

	test = (pI[1] >> 3) & 0x07;
	if ((test == 0) || (test == 1)) {
		pD[2 + size] = pI[2 + size];
		return *dwWritten = 2 + size + 1;	
	}

	return *dwWritten = 2 + size;
}

unsigned int __stdcall FuncF7(struct _exec_env *pEnv, struct _cb_info *pCB, unsigned int *dwFlags, char *pI, char *pD, unsigned int *dwWritten) {
	unsigned int size, test;

	pD[0] = pI[0];
	pD[1] = pI[1];

	size = GetModrmSize(*dwFlags, pI);
	if (size) {
		memcpy(pD + 2, pI + 2, size);
	}

	test = (pI[1] >> 3) & 0x07;
	if ((test == 0) || (test == 1)) {
		//pD[2 + size] = pI[2 + size];
		//return *dwWritten = 2 + size + 1;	

		pD[size + 2] = pI[size + 2];
		pD[size + 3] = pI[size + 3];
		if ((*dwFlags & FLAG_O16) == 0) {
			pD[size + 4] = pI[size + 4];
			pD[size + 5] = pI[size + 5];
			return *dwWritten = 2 + size + 4;
		}

		return *dwWritten = 2 + size + 2;
	}

	return *dwWritten = 2 + size;
}

const char pBranchFF[] = {
	0xA3, 0x00, 0x00, 0x00, 0x00,				// 0x00 - [<dwEaxSave>], eax
	0x8B, 0x00									// 0x05 - mov ...
};

const char pBranchFFCall[] = {
	0x87, 0x25, 0x00, 0x00, 0x00, 0x00,			// 0x00 - xchg esp, large [<dwVirtualStack>]
	0x9C, 										// 0x06 - pushf
	0x60,										// 0x07 - pusha
	0x68, 0x46, 0x02, 0x00, 0x00,				// 0x08 - push 0x00000246 - NEW FLAGS
	0x9D,										// 0x0D - popf
	0x50,                            			// 0x0E - push eax
	0x68, 0x00, 0x00, 0x00, 0x00,				// 0x0F - push <execution_environment>
	0xFF, 0x15, 0x00, 0x00, 0x00, 0x00,         // 0x14 - call [<dwBranchHandler>]

	0x61,										// 0x1A - popa
	0x9D,										// 0x1B - popf
	0x87, 0x25, 0x00, 0x00, 0x00, 0x00,			// 0x1C - xchg esp, large ds:<dwVirtualStack>
	0xA1, 0x00, 0x00, 0x00, 0x00,               // 0x22 - mov eax, [<dwEaxSave>]
	0xFF, 0x25, 0x00, 0x00, 0x00, 0x00			// 0x27 - jmp large dword ptr ds:<jumpbuff>
};

unsigned int __stdcall FuncFF(struct _exec_env *pEnv, struct _cb_info *pCB, unsigned int *dwFlags, char *pI, char *pD, unsigned int *dwWritten) {
	char *pCurr;
	unsigned int size, test;

	size = GetModrmSize(*dwFlags, pI);
	/*if (size) {
	memcpy(pBuffer, &pI[2], size);
	}*/
	
	test = (pI[1] >> 3) & 7;
	pCurr = pD;
	if ((test == 2) || (test == 4)) {
		memcpy(pCurr, pBranchFF, sizeof(pBranchFF));
		*(unsigned int *)(&pCurr[1]) = (unsigned int)&pEnv->returnRegister;
		pCurr[6] = pI[1] & 0xC7;
		pCurr += sizeof(pBranchFF);

		if (size) {
			memcpy(pCurr, pI + 2, size);
			pCurr += size;
		}

		if (test == 2) {
			pCurr[0] = 0x68; //push 
			pCurr += 1;
			*(unsigned int *)pCurr = (unsigned int)&pI[2 + size];
			pCurr += 4;
		}

		memcpy(pCurr, pBranchFFCall, sizeof(pBranchFFCall));
		*(unsigned int *)(&pCurr[0x02]) = (unsigned int)&pEnv->virtualStack;
		*(unsigned int *)(&pCurr[0x10]) = (unsigned int)pEnv;		
		*(unsigned int *)(&pCurr[0x16]) = (unsigned int)&dwBranchHandler;		
		*(unsigned int *)(&pCurr[0x1E]) = (unsigned int)&pEnv->virtualStack;
		*(unsigned int *)(&pCurr[0x23]) = (unsigned int)&pEnv->returnRegister;
		*(unsigned int *)(&pCurr[0x29]) = (unsigned int)&pEnv->jumpBuff;
		pCurr += sizeof(pBranchFFCall);

		*dwWritten = pCurr - pD;
		*dwFlags |= FLAG_BRANCH;
		return 2 + size;
	} else {
		memcpy(pD, pI, 2 + size);
		*dwWritten = 2 + size;
		return 2 + size;	
	}	
}

const char pSysEnter[] = {
	0x87, 0x25, 0x00, 0x00, 0x00, 0x00,			// 0x00 - xchg esp, [<dwVirtualStack>]
	0x9C,			 							// 0x06 - pushf
	0x60,										// 0x07 - pusha
	0x68, 0x46, 0x02, 0x00, 0x00,				// 0x08 - push 0x00000246 - NEW FLAGS
	0x9D,										// 0x0D - popf
	0x68, 0x00, 0x00, 0x00, 0x00,				// 0x0E - push <execution_environment>
	0xFF, 0x15, 0x00, 0x00, 0x00, 0x00,         // 0x13 - call [<dwSysHandler>]
	0x58,										// 0x19 - pop eax // pop the execution environment off the stack
	0x61,										// 0x1A - popa
	0x9D,										// 0x1B - popf
	0x87, 0x25, 0x00, 0x00, 0x00, 0x00,			// 0x1C - xchg esp, [<dwVirtualStack>]
	0x0F, 0x34									// 0x22 - sysenter :D

};

unsigned int __stdcall FuncSysEnter(struct _exec_env *pEnv, struct _cb_info *pCB, unsigned int *dwFlags, char *pI, char *pD, unsigned int *dwWritten) {
	pD -= 1; //back up one byte (probably 0x0F)
	memcpy(pD, pSysEnter, sizeof(pSysEnter));

	*(unsigned int *)(&pD[0x02]) = (unsigned int)&pEnv->virtualStack;
	*(unsigned int *)(&pD[0x0F]) = (unsigned int)pEnv;
	*(unsigned int *)(&pD[0x15]) = (unsigned int)&dwSysHandler;
	*(unsigned int *)(&pD[0x1E]) = (unsigned int)&pEnv->virtualStack;

	*dwWritten = sizeof(pSysEnter) - 1;
	*dwFlags |= FLAG_BRANCH;
	return 1;	
}


OpCopy Table00[] = {
    FuncModrm, FuncModrm, FuncModrm, FuncModrm, FuncImm8, FuncImm32, HandleSingle, HandleSingle,
    FuncModrm, FuncModrm, FuncModrm, FuncModrm, FuncImm8, FuncImm32, HandleSingle, FuncExt,
    FuncModrm, FuncModrm, FuncModrm, FuncModrm, FuncImm8, FuncImm32, HandleSingle, HandleSingle,
    FuncModrm, FuncModrm, FuncModrm, FuncModrm, FuncImm8, FuncImm32, HandleSingle, HandleSingle,
    FuncModrm, FuncModrm, FuncModrm, FuncModrm, FuncImm8, FuncImm32, FuncSeg, HandleSingle,
    FuncModrm, FuncModrm, FuncModrm, FuncModrm, FuncImm8, FuncImm32, FuncSeg, HandleSingle,
    FuncModrm, FuncModrm, FuncModrm, FuncModrm, FuncImm8, FuncImm32, FuncSeg, HandleSingle,
    FuncModrm, FuncModrm, FuncModrm, FuncModrm, FuncImm8, FuncImm32, FuncSeg, HandleSingle,
    HandleSingle, HandleSingle, HandleSingle, HandleSingle, HandleSingle, HandleSingle, HandleSingle, HandleSingle,
    HandleSingle, HandleSingle, HandleSingle, HandleSingle, HandleSingle, HandleSingle, HandleSingle, HandleSingle,
    HandleSingle, HandleSingle, HandleSingle, HandleSingle, HandleSingle, HandleSingle, HandleSingle, HandleSingle,
    HandleSingle, HandleSingle, HandleSingle, HandleSingle, HandleSingle, HandleSingle, HandleSingle, HandleSingle,
    HandleSingle, HandleSingle, FuncModrm, FuncModrm, FuncSeg, FuncSeg, FuncO16, FuncA16,
    FuncImm32, ModrmImm32, FuncImm8, ModrmImm8, HandleSingle, HandleSingle, HandleSingle, HandleSingle,
    FuncJCC_B, FuncJCC_B, FuncJCC_B, FuncJCC_B, FuncJCC_B, FuncJCC_B, FuncJCC_B, FuncJCC_B,
    FuncJCC_B, FuncJCC_B, FuncJCC_B, FuncJCC_B, FuncJCC_B, FuncJCC_B, FuncJCC_B, FuncJCC_B,
    ModrmImm8, ModrmImm32, ModrmImm8, ModrmImm8, FuncModrm, FuncModrm, FuncModrm, FuncModrm,
    FuncModrm, FuncModrm, FuncModrm, FuncModrm, FuncModrm, FuncModrm, FuncModrm, FuncModrm,
    HandleSingle, HandleSingle, HandleSingle, HandleSingle, HandleSingle, HandleSingle, HandleSingle, HandleSingle,
    HandleSingle, HandleSingle, FarPointer, HandleSingle, HandleSingle, HandleSingle, HandleSingle, HandleSingle,
    FuncAddr32, FuncAddr32, FuncAddr32, FuncAddr32, HandleSingle, HandleSingle, HandleSingle, HandleSingle,
    FuncImm8, FuncImm32, HandleSingle, HandleSingle, HandleSingle, HandleSingle, HandleSingle, HandleSingle,
    
	FuncImm8, FuncImm8, FuncImm8, FuncImm8, FuncImm8, FuncImm8, FuncImm8, FuncImm8,
    FuncImm32, FuncImm32, FuncImm32, FuncImm32, FuncImm32, FuncImm32, FuncImm32, FuncImm32,
    
	ModrmImm8, ModrmImm8, FuncCRetImm, FuncCRetn, FuncModrm, FuncModrm, ModrmImm8, ModrmImm32,
    FuncEnter, HandleSingle, DefaultBranch, FuncCRetFar, HandleSingle, FuncImm8, HandleSingle, HandleSingle,
    
	FuncModrm, FuncModrm, FuncModrm, FuncModrm, FuncImm8, FuncImm8, HandleSingle, HandleSingle,
    FuncModrm, FuncModrm, FuncModrm, FuncModrm, FuncModrm, FuncModrm, FuncModrm, FuncModrm,
    
	FuncJCC_B, FuncJCC_B, FuncJCC_B, FuncJCC_B, FuncImm8, FuncImm8, FuncImm8, FuncImm8,
    FuncCall, FuncJMP_D, FarPointer, FuncJMP_B, HandleSingle, HandleSingle, HandleSingle, HandleSingle,
    
	FuncLock, HandleSingle, FuncRep, FuncRep, HandleSingle, HandleSingle, FuncF6, FuncF7,
    HandleSingle, HandleSingle, HandleSingle, HandleSingle, HandleSingle, HandleSingle, FuncModrm, FuncFF
};

OpCopy Table0F[] = {
    FuncModrm, FuncModrm, FuncModrm, FuncModrm, DefaultFunc, FuncSysEnter, HandleSingle, HandleSingle,
    HandleSingle, HandleSingle, DefaultFunc, HandleSingle, DefaultFunc, DefaultFunc, DefaultFunc, DefaultFunc,
    FuncModrm, FuncModrm, FuncModrm, FuncModrm, FuncModrm, FuncModrm, FuncModrm, FuncModrm,
    FuncModrm, DefaultFunc, DefaultFunc, DefaultFunc, DefaultFunc, DefaultFunc, DefaultFunc, FuncModrm,
    HandleDouble, HandleDouble, HandleDouble, HandleDouble, DefaultFunc, DefaultFunc, DefaultFunc, DefaultFunc,
    FuncModrm, FuncModrm, FuncModrm, FuncModrm, FuncModrm, FuncModrm, FuncModrm, FuncModrm,
    DefaultFunc, HandleSingle, HandleSingle, DefaultFunc, FuncSysEnter, DefaultFunc, DefaultFunc, DefaultFunc,
    DefaultFunc, DefaultFunc, DefaultFunc, DefaultFunc, DefaultFunc, DefaultFunc, DefaultFunc, FuncVirtualPC,
    FuncModrm, FuncModrm, FuncModrm, FuncModrm, FuncModrm, FuncModrm, FuncModrm, FuncModrm,
    FuncModrm, FuncModrm, FuncModrm, FuncModrm, FuncModrm, FuncModrm, FuncModrm, FuncModrm,
    FuncModrm, FuncModrm, FuncModrm, FuncModrm, FuncModrm, FuncModrm, FuncModrm, FuncModrm,
    FuncModrm, FuncModrm, FuncModrm, FuncModrm, FuncModrm, FuncModrm, FuncModrm, FuncModrm,
    FuncModrm, FuncModrm, FuncModrm, FuncModrm, FuncModrm, FuncModrm, FuncModrm, FuncModrm,
    FuncModrm, FuncModrm, FuncModrm, FuncModrm, FuncModrm, FuncModrm, FuncModrm, FuncModrm,
    ModrmImm8, ModrmImm8, ModrmImm8, ModrmImm8, FuncModrm, FuncModrm, FuncModrm, HandleSingle,
    DefaultFunc, DefaultFunc, DefaultFunc, DefaultFunc, DefaultFunc, DefaultFunc, FuncModrm, FuncModrm,
    FuncJCC_D, FuncJCC_D, FuncJCC_D, FuncJCC_D, FuncJCC_D, FuncJCC_D, FuncJCC_D, FuncJCC_D,
    FuncJCC_D, FuncJCC_D, FuncJCC_D, FuncJCC_D, FuncJCC_D, FuncJCC_D, FuncJCC_D, FuncJCC_D,
    FuncModrm, FuncModrm, FuncModrm, FuncModrm, FuncModrm, FuncModrm, FuncModrm, FuncModrm,
    FuncModrm, FuncModrm, FuncModrm, FuncModrm, FuncModrm, FuncModrm, FuncModrm, FuncModrm,
    HandleSingle, HandleSingle, HandleSingle, FuncModrm, ModrmImm8, FuncModrm, DefaultFunc, DefaultFunc,
    HandleSingle, HandleSingle, HandleSingle, FuncModrm, ModrmImm8, FuncModrm, FuncModrm, FuncModrm,
    FuncModrm, FuncModrm, FuncModrm, FuncModrm, FuncModrm, FuncModrm, FuncModrm, FuncModrm,
    DefaultFunc, DefaultFunc, ModrmImm8, FuncModrm, FuncModrm, FuncModrm, FuncModrm, FuncModrm,
    FuncModrm, FuncModrm, ModrmImm8, FuncModrm, ModrmImm8, ModrmImm8, ModrmImm8, FuncModrm,
    HandleSingle, HandleSingle, HandleSingle, HandleSingle, HandleSingle, HandleSingle, HandleSingle, HandleSingle,
    DefaultFunc, FuncModrm, FuncModrm, FuncModrm, FuncModrm, FuncModrm, FuncModrm, FuncModrm,
    FuncModrm, FuncModrm, FuncModrm, FuncModrm, FuncModrm, FuncModrm, FuncModrm, FuncModrm,
    FuncModrm, FuncModrm, FuncModrm, FuncModrm, FuncModrm, FuncModrm, FuncModrm, FuncModrm,
    FuncModrm, FuncModrm, FuncModrm, FuncModrm, FuncModrm, FuncModrm, FuncModrm, FuncModrm,
    DefaultFunc, FuncModrm, FuncModrm, FuncModrm, FuncModrm, FuncModrm, FuncModrm, FuncModrm,
    FuncModrm, FuncModrm, FuncModrm, FuncModrm, FuncModrm, FuncModrm, FuncModrm, DefaultFunc
};