#ifndef _CB_H
#define _CB_H

#include "revtracer.h"
#include "mm.h"
#include "sync.h"

#define RIVER_BASIC_BLOCK_DETOUR				0x80000000

class RiverBasicBlock {
public :
	/* informations about the real block */
	nodep::UINT_PTR			address; // original address
	nodep::DWORD				dwSize;  // block size
	nodep::DWORD				dwCRC;   // block crc
	nodep::DWORD				dwFlags; // block flags

	/* statistical translation information */
	nodep::DWORD				dwOrigOpCount; // number of instructions in the original block
	nodep::DWORD				dwFwOpCount; // number of instructions in the translated block
	nodep::DWORD				dwBkOpCount; // number of instructions in the reverse block
	nodep::DWORD				dwTrOpCount; // number of instructions in the tracking code
	nodep::DWORD				dwRtOpCount; // number of instructions in the reverse trakcing code

	/* statistical runtime information */
	nodep::DWORD				dwFwPasses; // number of block executions
	nodep::DWORD				dwBkPasses; // number of reverse block executions

	/* actual code information */
	unsigned char		*pCode; // deprecated
	unsigned char       *pFwCode; // forward bb
	unsigned char       *pBkCode; // reverse bb
	unsigned char		*pTrackCode; // tracking code
	unsigned char		*pRevTrackCode; // reverse tracking code
	unsigned char       *pDisasmCode; // disassembled code

	/* control flow data */
	nodep::DWORD				dwBranchType;
	nodep::DWORD				dwBranchInstruction;

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
	nodep::DWORD historySize, logHashSize;

	RiverBasicBlockCache();
	~RiverBasicBlockCache();

	bool Init(RiverHeap *hp, nodep::DWORD logHSize, nodep::DWORD histSize);
	bool Destroy();

	RiverBasicBlock *NewBlock(nodep::UINT_PTR addr);
	RiverBasicBlock *FindBlock(nodep::UINT_PTR addr);

	typedef void(*BlockCallback)(void *, RiverBasicBlock *);
	void ForEachBlock(void *ctx, BlockCallback cb);
};

//RiverBasicBlock *NewBlock(struct _exec_env *pEnv);
//RiverBasicBlock *FindBlock(struct _exec_env *pEnv, unsigned long);

//void AddBlock(struct _exec_env *pEnv, RiverBasicBlock*);

void PrintHistory(struct ExecutionEnvironment *pEnv);
nodep::DWORD DumpHistory(struct ExecutionEnvironment *pEnv, unsigned char *o, unsigned long s, unsigned long *sz);

#endif

