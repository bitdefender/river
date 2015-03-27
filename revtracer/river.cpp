#include "river.h"
#include "extern.h"

#include "modrm32.h"
#include "riverinternl.h"
#include "CodeGen.h"

#include "RiverX86Disassembler.h"
#include "RiverSaveTranslator.h"

//void InitRiverInstruction(RiverCodeGen *cg, RiverInstruction *ri, BYTE **px86, DWORD *pFlags);
void ConvertRiverInstruction(RiverCodeGen *cg, RiverRuntime *rt, RiverInstruction *ri, BYTE **px86, DWORD *pFlags);
void EndRiverConversion(RiverRuntime *rt, BYTE **px86, DWORD *pFlags);
void RiverPrintInstruction(RiverInstruction *ri);

void TranslateReverse(RiverCodeGen *cg, RiverInstruction *rIn, RiverInstruction *rOut, DWORD *outCount);

/* x86toriver converts a single x86 intruction to one ore more river instructions */
/* returns the instruction length */
/* dwInstrCount contains the number of generated river instructions */
DWORD x86toriver(RiverCodeGen *cg, RiverX86Disassembler &dis, RiverSaveTranslator &save, BYTE *px86, struct RiverInstruction *pRiver, DWORD *dwInstrCount) {
	BYTE *pTmp = px86;
	DWORD pFlags = 0;
	*dwInstrCount = 0;

	cg->fwInstCount = 0;

	DbgPrint("= x86 to river ================================================================\n");

	do {
		BYTE *pAux = pTmp;
		DWORD iSize;
		//InitRiverInstruction(cg, pRiver, &pTmp, &pFlags);
		dis.Translate(pTmp, *pRiver, pFlags);
		//TranslateSave(cg, pRiver, &cg->fwRiverInst[cg->fwInstCount], &cg->fwInstCount);
		save.Translate(*pRiver, &cg->fwRiverInst[cg->fwInstCount], cg->fwInstCount);


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

