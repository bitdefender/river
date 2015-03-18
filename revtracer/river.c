#include "river.h"

#include "modrm32.h"
#include "riverinternl.h"

void InitRiverInstruction(struct _exec_env *pEnv, struct RiverInstruction *ri, BYTE **px86, DWORD *pFlags);
void ConvertRiverInstruction(struct _exec_env *pEnv, struct RiverInstruction *ri, BYTE **px86, DWORD *pFlags);
void EndRiverConversion(struct _exec_env *pEnv, BYTE **px86, DWORD *pFlags);
void RiverPrintInstruction(struct RiverInstruction *ri);

void TransalteSave(struct _exec_env *pEnv, struct RiverInstruction *rIn, struct RiverInstruction *rOut, DWORD *outCount);
void TranslateReverse(struct _exec_env *pEnv, struct RiverInstruction *rIn, struct RiverInstruction *rOut, DWORD *outCount);

/* x86toriver converts a single x86 intruction to one ore more river instructions */
/* returns the instruction length */
/* dwInstrCount contains the number of generated river instructions */
DWORD x86toriver(struct _exec_env *pEnv, BYTE *px86, struct RiverInstruction *pRiver, DWORD *dwInstrCount) {
	BYTE *pTmp = px86;
	DWORD pFlags = 0;
	*dwInstrCount = 0;

	pEnv->fwInstCount = 0;

	printf("= x86 to river ================================================================\n");

	do {
		BYTE *pAux = pTmp;
		DWORD iSize;
		InitRiverInstruction(pEnv, pRiver, &pTmp, &pFlags);
		TranslateSave(pEnv, pRiver, &pEnv->fwRiverInst[pEnv->fwInstCount], &pEnv->fwInstCount);

		printf(".%08x    ", pAux);
		iSize = pTmp - pAux;
		for (DWORD i = 0; i < iSize; ++i) {
			printf("%02x ", pAux[i]);
		}

		for (DWORD i = iSize; i < 8; ++i) {
			printf("   ");
		}

		RiverPrintInstruction(pRiver);

		(*dwInstrCount)++;
		pRiver++;
	} while (!(pFlags & RIVER_FLAG_BRANCH));

	printf("===============================================================================\n");
	return pTmp - px86;
}

/* rivertox86 converts a block of river instructions to x86 */
/* returns the nuber of bytes written in px86 */
DWORD rivertox86(struct _exec_env *pEnv, struct RiverInstruction *pRiver, DWORD dwInstrCount, BYTE *px86, unsigned int flg) {
	BYTE *pTmp = px86, *pAux;
	DWORD pFlags = flg;

	printf("= river to x86 ================================================================\n");

	for (DWORD i = 0; i < dwInstrCount; ++i) {
		pAux = pTmp;

		//pFlags = flg;
		ConvertRiverInstruction(pEnv, &pRiver[i], &pTmp, &pFlags);

		for (; pAux < pTmp; ++pAux) {
			printf("%02x ", *pAux);
		}
		printf("\n");
	}

	pAux = pTmp;
	EndRiverConversion(pEnv, &pTmp, &pFlags);
	for (; pAux < pTmp; ++pAux) {
		printf("%02x ", *pAux);
	}
	printf("\n");

	printf("===============================================================================\n");
	return pTmp - px86;
}

/* revir reverses the river instructions */
/* revir also marks unnecessary riverload/riversave instruction pairs */
DWORD revir(struct RiverInstruction *pRiver, DWORD dwInstrCount, struct RiverInstruction *pRevir) {
	return 0;
}



