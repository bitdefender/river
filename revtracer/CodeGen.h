#ifndef _CODEGEN_H
#define _CODEGEN_H

#include "river.h"
#include "cb.h"
#include "runtime.h"

#include "RiverX86Disassembler.h"

#include "X86Assembler.h"

#include "RiverMetaTranslator.h"

#include "RiverReverseTranslator.h"
#include "RiverSaveTranslator.h"

#include "SymbopTranslator.h"
#include "SymbopSaveTranslator.h"
#include "SymbopReverseTranslator.h"

#define RIVER_TRANSLATE_INSTRUCTIONS				128
#define RIVER_FORWARD_INSTRUCTIONS					512
#define RIVER_BACKWARD_INSTRUCTIONS					512

#define SYMBOP_TRACK_INSTRUCTIONS					512
#define SYMBOP_TRACK_FORWARD_INSTRUCTIONS			512
#define SYMBOP_TRACK_BACKWARD_INSTRUCTIONS			512

/* A resettable river code translator */
class RiverCodeGen {
private :
	RiverHeap *heap;
	
	RiverX86Disassembler disassembler;
	X86Assembler assembler;

	RiverMetaTranslator metaTranslator;

	RiverReverseTranslator revTranslator;
	RiverSaveTranslator saveTranslator;

	SymbopTranslator symbopTranslator;
	SymbopSaveTranslator symbopSaveTranslator;
	SymbopReverseTranslator symbopReverseTranslator;

	DWORD RiverCodeGen::TranslateBasicBlock(BYTE *px86, DWORD &dwInst, DWORD dwTranslationFlags);
public :
	//struct RiverInstruction trRiverInst[RIVER_TRANSLATE_INSTRUCTIONS]; // to be removed in the near future
	struct RiverInstruction fwRiverInst[RIVER_FORWARD_INSTRUCTIONS];
	struct RiverInstruction bkRiverInst[RIVER_BACKWARD_INSTRUCTIONS];
	struct RiverInstruction symbopInst[SYMBOP_TRACK_INSTRUCTIONS];
	struct RiverInstruction symbopFwRiverInst[SYMBOP_TRACK_FORWARD_INSTRUCTIONS];
	struct RiverInstruction symbopBkRiverInst[SYMBOP_TRACK_BACKWARD_INSTRUCTIONS];

	struct RiverAddress32 trRiverAddr[1024];
	
	DWORD trInstCount, fwInstCount, bkInstCount, /*srInstCount,*/ sfInstCount, sbInstCount, addrCount, outBufferSize;
	DWORD symbopInstCount;

	unsigned char *outBuffer;
	unsigned int regVersions[8];

	RelocableCodeBuffer codeBuffer;
public :
	RiverCodeGen();
	~RiverCodeGen();

	bool Init(RiverHeap *hp, RiverRuntime *rt, DWORD buffSz, DWORD dwTranslationFlags);
	bool Destroy();
	void Reset();

	struct RiverAddress *AllocAddr(WORD flags);
	struct RiverAddress *CloneAddress(const RiverAddress &mem, WORD flags);

	unsigned int GetCurrentReg(unsigned char regName) const;
	unsigned int GetPrevReg(unsigned char regName) const;

	unsigned int NextReg(unsigned char regName);

	bool Translate(RiverBasicBlock *pCB, DWORD dwTranslationFlags);
	bool DisassembleSingle(BYTE *&px86, RiverInstruction *rOut, DWORD &count, DWORD &dwFlags);
};

#endif
