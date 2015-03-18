#include "common.h"
#include "extern.h"
#include "cb.h"
#include "callgates.h"

#include "river.h"

struct UserCtx {
	DWORD callCount;
};

void RiverPrintInstruction(struct RiverInstruction *ri);
void TransalteSave(struct _exec_env *pEnv, struct RiverInstruction *rIn, struct RiverInstruction *rOut, DWORD *outCount);
void TranslateReverse(struct _exec_env *pEnv, struct RiverInstruction *rIn, struct RiverInstruction *rOut, DWORD *outCount);

void PushToExecutionBuffer(struct _exec_env *pEnv, DWORD value) {
	pEnv->execBuff -= 4;
	*((DWORD *)pEnv->execBuff) = value;
}

DWORD PopFromExecutionBuffer(struct _exec_env *pEnv) {
	DWORD ret = *((DWORD *)pEnv->execBuff);
	pEnv->execBuff += 4;
	return ret;
}

void __stdcall BranchHandler(struct _exec_env *pEnv, DWORD a) {
	struct _cb_info *pCB;
	unsigned int tmp;
	struct UserCtx *ctx = (struct UserCtx *)pEnv->userContext;

	DbgPrint("BranchHandler: %08X\n", a);
	//DbgPrint("pEnv %08X\n", pEnv);
	//DbgPrint("sizes: %08X, %08X, %08X, %08X\n", pEnv->heapSize, pEnv->historySize, pEnv->logHashSize, pEnv->outBufferSize);
	if (pEnv->bForward) {
		PushToExecutionBuffer(pEnv, pEnv->history[pEnv->posHist - 1]);
	}

	ctx->callCount++;

	if (ctx->callCount % 3 == 0) {
		// go backwards
		DbgPrint("Going Backwards!!!\n");
		DWORD addr = PopFromExecutionBuffer(pEnv);

		DbgPrint("Looking for block\n");
		pCB = FindBlock(pEnv, addr);
		if (pCB) {
			DbgPrint("Block found\n");
			pCB->dwParses++;
			pEnv->posHist -= 1;
			pEnv->bForward = 0;
			pEnv->jumpBuff = (DWORD)pCB->pBkCode;
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
			pCB = FindBlock(pEnv, a);
			if (pCB) {
				DbgPrint("Block found\n");
				pCB->dwParses++;
				TouchBlock(pEnv, pCB);
				pEnv->bForward = 1;
				pEnv->jumpBuff = (DWORD)pCB->pFwCode;
			}
			else {
				DbgPrint("Not Found\n");
				pCB = NewBlock(pEnv);
				pCB->address = a;

				Translate(pEnv, pCB, 0);

				DbgPrint("= river saving code ===========================================================\n");
				for (int i = 0; i < pEnv->fwInstCount; ++i) {
					RiverPrintInstruction(&pEnv->fwRiverInst[i]);
				}
				DbgPrint("===============================================================================\n");

				DbgPrint("= river reversing code ========================================================\n");
				for (int i = 0; i < pEnv->bkInstCount; ++i) {
					RiverPrintInstruction(&pEnv->bkRiverInst[i]);
				}
				DbgPrint("===============================================================================\n");

				AddBlock(pEnv, pCB);
				//pEnv->jumpBuff = (DWORD)pCB->pCode;
				TouchBlock(pEnv, pCB);
				pEnv->bForward = 1;
				pEnv->jumpBuff = (DWORD)pCB->pFwCode;
			}

		}
		__except (1) { //EXCEPTION_EXECUTE_HANDLER
			pEnv->jumpBuff = a;

			/*if ((pCB != NULL) && (pCB->dwParses > 0x800)) {
			int *a = 0;
			*a = arr;
			}*/
		}
	}
}


void __cdecl SysHandler(struct _exec_env *pEnv,
	DWORD r7, DWORD r6, DWORD r5, DWORD r4,
	DWORD r3, DWORD r2, DWORD r1, DWORD r0
	)
{
	UINT_PTR a;
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

	*(UINT_PTR *)pEnv->virtualStack = (UINT_PTR)pCB->pCode;
}


void __cdecl SysEndHandler(struct _exec_env *pEnv,
	DWORD r7, DWORD r6, DWORD r5, DWORD r4,
	DWORD r3, DWORD r2, DWORD r1, DWORD r0,
	DWORD eflags
	)
{
	*(UINT_PTR *)(pEnv->virtualStack - 4) = pEnv->returnRegister;
}


int overlap(unsigned int a1, unsigned int a2, unsigned int b1, unsigned int b2);


int main() {
	struct _exec_env *pEnv;
	struct UserCtx *ctx;
	DWORD dwCount = 0;
	pEnv = NewEnv(0x100000, 0x10000, 0x10000, 16, 0x10000);

	pEnv->userContext = AllocUserContext(pEnv, sizeof(struct UserCtx));
	ctx = (struct UserCtx *)pEnv->userContext;
	ctx->callCount = 0;

	struct RiverInstruction ris[20];
	unsigned char tBuff[64];

	unsigned char *pOverlap = (unsigned char *)*(unsigned int *)((unsigned char *)overlap + 1);
	pOverlap += (UINT_PTR)overlap + 5;

	/*x86toriver(pEnv, pOverlap, ris, &dwCount);
	rivertox86(pEnv, ris, dwCount, tBuff);*/

	DWORD ret = call_cdecl_4(pEnv, (_fn_cdecl_4)&overlap, 3, 7, 2, 10);
	printf("Done. ret = %d\n", ret);

	printf("Test %d\n", overlap(3, 7, 2, 10));

	return 0;
}