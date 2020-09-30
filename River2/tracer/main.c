#include "common.h"
#include "extern.h"
#include "cb.h"
#include "callgates.h"

#include <stdio.h>

FILE *fBlocks;

void __stdcall BranchHandler(struct _exec_env *pEnv, DWORD a) {
	struct _cb_info *pCB;

	DbgPrint("BranchHandler: %08X\n", a);
	fprintf(fBlocks, "0x%04x\n", a & 0xFFFF);
	fflush(fBlocks);
	//DbgPrint("pEnv %08X\n", pEnv);
	//DbgPrint("sizes: %08X, %08X, %08X, %08X\n", pEnv->heapSize, pEnv->historySize, pEnv->logHashSize, pEnv->outBufferSize);

	__try {
		DbgPrint("Looking for block\n");
		pCB = FindBlock(pEnv, a);
		if (pCB) {
			DbgPrint("Block found\n");
			pCB->dwParses++;
			pEnv->jumpBuff = (DWORD)pCB->pCode;
		}
		else {
			DbgPrint("Not Found\n");
			pCB = NewBlock(pEnv);
			pCB->address = a;

			Translate(pEnv, pCB, 0);

			AddBlock(pEnv, pCB);
			pEnv->jumpBuff = (DWORD)pCB->pCode;
		}

	}
	__except (1) { //EXCEPTION_EXECUTE_HANDLER
		pEnv->jumpBuff = a;

		/*if ((pCB != NULL) && (pCB->dwParses > 0x800)) {
		int *a = 0;
		*a = arr;
		}*/
	}

	fflush(stdout);
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

BYTE MapPE(DWORD *baseAddr);

int main(unsigned int argc, char *argv[]) {
	/*struct _exec_env *pEnv;
	pEnv = NewEnv(0x100000, 0x10000, 16, 0x10000);

	DWORD ret = call_cdecl_4(pEnv, (_fn_cdecl_4)&overlap, 3, 7, 2, 10);


	printf("Done. ret = %d\n", ret);
	printf("Test %d\n", overlap(3, 7, 2, 10));


	return 0;*/

	fopen_s(&fBlocks, "blocks.log", "wt");

	DWORD baseAddr = 0;
	if (!MapPE(&baseAddr)) {
		return 0;
	}



	struct _exec_env *pEnv;
	struct UserCtx *ctx;
	DWORD dwCount = 0;
	pEnv = NewEnv(0x100000, 0x10000, 16, 0x10000);
	
	//unsigned char *pOverlap = (unsigned char *)*(unsigned int *)((unsigned char *)overlap + 1);
	//pOverlap += (UINT_PTR)overlap + 5;

	/*x86toriver(pEnv, pOverlap, ris, &dwCount);
	rivertox86(pEnv, ris, dwCount, tBuff);*/

	//DWORD ret = call_cdecl_4(pEnv, (_fn_cdecl_4)&overlap, (void *)3, (void *)7, (void *)2, (void *)10);
	unsigned char *pMain = (unsigned char *)baseAddr + 0x96CE;
	DWORD ret = call_cdecl_2(pEnv, (_fn_cdecl_2)pMain, (void *)argc, (void *)argv);
	DbgPrint("Done. ret = %d\n", ret);

	DbgPrint("Test %d\n", overlap(3, 7, 2, 10));

	fclose(fBlocks);

	return 0;
}