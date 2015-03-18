#include "common.h"
#include "extern.h"
#include "cb.h"
#include "sync.h"
#include "crc32.h"
#include "mm.h"

/*#define HASH_TABLE_SIZE 0x10000
#define HISTORY_SIZE 0x10000*/

unsigned long HashFunc(unsigned int logHashSize, unsigned long a) {
	//DbgPrint("Value = %08x\n", a);
	//DbgPrint("logHashSize = %08x\n", logHashSize);
	//DbgPrint("mask = %08x\n", ((1 << logHashSize) - 1));
	return a & ((1 << logHashSize) - 1);
}

struct _cb_info *NewBlock(struct _exec_env *pEnv) {
	struct _cb_info *pNew;
	
	pNew = (struct _cb_info *)pEnv->heap.Alloc(sizeof(*pNew));

	if (pNew) {
		memset (pNew, 0, sizeof (*pNew));
		return pNew;
	}

	//dbg0(">> Memory allocation error!!\n");

	return NULL;
}

struct _cb_info *FindBlock (struct _exec_env *pEnv, unsigned long a) {
	struct _cb_info *pWalk;
	int arr = 0;
	unsigned long hash = HashFunc(pEnv->logHashSize, a);

	DbgPrint("HASH %08x\n", hash);
	TbmMutexLock(&pEnv->cbLock);

	pWalk = pEnv->hashTable[hash];

	while (pWalk) {
		if (pWalk->address == a) {
			if (pWalk->dwCRC == (unsigned long) crc32 (0xEDB88320, (BYTE *) a, pWalk->dwSize)) {
				TbmMutexUnlock(&pEnv->cbLock);
				return pWalk;
			} else {
				//	_asm int 3
				//	dbg1 ("___SMC___ at address %08X.\n", a);

				TbmMutexUnlock(&pEnv->cbLock);
				return NULL;
			}
		}

		pWalk = pWalk->pNext;
		if (++arr > 0x800) {
			TbmMutexUnlock(&pEnv->cbLock);
			__asm int 3
		}
	}

	TbmMutexUnlock(&pEnv->cbLock);

	return NULL;
}


void TouchBlock(struct _exec_env *pEnv, struct _cb_info *pCB) {
	TbmMutexLock(&pEnv->cbLock);

	if (pEnv->posHist < (pEnv->historySize - 1)) {
		pEnv->history[pEnv->posHist] = pCB->address;
		pEnv->posHist++;
	}
	pEnv->totHist++;
	TbmMutexUnlock(&pEnv->cbLock);
}

void AddBlock(struct _exec_env *pEnv, struct _cb_info *pCB) {
	unsigned long dwHash;

	TbmMutexLock(&pEnv->cbLock);

	dwHash = HashFunc(pEnv->logHashSize, pCB->address); // & 0xFFFF;

	pCB->pNext = pEnv->hashTable[dwHash];
	pEnv->hashTable[dwHash] = pCB;

	TbmMutexUnlock(&pEnv->cbLock);
}

int InitBlock(struct _exec_env *pEnv, unsigned int logHashSize, unsigned int historySize) {
	pEnv->hashTable = (struct _cb_info **)EnvMemoryAlloc((1 << logHashSize) * sizeof (pEnv->hashTable[0]));

	if (!pEnv->hashTable) {
		return 0;
	}

	memset (pEnv->hashTable, 0, (1 << logHashSize) * sizeof (pEnv->hashTable[0]));

	pEnv->history = (UINT_PTR *)EnvMemoryAlloc(historySize * sizeof (pEnv->history[0]));

	if (NULL == pEnv->history) {
		EnvMemoryFree((BYTE *)pEnv->hashTable);
		return 0;
	}

	memset (pEnv->history, 0, historySize * sizeof (pEnv->history[0]));

	DbgPrint("History is at %p.\n", pEnv->history);

	pEnv->posHist = pEnv->totHist = 0;
	pEnv->logHashSize = logHashSize;
	pEnv->historySize = historySize;

	return 1;
}


void CloseBlock(struct _exec_env *pEnv) {
	unsigned long idx, midx;
	struct _cb_info *pWalk, *pAdd;

//	SC_Lock (&dwCBLock);

	midx = 1 << pEnv->logHashSize;
	idx = 0;

	for (idx = 0; idx < midx; ++idx) {
		pWalk = pEnv->hashTable[idx];

		while (pWalk) {
		//	DbgPrint("[%08X] [%08X] %08X -> %p.\n", pWalk, pWalk->dwParses & 0x7FFFFFFF, pWalk->address, pWalk->pCode);

			pAdd = pWalk;
			pWalk = pWalk->pNext;

			if (pAdd->address != (UINT_PTR) pAdd->pCode) {
				pEnv->heap.Free(pAdd->pCode);
			}

			pEnv->heap.Free(pAdd);
		}
	}

//	SC_Unlock (&dwCBLock);

	EnvMemoryFree ((BYTE *)pEnv->hashTable);
	pEnv->hashTable = NULL;
	EnvMemoryFree ((BYTE *)pEnv->history);
	pEnv->history = NULL;
}

void PrintHistory(struct _exec_env *pEnv) {
	unsigned long idx;

	for (idx = 0; idx < pEnv->posHist; idx ++) {
		DbgPrint("%08X -> %08X", idx, pEnv->history[idx]);
	}
}

DWORD DumpHistory(struct _exec_env *pEnv, unsigned char *o, unsigned long s, unsigned long *sz) {
	if ((pEnv->posHist * sizeof(pEnv->history[0])) > s) {
		return 0;
	}

	memcpy (o, (BYTE *)pEnv->history, pEnv->posHist * sizeof (pEnv->history[0]));
	*sz = (pEnv->posHist * sizeof (pEnv->history[0]));

	return 1;
}

