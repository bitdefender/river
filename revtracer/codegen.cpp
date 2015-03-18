#include "execenv.h"

#include "common.h"
#include "cb.h"
#include "mm.h"
#include "modrm32.h"
#include "translatetbl.h"
#include "crc32.h"
#include "extern.h"

#include "river.h"

DWORD dwTransLock = 0;

DWORD dwSysHandler    = (DWORD) SysHandler;
DWORD dwSysEndHandler = (DWORD) SysEndHandler;
DWORD dwBranchHandler = (DWORD) BranchHandler;

void TranslateReverse(struct _exec_env *pEnv, struct RiverInstruction *rIn, struct RiverInstruction *rOut, DWORD *outCount);

unsigned char *DuplicateBuffer(struct _exec_env *pEnv, unsigned char *p, unsigned int sz) {
	unsigned int mSz = (sz + 0x0F) & ~0x0F;
	
	unsigned char *pBuf = (unsigned char *)pEnv->heap.Alloc(mSz);
	if (pBuf == NULL) {
		return NULL;
	}

	memcpy(pBuf, p, sz);
	memset(pBuf + sz, 0x90, mSz - sz);
	return pBuf;
}

unsigned char *ConsolidateBlock(struct _exec_env *pEnv, unsigned char *outBuff, unsigned int outSz, unsigned char *saveBuff, unsigned int saveSz) {
	unsigned int mSz = (outSz + saveSz + 0x0F) & (~0x0F);

	unsigned char *pBuf = (unsigned char *)pEnv->heap.Alloc(mSz);
	if (NULL == pBuf) {
		return NULL;
	}

	memcpy(pBuf, saveBuff, saveSz);
	memcpy(&pBuf[saveSz], outBuff, outSz);
	memset(&pBuf[saveSz + outSz], 0x90, mSz - outSz - saveSz);
	return pBuf;
}

void MakeJMP(struct _exec_env *pEnv, struct RiverInstruction *ri, DWORD jmpAddr) {
	ri->modifiers = 0;
	ri->specifiers = 0;
	ri->opCode = 0xE9;
	ri->opTypes[0] = RIVER_OPTYPE_IMM | RIVER_OPSIZE_32;
	ri->operands[0].asImm32 = jmpAddr;

	ri->opTypes[1] = RIVER_OPTYPE_IMM | RIVER_OPSIZE_32;
	ri->operands[1].asImm32 = 0;

}

int Translate(struct _exec_env *pEnv, struct _cb_info *pCB, DWORD dwTranslationFlags) {
	DWORD tmp;

	if (dwTranslationFlags & 0x80000000) {
		pCB->dwSize = 0;
		pCB->dwCRC = (DWORD)crc32(0xEDB88320, (BYTE *)pCB->address, 0);

		pEnv->outBufferSize = 0;
		pCB->pCode = pCB->pFwCode = (unsigned char *)pCB->address;
		pCB->pBkCode = NULL;

		return pEnv->outBufferSize;
	} else {

		RiverMemReset(pEnv);
		ResetRegs(pEnv);

		pCB->dwSize = x86toriver(pEnv, (BYTE *)pCB->address, pEnv->trRiverInst, &pEnv->trInstCount);
		pCB->dwCRC = (DWORD)crc32(0xEDB88320, (BYTE *)pCB->address, pCB->dwSize);

		for (DWORD i = 0; i < pEnv->fwInstCount; ++i) {
			TranslateReverse(pEnv, &pEnv->fwRiverInst[pEnv->fwInstCount - 1 - i], &pEnv->bkRiverInst[i], &tmp);
		}
		MakeJMP(pEnv, &pEnv->bkRiverInst[pEnv->fwInstCount], pCB->address);

		pEnv->bkInstCount = pEnv->fwInstCount + 1;

		pEnv->outBufferSize = rivertox86(pEnv, pEnv->trRiverInst, pEnv->trInstCount, pEnv->outBuffer, 0x00);
		pCB->pCode = DuplicateBuffer(pEnv, pEnv->outBuffer, pEnv->outBufferSize);

		pEnv->outBufferSize = rivertox86(pEnv, pEnv->fwRiverInst, pEnv->fwInstCount, pEnv->outBuffer, 0x01);
		pCB->pFwCode = DuplicateBuffer(pEnv, pEnv->outBuffer, pEnv->outBufferSize);
		
		pEnv->outBufferSize = rivertox86(pEnv, pEnv->bkRiverInst, pEnv->bkInstCount, pEnv->outBuffer, 0x00);
		pCB->pBkCode = DuplicateBuffer(pEnv, pEnv->outBuffer, pEnv->outBufferSize);


		return pEnv->outBufferSize;
	}
}
