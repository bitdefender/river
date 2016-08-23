#ifndef _CB_H
#define _CB_H

#include "revtracer.h"
#include "mm.h"
#include "sync.h"

#define RIVER_BASIC_BLOCK_DETOUR				0x80000000

class RiverBasicBlock {
public :
	/* informations about the real block */
	rev::UINT_PTR			address; // original address
	rev::DWORD				dwSize;  // block size
	rev::DWORD				dwCRC;   // block crc
	rev::DWORD				dwFlags; // block flags

	/* statistical translation information */
	rev::DWORD				dwOrigOpCount; // number of instructions in the original block
	rev::DWORD				dwFwOpCount; // number of instructions in the translated block
	rev::DWORD				dwBkOpCount; // number of instructions in the reverse block
	rev::DWORD				dwTrOpCount; // number of instructions in the tracking code
	rev::DWORD				dwRtOpCount; // number of instructions in the reverse trakcing code

	/* statistical runtime information */
	rev::DWORD				dwFwPasses; // number of block executions
	rev::DWORD				dwBkPasses; // number of reverse block executions

	/* actual code information */
	unsigned char		*pCode; // deprecated
	unsigned char       *pFwCode; // forward bb
	unsigned char       *pBkCode; // reverse bb
	unsigned char		*pTrackCode; // tracking code
	unsigned char		*pRevTrackCode; // reverse tracking code
	unsigned char       *pDisasmCode; // disassembled code

	/* block linkage (for hash table) */
	RiverBasicBlock		*pNext;

	/* branching cache, in order to speed up lookup */
	RiverBasicBlock		*pBranchCache[2];

	void MarkForward();
	void MarkBackward();
};

class RiverBasicBlockCache {
private :
	RiverHeap *heap;
public :
	RiverMutex cbLock; //  = 0;
	RiverBasicBlock **hashTable; // = 0
	rev::DWORD historySize, logHashSize;

	RiverBasicBlockCache();
	~RiverBasicBlockCache();

	bool Init(RiverHeap *hp, rev::DWORD logHSize, rev::DWORD histSize);
	bool Destroy();

	RiverBasicBlock *NewBlock(rev::UINT_PTR addr);
	RiverBasicBlock *FindBlock(rev::UINT_PTR addr);

	typedef void(*BlockCallback)(void *, RiverBasicBlock *);
	void ForEachBlock(void *ctx, BlockCallback cb);
};

//RiverBasicBlock *NewBlock(struct _exec_env *pEnv);
//RiverBasicBlock *FindBlock(struct _exec_env *pEnv, unsigned long);

//void AddBlock(struct _exec_env *pEnv, RiverBasicBlock*);

void PrintHistory(struct ExecutionEnvironment *pEnv);
rev::DWORD DumpHistory(struct ExecutionEnvironment *pEnv, unsigned char *o, unsigned long s, unsigned long *sz);

#endif

