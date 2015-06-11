#include "common.h"
#include "extern.h"
#include "cb.h"
#include "sync.h"
#include "crc32.h"
#include "mm.h"

/*#define HASH_TABLE_SIZE 0x10000
#define HISTORY_SIZE 0x10000*/

void RiverBasicBlock::MarkForward() {
	dwFwPasses++;
}

void RiverBasicBlock::MarkBackward() {
	dwBkPasses++;
}


DWORD HashFunc(unsigned int logHashSize, unsigned long a) {
	//DbgPrint("Value = %08x\n", a);
	//DbgPrint("logHashSize = %08x\n", logHashSize);
	//DbgPrint("mask = %08x\n", ((1 << logHashSize) - 1));
	return a & ((1 << logHashSize) - 1);
}

RiverBasicBlockCache::RiverBasicBlockCache() {
	hashTable = NULL;
	logHashSize = 0;
}

RiverBasicBlockCache::~RiverBasicBlockCache() {
	if (0 != logHashSize) {
		Destroy();
	}
}

RiverBasicBlock *RiverBasicBlockCache::NewBlock(UINT_PTR a) {
	RiverBasicBlock *pNew;
	
	pNew = (RiverBasicBlock *)heap->Alloc(sizeof(*pNew));

	if (NULL == pNew) {
		return NULL;
	}

	memset(pNew, 0, sizeof (*pNew));
	pNew->address = a;

	cbLock.Lock();

	DWORD dwHash = HashFunc(logHashSize, pNew->address); // & 0xFFFF;

	pNew->pNext = hashTable[dwHash];
	hashTable[dwHash] = pNew;

	cbLock.Unlock();

	return pNew;
}

RiverBasicBlock *RiverBasicBlockCache::FindBlock(UINT_PTR a) {
	RiverBasicBlock *pWalk;
	int arr = 0;
	unsigned long hash = HashFunc(logHashSize, a);

	DbgPrint("HASH %08x\n", hash);
	cbLock.Lock();

	pWalk = hashTable[hash];

	while (pWalk) {
		if (pWalk->address == a) {
			if (pWalk->dwCRC == (unsigned long) crc32 (0xEDB88320, (BYTE *) a, pWalk->dwSize)) {
				cbLock.Unlock();
				return pWalk;
			} else {
				//	_asm int 3
				//	dbg1 ("___SMC___ at address %08X.\n", a);

				cbLock.Unlock();
				return NULL;
			}
		}

		pWalk = pWalk->pNext;
		if (++arr > 0x800) {
			cbLock.Unlock();
			__asm int 3
		}
	}

	cbLock.Unlock();

	return NULL;
}


void TouchBlock(struct _exec_env *pEnv, struct _cb_info *pCB) {
	//TbmMutexLock(&pEnv->cbLock);

	/*if (pEnv->posHist < (pEnv->historySize - 1)) {
		pEnv->history[pEnv->posHist] = pCB->address;
		pEnv->posHist++;
	}
	pEnv->totHist++;*/
	//TbmMutexUnlock(&pEnv->cbLock);
}

void RiverBasicBlockCache::ForEachBlock(void *ctx, BlockCallback cb) {
	for (int i = 0; i < (1 << logHashSize); ++i) {
		RiverBasicBlock *pWalk = hashTable[i];

		while (pWalk) {
			cb(ctx, pWalk);
			pWalk = pWalk->pNext;
		}
	}
}

bool RiverBasicBlockCache::Init(RiverHeap *hp, DWORD logHSize, DWORD histSize) {
	heap = hp;

	logHashSize = logHSize;
	historySize = histSize;
	hashTable = (RiverBasicBlock **)EnvMemoryAlloc((1 << logHashSize) * sizeof (hashTable[0]));

	if (0 == hashTable) {
		return false;
	}

	memset (hashTable, 0, (1 << logHashSize) * sizeof (hashTable[0]));

	/*history = (UINT_PTR *)EnvMemoryAlloc(historySize * sizeof (history[0]));

	if (NULL == pEnv->history) {
		EnvMemoryFree((BYTE *)pEnv->hashTable);
		return 0;
	}

	memset (pEnv->history, 0, historySize * sizeof (pEnv->history[0]));

	DbgPrint("History is at %p.\n", pEnv->history);

	pEnv->posHist = pEnv->totHist = 0;*/
	return true;
}


bool RiverBasicBlockCache::Destroy() {
	unsigned long idx, midx;
	RiverBasicBlock *pWalk, *pAdd;

//	SC_Lock (&dwCBLock);

	midx = 1 << logHashSize;
	idx = 0;

	for (idx = 0; idx < midx; ++idx) {
		pWalk = hashTable[idx];

		while (pWalk) {
		//	DbgPrint("[%08X] [%08X] %08X -> %p.\n", pWalk, pWalk->dwParses & 0x7FFFFFFF, pWalk->address, pWalk->pCode);

			pAdd = pWalk;
			pWalk = pWalk->pNext;

			if (pAdd->address != (UINT_PTR) pAdd->pCode) {
				heap->Free(pAdd->pCode);
			}

			heap->Free(pAdd);
		}
	}

//	SC_Unlock (&dwCBLock);

	EnvMemoryFree ((BYTE *)hashTable);
	hashTable = NULL;
	logHashSize = 0;


	/*EnvMemoryFree ((BYTE *)pEnv->history);
	pEnv->history = NULL;*/
	return true;
}

/*void PrintHistory(struct _exec_env *pEnv) {
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
}*/

