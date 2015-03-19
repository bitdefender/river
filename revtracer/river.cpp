#include "river.h"
#include "extern.h"

#include "modrm32.h"
#include "riverinternl.h"
#include "CodeGen.h"

void InitRiverInstruction(RiverCodeGen *cg, RiverInstruction *ri, BYTE **px86, DWORD *pFlags);
void ConvertRiverInstruction(RiverCodeGen *cg, RiverRuntime *rt, RiverInstruction *ri, BYTE **px86, DWORD *pFlags);
void EndRiverConversion(RiverRuntime *rt, BYTE **px86, DWORD *pFlags);
void RiverPrintInstruction(RiverInstruction *ri);

void TranslateSave(RiverCodeGen *cg, RiverInstruction *rIn, RiverInstruction *rOut, DWORD *outCount);
void TranslateReverse(RiverCodeGen *cg, RiverInstruction *rIn, RiverInstruction *rOut, DWORD *outCount);

/* x86toriver converts a single x86 intruction to one ore more river instructions */
/* returns the instruction length */
/* dwInstrCount contains the number of generated river instructions */
DWORD x86toriver(RiverCodeGen *cg, BYTE *px86, struct RiverInstruction *pRiver, DWORD *dwInstrCount) {
	BYTE *pTmp = px86;
	DWORD pFlags = 0;
	*dwInstrCount = 0;

	cg->fwInstCount = 0;

	DbgPrint("= x86 to river ================================================================\n");

	do {
		BYTE *pAux = pTmp;
		DWORD iSize;
		InitRiverInstruction(cg, pRiver, &pTmp, &pFlags);
		TranslateSave(cg, pRiver, &cg->fwRiverInst[cg->fwInstCount], &cg->fwInstCount);

		DbgPrint(".%08x    ", pAux);
		iSize = pTmp - pAux;
		for (DWORD i = 0; i < iSize; ++i) {
			DbgPrint("%02x ", pAux[i]);
		}

		for (DWORD i = iSize; i < 8; ++i) {
			DbgPrint("   ");
		}

		RiverPrintInstruction(pRiver);

		(*dwInstrCount)++;
		pRiver++;
	} while (!(pFlags & RIVER_FLAG_BRANCH));

	DbgPrint("===============================================================================\n");
	return pTmp - px86;
}

/* rivertox86 converts a block of river instructions to x86 */
/* returns the nuber of bytes written in px86 */
DWORD rivertox86(RiverCodeGen *cg, RiverRuntime *rt, struct RiverInstruction *pRiver, DWORD dwInstrCount, BYTE *px86, DWORD flg) {
	BYTE *pTmp = px86, *pAux;
	DWORD pFlags = flg;

	DbgPrint("= river to x86 ================================================================\n");

	for (DWORD i = 0; i < dwInstrCount; ++i) {
		pAux = pTmp;

		//pFlags = flg;
		ConvertRiverInstruction(cg, rt, &pRiver[i], &pTmp, &pFlags);

		for (; pAux < pTmp; ++pAux) {
			DbgPrint("%02x ", *pAux);
		}
		DbgPrint("\n");
	}

	pAux = pTmp;
	EndRiverConversion(rt, &pTmp, &pFlags);
	for (; pAux < pTmp; ++pAux) {
		DbgPrint("%02x ", *pAux);
	}
	DbgPrint("\n");

	DbgPrint("===============================================================================\n");
	return pTmp - px86;
}

