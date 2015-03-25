#ifndef _CODEGEN_H
#define _CODEGEN_H

#include "river.h"
#include "cb.h"
#include "runtime.h"

#include "RiverX86Assembler.h"

#define RIVER_TRANSLATE_INSTRUCTIONS				128
#define RIVER_FORWARD_INSTRUCTIONS					512
#define RIVER_BACKWARD_INSTRUCTIONS					512

/* A resettable river code translator */
class RiverCodeGen {
private :
	RiverHeap *heap;
	
	RiverX86Assembler assembler;
public :
	struct RiverInstruction trRiverInst[RIVER_TRANSLATE_INSTRUCTIONS]; // to be removed in the near future
	struct RiverInstruction fwRiverInst[RIVER_FORWARD_INSTRUCTIONS];
	struct RiverInstruction bkRiverInst[RIVER_BACKWARD_INSTRUCTIONS];
	struct RiverAddress32 trRiverAddr[512];
	DWORD trInstCount, fwInstCount, bkInstCount, addrCount, outBufferSize;

	unsigned char *outBuffer;
	unsigned int regVersions[8];
public :
	RiverCodeGen();
	~RiverCodeGen();

	bool Init(RiverHeap *hp, RiverRuntime *rt, DWORD buffSz);
	bool Destroy();
	void Reset();

	struct RiverAddress *AllocAddr(WORD flags);

	unsigned int GetCurrentReg(unsigned char regName) const;
	unsigned int GetPrevReg(unsigned char regName) const;

	unsigned int NextReg(unsigned char regName);

	bool Translate(RiverBasicBlock *pCB, DWORD dwTranslationFlags);
};

#endif
