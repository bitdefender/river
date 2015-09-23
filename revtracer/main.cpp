#include "common.h"
#include "extern.h"
#include "cb.h"
#include "callgates.h"

#include "river.h"
#include <intrin.h>

#include <stdio.h>
#include <stdlib.h>

FILE *fBlocks;

DWORD __stdcall TrackAddr(struct _exec_env *pEnv, DWORD dwAddr) {
	DbgPrint("TrackAddr 0x%08x\n", dwAddr);
	return 0;
}

DWORD __stdcall MarkAddr(struct _exec_env *pEnv, DWORD dwAddr, DWORD value) {
	DbgPrint("MarkAddr 0x%08x <= %d\n", dwAddr, value);
	return 0;
}

DWORD dwAddressTrackHandler = (DWORD)&TrackAddr;
DWORD dwAddressMarkHandler = (DWORD)&MarkAddr;

struct UserCtx {
	DWORD callCount;
};

void RiverPrintInstruction(struct RiverInstruction *ri);
void TransalteSave(struct _exec_env *pEnv, struct RiverInstruction *rIn, struct RiverInstruction *rOut, DWORD *outCount);
void TranslateReverse(struct _exec_env *pEnv, struct RiverInstruction *rIn, struct RiverInstruction *rOut, DWORD *outCount);

void PushToExecutionBuffer(struct _exec_env *pEnv, DWORD value) {
	pEnv->runtimeContext.execBuff -= 4;
	*((DWORD *)pEnv->runtimeContext.execBuff) = value;
}

DWORD PopFromExecutionBuffer(struct _exec_env *pEnv) {
	DWORD ret = *((DWORD *)pEnv->runtimeContext.execBuff);
	pEnv->runtimeContext.execBuff += 4;
	return ret;
}

DWORD TopFromExecutionBuffer(struct _exec_env *pEnv) {
	return *((DWORD *)pEnv->runtimeContext.execBuff);
}

extern DWORD dwExitProcess;
struct Regs {
	DWORD edi;
	DWORD esi;
	DWORD ebp;
	DWORD esp;

	DWORD ebx;
	DWORD edx;
	DWORD ecx;
	DWORD eax;
	DWORD eflags;
} regClone[2];
DWORD execStack[2];

bool RegCheck(struct _exec_env *pEnv, const Regs &r1, const Regs &r2) {
	if (r1.eax != r2.eax) {
		DbgPrint("EAX inconsistent!\n");
		return false;
	}

	if (r1.ecx != r2.ecx) {
		DbgPrint("ECX inconsistent!\n");
		return false;
	}

	if (r1.edx != r2.edx) {
		DbgPrint("EDX inconsistent!\n");
		return false;
	}

	if (r1.ebx != r2.ebx) {
		DbgPrint("EBX inconsistent!\n");
		return false;
	}

	if (r1.esp != r2.esp) {
		DbgPrint("ESP inconsistent!\n");
		return false;
	}

	if (r1.ebp != r2.ebp) {
		DbgPrint("EBP inconsistent!\n");
		return false;
	}

	if (r1.esi != r2.esi) {
		DbgPrint("ESI inconsistent!\n");
		return false;
	}

	if (r1.edi != r2.edi) {
		DbgPrint("ESI inconsistent!\n");
		return false;
	}

	if (r1.eflags != r2.eflags) {
		DbgPrint("EFLAGS inconsistent!\n");
		return false;
	}
	return true;
}

struct ExecStats {
	int blocksForward, blocksBackward;
	int nativeForward, nativeBackward;
	int transForward, transBackward;
};

void AddStates(void *ctx, RiverBasicBlock *bb) {
	ExecStats *stats = (ExecStats *)ctx;

	stats->blocksForward += bb->dwFwPasses;
	stats->blocksBackward += bb->dwBkPasses;

	stats->transForward += bb->dwFwPasses * bb->dwFwOpCount;
	stats->transBackward += bb->dwBkPasses * bb->dwBkOpCount;

	stats->nativeForward += bb->dwFwPasses * bb->dwOrigOpCount;
	stats->nativeBackward += bb->dwBkPasses * bb->dwOrigOpCount;
}

void PrintStats(struct _exec_env *env) {
	ExecStats stats;
	memset(&stats, 0, sizeof(stats));

	env->blockCache.ForEachBlock(&stats, AddStates);
	DbgPrint("========================================\n");
	DbgPrint("Native fw:%9d; Native bw:%9d\n", stats.nativeForward, stats.nativeBackward);
	DbgPrint("Blocks fw:%9d; Blocks bw:%9d\n", stats.blocksForward, stats.blocksBackward);
	DbgPrint("Trans fw: %9d; Trans bw: %9d\n", stats.transForward,  stats.transBackward);
	DbgPrint("========================================\n");
}

typedef void(*TrackFunc)(DWORD trackBuffer);

void __stdcall BranchHandler(struct _exec_env *pEnv, DWORD a) {
	Regs *currentRegs = (Regs *)((&a) + 1);
	RiverBasicBlock *pCB;
	struct UserCtx *ctx = (struct UserCtx *)pEnv->userContext;

	pEnv->runtimeContext.trackBuff = pEnv->runtimeContext.trackBase;

	//DbgPrint("BranchHandler: %08X\n", a);

	DWORD dwLastBlock = pEnv->lastFwBlock; //TopFromExecutionBuffer(pEnv);
	RiverBasicBlock *pLast = pEnv->blockCache.FindBlock(dwLastBlock);
	if (NULL != pLast) {
		((TrackFunc)pLast->pTrackCode)(pEnv->runtimeContext.trackBase - 4);
	}


	fprintf(fBlocks, "0x%08x\n", a /*& 0xFFFF*/);
	fflush(fBlocks);

	if (a == dwExitProcess) {
		PrintStats(pEnv);
		exit(0);
	}


	//DbgPrint("pEnv %08X\n", pEnv);
	//DbgPrint("sizes: %08X, %08X, %08X, %08X\n", pEnv->heapSize, pEnv->historySize, pEnv->logHashSize, pEnv->outBufferSize);
	if (pEnv->bForward) {
		PushToExecutionBuffer(pEnv, pEnv->lastFwBlock);
	} else {
		/*if (!RegCheck(pEnv, *currentRegs, regClone)) {
			__asm int 3;
		}*/
		switch (ctx->callCount % 5) {
		case 3:
			//copy the registers to validate them later
			if (!RegCheck(pEnv, *currentRegs, regClone[1])) {
				__asm int 3;
			}

			if (execStack[1] != pEnv->runtimeContext.virtualStack) {
				//__asm int 3
			}
			break;
		case 4:
			//copy the registers to validate them later
			if (!RegCheck(pEnv, *currentRegs, regClone[0])) {
				__asm int 3;
			}

			if (execStack[0] != pEnv->runtimeContext.virtualStack) {
				//__asm int 3
			}
			break;
		}
	}

	ctx->callCount++;

	DWORD *stk = (DWORD *)pEnv->runtimeContext.virtualStack;
	DbgPrint("Stack :\n");
	DbgPrint("0x%08x: 0x%08x 0x%08x 0x%08x 0x%08x\n", stk + 0x00, stk[0], stk[1], stk[2], stk[3]);
	DbgPrint("EAX: 0x%08x  ECX: 0x%08x  EDX: 0x%08x  EBX: 0x%08x\n", currentRegs->eax, currentRegs->ecx, currentRegs->edx, currentRegs->ebx);
	DbgPrint("ESP: 0x%08x  EBP: 0x%08x  ESI: 0x%08x  EDI: 0x%08x\n", currentRegs->esp, currentRegs->ebp, currentRegs->esi, currentRegs->edi);
	DbgPrint("Flags: 0x%08x\n", currentRegs->eflags);
	/*DbgPrint("0x%08x: 0x%08x 0x%08x 0x%08x 0x%08x\n", stk + 0x10, stk[4], stk[5], stk[6], stk[7]);
	DbgPrint("0x%08x: 0x%08x 0x%08x 0x%08x 0x%08x\n", stk + 0x20, stk[8], stk[9], stk[10], stk[11]);
	DbgPrint("0x%08x: 0x%08x 0x%08x 0x%08x 0x%08x\n", stk + 0x30, stk[12], stk[13], stk[14], stk[15]);*/

	if ((ctx->callCount % 5) > 5) {
		// go backwards
		DWORD addr = PopFromExecutionBuffer(pEnv);
		DbgPrint("Going Backwards to %08X!!!\n", addr);
		
		DbgPrint("Looking for block\n");
		pCB = pEnv->blockCache.FindBlock(addr);
		if (pCB) {
			DbgPrint("Block found\n");
			pCB->MarkBackward();
			//pEnv->posHist -= 1;
			pEnv->bForward = 0;
			pEnv->runtimeContext.jumpBuff = (DWORD)pCB->pBkCode;
		}
		else {
			DbgPrint("No reverse block found!");
			__asm int 3;
		}

	} else {
		// go forwards
		DbgPrint("Going Forwards from %08X!!!\n", a);
		__try {
			DbgPrint("Looking for block\n");
			pCB = pEnv->blockCache.FindBlock(a);
			if (pCB) {
				DbgPrint("Block found\n");
			} else {
				DbgPrint("Not Found\n");
				pCB = pEnv->blockCache.NewBlock(a);

				pEnv->codeGen.Translate(pCB, 0);

				DbgPrint("= river saving code ===========================================================\n");
				for (DWORD i = 0; i < pEnv->codeGen.fwInstCount; ++i) {
					RiverPrintInstruction(&pEnv->codeGen.fwRiverInst[i]);
				}
				DbgPrint("===============================================================================\n");

				DbgPrint("= river reversing code ========================================================\n");
				for (DWORD i = 0; i < pEnv->codeGen.bkInstCount; ++i) {
					RiverPrintInstruction(&pEnv->codeGen.bkRiverInst[i]);
				}
				DbgPrint("===============================================================================\n");
			}
			pCB->MarkForward();
			//pEnv->jumpBuff = (DWORD)pCB->pCode;
			//TouchBlock(pEnv, pCB);
			pEnv->lastFwBlock = pCB->address;
			pEnv->bForward = 1;

			switch (ctx->callCount % 5) {
				case 1: 
					//copy the registers to validate them later
					memcpy(&regClone[0], currentRegs, sizeof(regClone[0]));
					execStack[0] = pEnv->runtimeContext.virtualStack;
					break;
				case 2:
					//copy the registers to validate them later
					memcpy(&regClone[1], currentRegs, sizeof(regClone[1]));
					execStack[1] = pEnv->runtimeContext.virtualStack;
					break;
			}
			

			pEnv->runtimeContext.jumpBuff = (DWORD)pCB->pFwCode;
			fflush(stdout);
		}
		__except (1) { //EXCEPTION_EXECUTE_HANDLER
			pEnv->runtimeContext.jumpBuff = a;

			/*if ((pCB != NULL) && (pCB->dwParses > 0x800)) {
			int *a = 0;
			*a = arr;
			}*/
		}
	}
}


void __stdcall SysHandler(struct _exec_env *pEnv) {
	DbgPrint("SysHandler!!!\n");

	fprintf(fBlocks, "Syscall!!\n");
	fflush(fBlocks);

	TakeSnapshot();

	/*UINT_PTR a;
	struct _cb_info *pCB;

	a = *(UINT_PTR*)pEnv->virtualStack;
	pEnv->returnRegister = a;

	pCB = FindBlock(pEnv, a);

	if (pCB) {
		if ((pCB->dwParses & CB_FLAG_SYSOUT) == CB_FLAG_SYSOUT) {
			pCB->dwParses++;
			*(UINT_PTR *)pEnv->virtualStack = (UINT_PTR)pCB->pCode;
			return;
		}
		else {
			//	dbg0 ("This CB receives control after a sysexit, but no CB_FLAG_SYSOUT!\n");
			//	_asm int 3
		}
	}

	pCB = NewBlock(pEnv);

	pCB->address = a;
	Translate(pEnv, pCB, CB_FLAG_SYSOUT);
	AddBlock(pEnv, pCB);

	*(UINT_PTR *)pEnv->virtualStack = (UINT_PTR)pCB->pCode;*/
}


void __cdecl SysEndHandler(struct _exec_env *pEnv,
	DWORD r7, DWORD r6, DWORD r5, DWORD r4,
	DWORD r3, DWORD r2, DWORD r1, DWORD r0,
	DWORD eflags
	)
{
	*(UINT_PTR *)(pEnv->runtimeContext.virtualStack - 4) = pEnv->runtimeContext.returnRegister;
}


int overlap(unsigned int a1, unsigned int a2, unsigned int b1, unsigned int b2);

bool MapPE(DWORD &baseAddr);

unsigned char goat[] = { /*0xEB, 0x00, 0x90,*/ 0xEB, 0x00, 0x90, 0xB8, 0x10, 0x00, 0x00, 0x00, 0x29, 0xC4, 0x01, 0xC4, 0xEB, 0x00, 0x90, 0xC3 };

int main(unsigned int argc, char *argv[]) {
	DWORD baseAddr = 0xf000000;
	if (!MapPE(baseAddr)) {
		return false;
	}

#ifdef _USE_VBOX_SNAPSHOTS
	
#endif

	//SegmentHandler(NULL, 0x756b271e, 0x33);

	fopen_s(&fBlocks, "blocks.log", "wt");
	
	struct _exec_env *pEnv;
	struct UserCtx *ctx;
	DWORD dwCount = 0;
	pEnv = new _exec_env(0x1000000, 0x10000, 0x2000000, 16, 0x10000);

	pEnv->userContext = AllocUserContext(pEnv, sizeof(struct UserCtx));
	ctx = (struct UserCtx *)pEnv->userContext;
	ctx->callCount = 0;

	//unsigned char *pOverlap = (unsigned char *)*(unsigned int *)((unsigned char *)overlap + 1);
	//pOverlap += (UINT_PTR)overlap + 5;

	/*x86toriver(pEnv, pOverlap, ris, &dwCount);
	rivertox86(pEnv, ris, dwCount, tBuff);*/

	//DWORD ret = call_cdecl_4(pEnv, (_fn_cdecl_4)&overlap, (void *)3, (void *)7, (void *)2, (void *)10);
	unsigned char *pMain = (unsigned char *)baseAddr + 0x96CE;
	//unsigned char *pMain = (unsigned char *)baseAddr + 0x1347;
	//unsigned char *pMain = goat;
	DWORD ret = call_cdecl_2(pEnv, (_fn_cdecl_2)pMain, (void *)argc, (void *)argv);
	DbgPrint("Done. ret = %d\n", ret);

	PrintStats(pEnv);

	/*DbgPrint("Test %d\n", overlap(3, 7, 2, 10));*/

	fclose(fBlocks);

	return 0;
}