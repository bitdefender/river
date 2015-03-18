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

char *DuplicateBuffer(struct _exec_env *pEnv, char *p, unsigned int sz) {
	unsigned int mSz = (sz + 0x0F) & ~0x0F;
	
	char *pBuf = SC_HeapAlloc(pEnv, mSz);
	if (pBuf == NULL) {
		return NULL;
	}

	memcpy(pBuf, p, sz);
	memset(pBuf + sz, 0x90, mSz - sz);
	return pBuf;
}

char *ConsolidateBlock(struct _exec_env *pEnv, char *outBuff, unsigned int outSz, char *saveBuff, unsigned int saveSz) {
	unsigned int mSz = (outSz + saveSz + 0x0F) & (~0x0F);

	char *pBuf = SC_HeapAlloc(pEnv, mSz);
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
	//DWORD dwInstCount;
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

/*int Translate(struct _exec_env *pEnv, struct _cb_info *pCB, DWORD dwTranslationFlags) {
	BYTE		*pInstruction;	// pointer to the current instruction
	BYTE		*pDestination;	// pointer to the destination area
	BYTE		*pSave;			// pointer to the oplog instructions

	DWORD		dwI, dwJ, dwS, dwFlags, dwResult, i;
	DWORD		dwOutWr, dwSaveWr;

	dwResult = 0;

	DbgPrint("Starting analysis from address %08X\n", pCB->address);
	DbgPrint("-------------------------------------------------------------------\n");


	dwI	= 0;
	dwJ = 0; 
	dwS = 0;

	pCB->dwParses  = dwTranslationFlags;
	pCB->pCode = pEnv->outBuffer;
	pCB->pSave = pEnv->saveBuffer;
	
	if ((dwTranslationFlags & FLAG_BRANCH) == FLAG_BRANCH) {
		dwJ = AddSysEndPrefix(pEnv, pEnv->outBuffer);
	}

	do {
		dwFlags 	= FLAG_NONE;

		DbgPrint(".%08X\t", pCB->address + dwI);

		do{
			dwFlags         &= ~FLAG_PFX;

			pInstruction    = (BYTE *) (pCB->address + dwI);
			pDestination    = (BYTE *) (pCB->pCode + dwJ);
			pSave			= (BYTE *) (pCB->pSave + dwS);


			if (dwFlags & FLAG_EXT) {
				dwResult = TranslateTable0F[*pInstruction](pEnv, pCB, &dwFlags, pInstruction, pDestination, &dwOutWr);
				SaveTable0F[*pInstruction](pEnv, pCB, &dwFlags, pInstruction, pSave, &dwSaveWr);
			} else {
				dwResult = TranslateTable00[*pInstruction](pEnv, pCB, &dwFlags, pInstruction, pDestination, &dwOutWr);
				SaveTable00[*pInstruction](pEnv, pCB, &dwFlags, pInstruction, pSave, &dwSaveWr);
			}


    		
			for (i = 0; i < dwResult; i ++)
			{
				DbgPrint("%02X ", pInstruction[i]);
			}
		

			if (dwResult == 0) {
				// jumping out!
				pCB->pCode = (BYTE *) pCB->address;
				return 0;
			}

			dwI += dwResult;
			dwJ += dwOutWr;
			dwS += dwSaveWr;

		} while (dwFlags & FLAG_PFX);

		DbgPrint("\n");

	} while ((dwFlags & FLAG_BRANCH) != FLAG_BRANCH);

	pCB->pCode = ConsolidateBlock(pEnv, pEnv->outBuffer, dwJ, pEnv->saveBuffer, dwS); //DuplicateBuffer(pEnv, pEnv->outBuffer, dwJ);
	pCB->dwCRC = (DWORD)crc32(0xEDB88320, (BYTE *)pCB->address, dwI);
	pCB->dwSize = dwI;

	if (!pCB->pCode) {
		// jumping out!
		pCB->pCode = (BYTE *) pCB->address;
	}

	return dwJ;
}*/


