#include "common.h"
#include "revtracer.h"
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


rev::DWORD HashFunc(unsigned int logHashSize, unsigned long a) {
	//DbgPrint("Value = %08x\n", a);
	//DbgPrint("logHashSize = %08x\n", logHashSize);
	//DbgPrint("mask = %08x\n", ((1 << logHashSize) - 1));
	return a & ((1 << logHashSize) - 1);
}

#ifndef BLOCK_CACHE_READ_ONLY
RiverBasicBlockCache::RiverBasicBlockCache() {
	hashTable = NULL;
	logHashSize = 0;
}

RiverBasicBlockCache::~RiverBasicBlockCache() {
	if (0 != logHashSize) {
		Destroy();
	}
}

RiverBasicBlock *RiverBasicBlockCache::NewBlock(rev::UINT_PTR a) {
	RiverBasicBlock *pNew;
	
	pNew = (RiverBasicBlock *)heap->Alloc(sizeof(*pNew));

	if (NULL == pNew) {
		return NULL;
	}

	rev_memset(pNew, 0, sizeof(*pNew));
	pNew->address = a;

	cbLock.Lock();

	rev::DWORD dwHash = HashFunc(logHashSize, pNew->address); // & 0xFFFF;

	pNew->pNext = hashTable[dwHash];
	hashTable[dwHash] = pNew;

	cbLock.Unlock();

	return pNew;
}

bool RiverBasicBlockCache::Init(RiverHeap *hp, rev::DWORD logHSize, rev::DWORD histSize) {
	heap = hp;

	logHashSize = logHSize;
	historySize = histSize;
	hashTable = (RiverBasicBlock **)rev::revtracerAPI.memoryAllocFunc((1 << logHashSize) * sizeof(hashTable[0]));

	if (0 == hashTable) {
		return false;
	}

	rev_memset(hashTable, 0, (1 << logHashSize) * sizeof(hashTable[0]));

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

			if (pAdd->address != (rev::UINT_PTR)pAdd->pCode) {
				heap->Free(pAdd->pCode);
			}

			heap->Free(pAdd);
		}
	}

	//	SC_Unlock (&dwCBLock);

	rev::revtracerAPI.memoryFreeFunc((rev::BYTE *)hashTable);
	hashTable = NULL;
	logHashSize = 0;


	/*EnvMemoryFree ((BYTE *)pEnv->history);
	pEnv->history = NULL;*/
	return true;
}

#endif

RiverBasicBlock *RiverBasicBlockCache::FindBlock(rev::UINT_PTR a) {
	RiverBasicBlock *pWalk;
	int arr = 0;
	unsigned long hash = HashFunc(logHashSize, a);

	cbLock.Lock();

	pWalk = hashTable[hash];

	while (pWalk) {
		if (pWalk->address == a) {
			if (RIVER_BASIC_BLOCK_DETOUR & pWalk->dwFlags) { // do not crc check this block as it is a detour
				cbLock.Unlock();
				return pWalk;
			}

#ifndef BLOCK_CACHE_READ_ONLY
			if (pWalk->dwCRC == (unsigned long) crc32 (0xEDB88320, (rev::BYTE *) a, pWalk->dwSize)) {
				cbLock.Unlock();
				return pWalk;
			} else {
				//	_asm int 3
				//	dbg1 ("___SMC___ at address %08X.\n", a);

				cbLock.Unlock();
				return NULL;
			}
#else
			cbLock.Unlock();
			return pWalk;
#endif
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

void RiverBasicBlockCache::ForEachBlock(void *ctx, BlockCallback cb) {
	for (int i = 0; i < (1 << logHashSize); ++i) {
		RiverBasicBlock *pWalk = hashTable[i];

		while (pWalk) {
			cb(ctx, pWalk);
			pWalk = pWalk->pNext;
		}
	}
}


