#ifndef _CB_H
#define _CB_H

#include "revtracer.h"
#include "mm.h"
#include "sync.h"

using namespace rev;

#define RIVER_BASIC_BLOCK_DETOUR				0x80000000

class RiverBasicBlock {
public :
	/* informations about the real block */
	UINT_PTR			address; // original address
	DWORD				dwSize;  // block size
	DWORD				dwCRC;   // block crc
	DWORD				dwFlags; // block flags

	/* statistical translation information */
	DWORD				dwOrigOpCount; // number of instructions in the original block
	DWORD				dwFwOpCount; // number of instructions in the translated block
	DWORD				dwBkOpCount; // number of instructions in the reverse block
	DWORD				dwTrOpCount; // number of instruction in the tracking code

	/* statistical runtime information */
	DWORD				dwFwPasses; // number of block executions
	DWORD				dwBkPasses; // number of reverse block executions

	/* actual code information */
	unsigned char		*pCode; // deprecated
	unsigned char       *pFwCode; // forward bb
	unsigned char       *pBkCode; // reverse bb
	unsigned char		*pTrackCode; // tracking code

	/* block linkage (for hash table) */
	RiverBasicBlock		*pNext;

	void MarkForward();
	void MarkBackward();
};

class RiverBasicBlockCache {
private :
	RiverHeap *heap;
public :
	RiverMutex cbLock; //  = 0;
	RiverBasicBlock **hashTable; // = 0
	DWORD  historySize, logHashSize;

	RiverBasicBlockCache();
	~RiverBasicBlockCache();

	bool Init(RiverHeap *hp, DWORD logHSize, DWORD histSize);
	bool Destroy();

	RiverBasicBlock *NewBlock(UINT_PTR addr);
	RiverBasicBlock *FindBlock(UINT_PTR addr);

	typedef void(*BlockCallback)(void *, RiverBasicBlock *);
	void ForEachBlock(void *ctx, BlockCallback cb);
};

//RiverBasicBlock *NewBlock(struct _exec_env *pEnv);
//RiverBasicBlock *FindBlock(struct _exec_env *pEnv, unsigned long);

//void AddBlock(struct _exec_env *pEnv, RiverBasicBlock*);

void PrintHistory (struct _exec_env *pEnv);
DWORD DumpHistory(struct _exec_env *pEnv, unsigned char *o, unsigned long s, unsigned long *sz);

#endif

