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

#define RIVER_FORWARD_INSTRUCTIONS					1024
#define RIVER_BACKWARD_INSTRUCTIONS					1024

#define SYMBOP_TRACK_INSTRUCTIONS					1024
#define SYMBOP_TRACK_FORWARD_INSTRUCTIONS			1024
#define SYMBOP_TRACK_BACKWARD_INSTRUCTIONS			1024

#define TRANSLATION_EXIT							0x80000000
#define TRANSLATION_HOOK							0x40000000

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

	rev::DWORD TranslateBasicBlock(rev::BYTE *px86, rev::DWORD &dwInst, rev::BYTE *&disasm, rev::DWORD dwTranslationFlags);
public :
	struct RiverInstruction fwRiverInst[RIVER_FORWARD_INSTRUCTIONS];
	struct RiverInstruction bkRiverInst[RIVER_BACKWARD_INSTRUCTIONS];
	struct RiverInstruction symbopInst[SYMBOP_TRACK_INSTRUCTIONS];
	struct RiverInstruction symbopFwRiverInst[SYMBOP_TRACK_FORWARD_INSTRUCTIONS];
	struct RiverInstruction symbopBkRiverInst[SYMBOP_TRACK_BACKWARD_INSTRUCTIONS];

	struct RiverAddress32 trRiverAddr[16384];
	
	rev::DWORD trInstCount, fwInstCount, bkInstCount, /*srInstCount,*/ sfInstCount, sbInstCount, addrCount, outBufferSize;
	rev::DWORD symbopInstCount;

	unsigned char *outBuffer;
	unsigned int regVersions[8];

	RelocableCodeBuffer codeBuffer;
public :
	RiverCodeGen();
	~RiverCodeGen();

	bool Init(RiverHeap *hp, RiverRuntime *rt, rev::DWORD buffSz, rev::DWORD dwTranslationFlags);
	bool Destroy();
	void Reset();

	struct RiverAddress *AllocAddr(rev::WORD flags);
	struct RiverAddress *CloneAddress(const RiverAddress &mem, rev::WORD flags);

	unsigned int GetCurrentReg(unsigned char regName) const;
	unsigned int GetPrevReg(unsigned char regName) const;

	unsigned int NextReg(unsigned char regName);

	bool Translate(RiverBasicBlock *pCB, rev::DWORD dwTranslationFlags);
	bool DisassembleSingle(rev::BYTE *&px86, RiverInstruction *rOut, rev::DWORD &count, rev::DWORD &dwFlags, rev::DWORD index);
};

#endif
