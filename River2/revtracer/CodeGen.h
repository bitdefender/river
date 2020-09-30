#ifndef _CODEGEN_H
#define _CODEGEN_H

#include "river.h"
#include "cb.h"
#include "Runtime.h"

#include "RiverX86Disassembler.h"

#include "X86Assembler.h"

#include "RiverMetaTranslator.h"

#include "RiverRepTranslator.h"

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

/* A resettable river code translator */
class RiverCodeGen {
private :
	RiverHeap *heap;
	
	RiverX86Disassembler disassembler;
	X86Assembler assembler;

	RiverMetaTranslator metaTranslator;

	RiverRepTranslator repTranslator;

	RiverReverseTranslator revTranslator;
	RiverSaveTranslator saveTranslator;

	SymbopTranslator symbopTranslator;
	SymbopSaveTranslator symbopSaveTranslator;
	SymbopReverseTranslator symbopReverseTranslator;

	nodep::DWORD TranslateBasicBlock(nodep::BYTE *px86,
			nodep::DWORD &dwInst, nodep::BYTE *&disasm,
			nodep::DWORD dwTranslationFlags, nodep::DWORD *disassFlags,
			struct rev::BranchNext *next, RevtracerError *rerror);
public :
	struct RiverInstruction fwRiverInst[RIVER_FORWARD_INSTRUCTIONS];
	struct RiverInstruction bkRiverInst[RIVER_BACKWARD_INSTRUCTIONS];
	struct RiverInstruction symbopInst[SYMBOP_TRACK_INSTRUCTIONS];
	struct RiverInstruction symbopFwRiverInst[SYMBOP_TRACK_FORWARD_INSTRUCTIONS];
	struct RiverInstruction symbopBkRiverInst[SYMBOP_TRACK_BACKWARD_INSTRUCTIONS];

	struct RiverAddress32 trRiverAddr[16384];
	
	nodep::DWORD trInstCount, fwInstCount, bkInstCount, /*srInstCount,*/ sfInstCount, sbInstCount, addrCount, outBufferSize;
	nodep::DWORD symbopInstCount;

	unsigned char *outBuffer;
	unsigned int regVersions[8];

	RelocableCodeBuffer codeBuffer;
public :
	RiverCodeGen();
	~RiverCodeGen();

	bool Init(RiverHeap *hp, RiverRuntime *rt, nodep::DWORD buffSz, nodep::DWORD dwTranslationFlags);
	bool Destroy();
	void Reset();

	struct RiverAddress *AllocAddr(nodep::WORD flags);
	struct RiverAddress *CloneAddress(const RiverAddress &mem, nodep::WORD flags);

	unsigned int GetCurrentReg(unsigned char regName) const;
	unsigned int GetPrevReg(unsigned char regName) const;

	unsigned int NextReg(unsigned char regName);

	bool Translate(RiverBasicBlock *pCB, nodep::DWORD dwTranslationFlags, RevtracerError *rerror);
	bool DisassembleSingle(nodep::BYTE *&px86, RiverInstruction *rOut, nodep::DWORD &count, nodep::DWORD &dwFlags, RevtracerError *rerror);
};

#endif
