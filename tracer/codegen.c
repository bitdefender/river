#include "execenv.h"

#include "common.h"
#include "cb.h"
#include "mm.h"
#include "modrm32.h"
#include "codetbl.h"
#include "crc32.h"
#include "extern.h"

//static BYTE poutBuffer [0x100000] = { 0 };


/*extern DWORD dwVirtualStack;
extern DWORD dwEaxSave;
extern DWORD dwJmpBuf;*/


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

int Translate(struct _exec_env *pEnv, struct _cb_info *pCB, DWORD dwTranslationFlags) {
	BYTE		*pInstruction;	// pointer catre instructiunea curenta
	BYTE		*pDestination;	// pointer to the destination area

	DWORD		dwI, dwJ, dwFlags, dwResult, dwWritten, i;

	dwResult = 0;

	DbgPrint("Starting analysis from address %08X\n", pCB->address);
	DbgPrint("-------------------------------------------------------------------\n");


	dwI	= 0;
	dwJ = 0; 

	pCB->dwParses  = dwTranslationFlags;
	pCB->pCode     = pEnv->outBuffer;
	
	if ((dwTranslationFlags & FLAG_BRANCH) == FLAG_BRANCH) {
		dwJ = AddSysEndPrefix(pEnv, pEnv->outBuffer);
	}

	do{
		dwFlags 	= FLAG_NONE;

		DbgPrint(".%08X\t", pCB->address + dwI);

		do{
			dwFlags         &= ~FLAG_PFX;

			pInstruction    = (BYTE *) (pCB->address + dwI);
			pDestination    = (BYTE *) (pCB->pCode     + dwJ);


			if (dwFlags & FLAG_EXT) {
				dwResult = Table0F [*pInstruction] (pEnv, pCB, &dwFlags, pInstruction, pDestination, &dwWritten);
			} else {
				dwResult = Table00 [*pInstruction] (pEnv, pCB, &dwFlags, pInstruction, pDestination, &dwWritten);
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
			dwJ += dwWritten;

		} while (dwFlags & FLAG_PFX);

		DbgPrint("\n");

	} while ((dwFlags & FLAG_BRANCH) != FLAG_BRANCH);

	pCB->pCode  = DuplicateBuffer(pEnv, pEnv->outBuffer, dwJ);
	pCB->dwCRC  = (DWORD)crc32(0xEDB88320, (BYTE *)pCB->address, dwI);
	pCB->dwSize = dwI;

	if (!pCB->pCode) {
		// jumping out!
		pCB->pCode = (BYTE *) pCB->address;
	}

	return dwJ;
}


