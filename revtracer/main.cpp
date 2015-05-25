#include "common.h"
#include "extern.h"
#include "cb.h"
#include "callgates.h"

#include "river.h"
#include <intrin.h>

#include <stdio.h>

FILE *fBlocks;

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

extern DWORD dwExitProcess;

DWORD __stdcall SegmentHandler(struct _exec_env *pEnv, DWORD addr, WORD sg) {
/*#pragma pack (push, 1)
	struct {
		WORD limit;
		DWORD base;
	} gdt;
#pragma pack (pop)
	_sgdt(&gdt);

	BYTE (*gdtEntries)[8] = (BYTE (*)[8])((void *)gdt.base);

	DWORD newBase = gdtEntries[seg][7];
	newBase <<= 8;
	newBase |= gdtEntries[seg][4];
	newBase <<= 8;
	newBase |= gdtEntries[seg][3];
	newBase <<= 8;
	newBase |= gdtEntries[seg][2];*/

	WORD tmp;
	DWORD rt;

	__asm mov ax, gs
	__asm mov tmp, ax

	__asm mov ax, sg
	__asm mov gs, ax

	__asm mov eax, addr
	__asm lea eax, gs:[eax]
	__asm mov rt, eax

	__asm mov ax, tmp
	__asm mov gs, ax

	return 0;
}

void __stdcall BranchHandler(struct _exec_env *pEnv, DWORD a) {
	RiverBasicBlock *pCB;
	struct UserCtx *ctx = (struct UserCtx *)pEnv->userContext;

	DbgPrint("BranchHandler: %08X\n", a);

	fprintf(fBlocks, "0x%08x\n", a /*& 0xFFFF*/);
	fflush(fBlocks);

	if (a == dwExitProcess) {
		a = pEnv->exitAddr;
	}


	//DbgPrint("pEnv %08X\n", pEnv);
	//DbgPrint("sizes: %08X, %08X, %08X, %08X\n", pEnv->heapSize, pEnv->historySize, pEnv->logHashSize, pEnv->outBufferSize);
	if (pEnv->bForward) {
		PushToExecutionBuffer(pEnv, pEnv->lastFwBlock);
	}

	ctx->callCount++;

	if (/*ctx->callCount % 3*/ 1 == 0) {
		// go backwards
		DbgPrint("Going Backwards!!!\n");
		DWORD addr = PopFromExecutionBuffer(pEnv);

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
		DbgPrint("Going Forwards!!!\n");
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

int main(unsigned int argc, char *argv[]) {
	DWORD baseAddr = 0xf000000;
	if (!MapPE(baseAddr)) {
		return false;
	}

	//SegmentHandler(NULL, 0x756b271e, 0x33);

	fopen_s(&fBlocks, "blocks.log", "wt");
	
	struct _exec_env *pEnv;
	struct UserCtx *ctx;
	DWORD dwCount = 0;
	pEnv = new _exec_env(0x1000000, 0x10000, 0x1000000, 16, 0x10000);

	pEnv->userContext = AllocUserContext(pEnv, sizeof(struct UserCtx));
	ctx = (struct UserCtx *)pEnv->userContext;
	ctx->callCount = 0;

	//unsigned char *pOverlap = (unsigned char *)*(unsigned int *)((unsigned char *)overlap + 1);
	//pOverlap += (UINT_PTR)overlap + 5;

	/*x86toriver(pEnv, pOverlap, ris, &dwCount);
	rivertox86(pEnv, ris, dwCount, tBuff);*/

	//DWORD ret = call_cdecl_4(pEnv, (_fn_cdecl_4)&overlap, (void *)3, (void *)7, (void *)2, (void *)10);
	unsigned char *pMain = (unsigned char *)baseAddr + 0x96CE;
	DWORD ret = call_cdecl_2(pEnv, (_fn_cdecl_2)pMain, (void *)argc, (void *)argv);
	DbgPrint("Done. ret = %d\n", ret);

	/*DbgPrint("Test %d\n", overlap(3, 7, 2, 10));*/

	fclose(fBlocks);

	return 0;
}