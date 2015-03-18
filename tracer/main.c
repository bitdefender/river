#include "common.h"
#include "extern.h"
#include "cb.h"
#include "callgates.h"

void __stdcall BranchHandler(struct _exec_env *pEnv, DWORD a) {
	struct _cb_info *pCB;

	DbgPrint("BranchHandler: %08X\n", a);
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
	pEnv = NewEnv(0x100000, 0x10000, 16, 0x10000);

	DWORD ret = call_cdecl_4(pEnv, (_fn_cdecl_4)&overlap, 3, 7, 2, 10);


	printf("Done. ret = %d\n", ret);
	printf("Test %d\n", overlap(3, 7, 2, 10));


	return 0;
}