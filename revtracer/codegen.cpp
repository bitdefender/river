#include "CodeGen.h"

#include "common.h"
#include "cb.h"
#include "mm.h"
#include "modrm32.h"
#include "translatetbl.h"
#include "crc32.h"
#include "revtracer.h"

#include "river.h"

#include "RiverX86Disassembler.h"
#include "RiverReverseTranslator.h"
#include "RiverSaveTranslator.h"

/* x86toriver converts a single x86 intruction to one ore more river instructions */
/* returns the instruction length */
/* dwInstrCount contains the number of generated river instructions */
DWORD x86toriver(RiverCodeGen *cg, RiverX86Disassembler &dis, RiverSaveTranslator &save, BYTE *px86, struct RiverInstruction *pRiver, DWORD *dwInstrCount);

/* rivertox86 converts a block of river instructions to x86 */
/* returns the nuber of bytes written in px86 */
DWORD rivertox86(RiverCodeGen *pEnv, RiverRuntime *rt, RiverInstruction *pRiver, DWORD dwInstrCount, BYTE *px86, DWORD flags);

RiverCodeGen::RiverCodeGen() {
	outBufferSize = 0;
	outBuffer = NULL;
	heap = NULL;
}

RiverCodeGen::~RiverCodeGen() {
	if (0 != outBufferSize) {
		Destroy();
	}
}

bool RiverCodeGen::Init(RiverHeap *hp, RiverRuntime *rt, DWORD buffSz) {
	heap = hp;
	if (NULL == (outBuffer = (unsigned char *)revtracerAPI.memoryAllocFunc(buffSz))) {
		return false;
	}
	outBufferSize = buffSz;
	codeBuffer.Init(outBuffer);

	disassembler.Init(this);

	metaTranslator.Init(this);

	revTranslator.Init(this);
	saveTranslator.Init(this);

	symbopTranslator.Init(this);

	return assembler.Init(rt);
}

bool RiverCodeGen::Destroy() {
	revtracerAPI.memoryFreeFunc(outBuffer);
	outBuffer = NULL;
	outBufferSize = 0;
	heap = NULL;
	return true;
}

void RiverCodeGen::Reset() {
	addrCount = trInstCount = fwInstCount = bkInstCount = 0;
	symbopInstCount = 0;
	memset(regVersions, 0, sizeof(regVersions));
}


static int callCount = 0;
struct RiverAddress *RiverCodeGen::AllocAddr(WORD flags) {
	struct RiverAddress *ret = &trRiverAddr[addrCount];
	callCount++;
	addrCount++;
	return ret;
}

RiverAddress *RiverCodeGen::CloneAddress(const RiverAddress &mem, WORD flags) {
	struct RiverAddress *ret = AllocAddr(flags);
	memcpy(ret, &mem, sizeof(*ret));
	return ret;
}


unsigned int RiverCodeGen::GetCurrentReg(unsigned char regName) const {
	if (RIVER_REG_NONE == (regName)) return regName;
	BYTE rTmp = regName & 0x07;
	return regVersions[rTmp] | regName;
}

unsigned int RiverCodeGen::GetPrevReg(unsigned char regName) const {
	if (RIVER_REG_NONE == (regName)) return regName;
	BYTE rTmp = regName & 0x07;
	return (regVersions[rTmp] - 0x100) | regName;
}

unsigned int RiverCodeGen::NextReg(unsigned char regName) {
	regVersions[regName] += 0x100;
	return GetCurrentReg(regName);
}




DWORD dwTransLock = 0;

extern "C" {
	void __stdcall BranchHandler(struct _exec_env *pEnv, ADDR_TYPE a);
	void __stdcall SysHandler(struct _exec_env *pEnv);
};

DWORD dwSysHandler    = (DWORD) ::SysHandler;
DWORD dwBranchHandler = (DWORD) ::BranchHandler;

unsigned char *DuplicateBuffer(RiverHeap *h, unsigned char *p, unsigned int sz) {
	unsigned int mSz = (sz + 0x0F) & ~0x0F;
	
	unsigned char *pBuf = (unsigned char *)h->Alloc(mSz);
	if (pBuf == NULL) {
		return NULL;
	}

	memcpy(pBuf, p, sz);
	memset(pBuf + sz, 0x90, mSz - sz);
	return pBuf;
}

unsigned char *ConsolidateBlock(RiverHeap *h, unsigned char *outBuff, unsigned int outSz, unsigned char *saveBuff, unsigned int saveSz) {
	unsigned int mSz = (outSz + saveSz + 0x0F) & (~0x0F);

	unsigned char *pBuf = (unsigned char *)h->Alloc(mSz);
	if (NULL == pBuf) {
		return NULL;
	}

	memcpy(pBuf, saveBuff, saveSz);
	memcpy(&pBuf[saveSz], outBuff, outSz);
	memset(&pBuf[saveSz + outSz], 0x90, mSz - outSz - saveSz);
	return pBuf;
}

void MakeJMP(struct RiverInstruction *ri, DWORD jmpAddr) {
	ri->modifiers = 0;
	ri->specifiers = 0;
	ri->family = 0;
	ri->opCode = 0xE9;
	ri->opTypes[0] = RIVER_OPTYPE_IMM | RIVER_OPSIZE_32;
	ri->operands[0].asImm32 = jmpAddr;

	ri->opTypes[1] = RIVER_OPTYPE_IMM | RIVER_OPSIZE_32;
	ri->operands[1].asImm32 = 0;
}

void RiverPrintInstruction(RiverInstruction *ri);

DWORD RiverCodeGen::TranslateBasicBlock(BYTE *px86, DWORD &dwInst) {
	BYTE *pTmp = px86;
	DWORD pFlags = 0;

	revtracerAPI.dbgPrintFunc("= x86 to river ================================================================\n");

	RiverInstruction dis, disMeta[16];
	RiverInstruction symbopMain[16]; // , symbopTrack[16];
	DWORD svCount, metaCount; // , trackCount;

	do {
		BYTE *pAux = pTmp;
		DWORD iSize, mSize = 0;
		disassembler.Translate(pTmp, dis, pFlags);
		metaTranslator.Translate(dis, disMeta, mSize);

		svCount = 0;
		//saveTranslator.Translate(dis, &fwRiverInst[fwInstCount], fwInstCount);
		//saveTranslator.Translate(dis, disSave, svCount);

		metaCount = 0;
		//trackCount = 0;
		//symbopInstCount = 0;
		for (DWORD i = 0; i < mSize; ++i) {
			//saveTranslator.Translate(disMeta[i], &fwRiverInst[fwInstCount], fwInstCount);
			saveTranslator.Translate(disMeta[i], &symbopMain[svCount], svCount);
		}

		for (DWORD i = 0; i < svCount; ++i) {
			symbopTranslator.Translate(symbopMain[i], &fwRiverInst[fwInstCount], fwInstCount, &symbopInst[symbopInstCount], symbopInstCount);
		}

		for (DWORD i = 0; i < mSize; ++i) {
			if (RIVER_FAMILY_NATIVE == RIVER_FAMILY(disMeta[i].family)) {

				revtracerAPI.dbgPrintFunc(".%08x    ", pAux);
				iSize = pTmp - pAux;
				for (DWORD i = 0; i < iSize; ++i) {
					revtracerAPI.dbgPrintFunc("%02x ", pAux[i]);
				}

				for (DWORD i = iSize; i < 8; ++i) {
					revtracerAPI.dbgPrintFunc("   ");
				}
			} else {
				revtracerAPI.dbgPrintFunc(".                                    ");
			}
			RiverPrintInstruction(&disMeta[i]);
		}

		dwInst++;
	} while (!(pFlags & RIVER_FLAG_BRANCH));

	revtracerAPI.dbgPrintFunc("===============================================================================\n");
	return pTmp - px86;
}

void GetSerializableInstruction(const RiverInstruction &rI, RiverInstruction &rO) {
	memcpy(&rO, &rI, sizeof(rI));

	for (int i = 0; i < 4; ++i) {
		if (RIVER_OPTYPE(rO.opTypes[i]) == RIVER_OPTYPE_MEM) {
			rO.operands[i].asAddress = NULL;
		}
	}
}

namespace rev {
	BOOL Kernel32WriteFile(
		HANDLE hFile,
		void *lpBuffer,
		DWORD nNumberOfBytesToWrite,
		DWORD *lpNumberOfBytesWritten
		);
};

bool SerializeInstructions(const RiverInstruction *code, int count) {
	RiverInstruction tmp;
	DWORD dwWr;
	for (int i = 0; i < count; ++i) {
		GetSerializableInstruction(code[i], tmp);
		if (0 == rev::Kernel32WriteFile(revtracerConfig.hBlocks, &tmp, sizeof(tmp), &dwWr)) {
			//debug print something
			return false;
		}

		for (int j = 0; j < 4; ++j) {
			if (RIVER_OPTYPE(code[i].opTypes[j]) == RIVER_OPTYPE_MEM) {
				if (0 == rev::Kernel32WriteFile(revtracerConfig.hBlocks, &code[i].operands[j].asAddress->type, sizeof(*code[i].operands[j].asAddress) - sizeof(void *), &dwWr)) {
					//debug print something
					return false;
				}
			}
		}
	}
	return true;
}

bool SaveToStream(
	const RiverInstruction *forwardCode, DWORD dwFwOpCount, 
	const RiverInstruction *backwardCode, DWORD dwBkOpCount,
	const RiverInstruction *trackCode, DWORD dwTrOpCount
) {
	DWORD header[4] = { 'BBVR', dwFwOpCount, dwBkOpCount, dwTrOpCount };
	DWORD dwWr;

	if (0 == Kernel32WriteFile(revtracerConfig.hBlocks, header, sizeof(header), &dwWr)) {
		//debug print something
		return false;
	}

	if (
		!SerializeInstructions(forwardCode, dwFwOpCount) ||
		!SerializeInstructions(backwardCode, dwBkOpCount) ||
		!SerializeInstructions(trackCode, dwTrOpCount)
		) {
		return false;
	}

	return true;
}

bool RiverCodeGen::Translate(RiverBasicBlock *pCB, DWORD dwTranslationFlags) {
	if (dwTranslationFlags & 0x80000000) {
		pCB->dwSize = 0;
		pCB->dwCRC = (DWORD)crc32(0xEDB88320, (BYTE *)pCB->address, 0);

		outBufferSize = 0;
		pCB->pCode = pCB->pFwCode = (unsigned char *)pCB->address;
		pCB->pBkCode = NULL;

		return true;
	} else {

		Reset();

		pCB->dwOrigOpCount = 0;
		pCB->dwSize = TranslateBasicBlock((BYTE *)pCB->address, pCB->dwOrigOpCount); //(this, disassembler, saveTranslator, (BYTE *)pCB->address, fwRiverInst, &pCB->dwOrigOpCount);
		trInstCount += pCB->dwOrigOpCount;
		pCB->dwCRC = (DWORD)crc32(0xEDB88320, (BYTE *)pCB->address, pCB->dwSize);

		for (DWORD i = 0; i < fwInstCount; ++i) {
			//TranslateReverse(this, &fwRiverInst[fwInstCount - 1 - i], &bkRiverInst[i], &tmp);
			revTranslator.Translate(fwRiverInst[fwInstCount - 1 - i], bkRiverInst[i]);
		}
		MakeJMP(&bkRiverInst[fwInstCount], pCB->address);

		revtracerAPI.dbgPrintFunc("= SymbopTrack =================================================================\n");
		for (unsigned int i = 0; i < symbopInstCount; ++i) {
			RiverPrintInstruction(&symbopInst[i]);
		}
		revtracerAPI.dbgPrintFunc("===============================================================================\n");

		bkInstCount = fwInstCount + 1;

		if (revtracerConfig.dumpBlocks) {
			SaveToStream(fwRiverInst, fwInstCount, bkRiverInst, bkInstCount, symbopInst, symbopInstCount);
		}

		//outBufferSize = rivertox86(this, rt, trRiverInst, trInstCount, outBuffer, 0x00);
		//assembler.Assemble(trRiverInst, trInstCount, outBuffer, 0x00, pCB->, outBufferSize);
		//pCB->pCode = DuplicateBuffer(heap, outBuffer, outBufferSize);

		//outBufferSize = rivertox86(this, rt, fwRiverInst, fwInstCount, outBuffer, 0x01);
		codeBuffer.Reset();
		assembler.Assemble(fwRiverInst, fwInstCount, codeBuffer, 0x10, pCB->dwFwOpCount, outBufferSize);
		pCB->pFwCode = DuplicateBuffer(heap, outBuffer, outBufferSize);
		//assembler.CopyFix(pCB->pFwCode, outBuffer);
		codeBuffer.CopyToFixed(pCB->pFwCode);
		
		//outBufferSize = rivertox86(this, rt, bkRiverInst, bkInstCount, outBuffer, 0x00);
		codeBuffer.Reset();
		assembler.Assemble(bkRiverInst, bkInstCount, codeBuffer, 0x00, pCB->dwBkOpCount, outBufferSize);
		pCB->pBkCode = DuplicateBuffer(heap, outBuffer, outBufferSize);
		//assembler.CopyFix(pCB->pBkCode, outBuffer);
		codeBuffer.CopyToFixed(pCB->pBkCode);

		codeBuffer.Reset();
		assembler.AssembleTracking(symbopInst, symbopInstCount, codeBuffer, 0x10, pCB->dwTrOpCount, outBufferSize);
		pCB->pTrackCode = DuplicateBuffer(heap, outBuffer, outBufferSize);
		codeBuffer.CopyToFixed(pCB->pTrackCode);

		return true;
	}
}
