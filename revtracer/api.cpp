#include "revtracer.h"

#include "common.h"
#include "execenv.h"
#include "river.h"

#include "AddressContainer.h"
#include "Tracking.h"

using namespace nodep;

#define PRINT_RUNTIME_TRACKING	PRINT_INFO | PRINT_RUNTIME | PRINT_TRACKING
#define PRINT_BRANCHING_ERROR	PRINT_ERROR | PRINT_BRANCH_HANDLER
#define PRINT_BRANCHING_DEBUG	PRINT_DEBUG | PRINT_BRANCH_HANDLER
#define PRINT_BRANCHING_INFO	PRINT_INFO | PRINT_BRANCH_HANDLER

#define DEBUG_BREAK_ { \
	*(WORD *)revtracerConfig.mainModule = 'ZM'; \
	DEBUG_BREAK; \
	*(WORD *)revtracerConfig.mainModule = '  '; \
}

DWORD dwAddressTrackHandler = (DWORD)&rev::TrackAddr;
DWORD dwAddressMarkHandler = (DWORD)&rev::MarkAddr;

void RiverPrintInstruction(DWORD printMask, RiverInstruction *ri);

extern "C" {
	void PushToExecutionBuffer(ExecutionEnvironment *pEnv, DWORD value) {
		pEnv->runtimeContext.execBuff -= 4;
		*((DWORD *)pEnv->runtimeContext.execBuff) = value;
	}

	DWORD PopFromExecutionBuffer(ExecutionEnvironment *pEnv) {
		DWORD ret = *((DWORD *)pEnv->runtimeContext.execBuff);
		pEnv->runtimeContext.execBuff += 4;
		return ret;
	}

	DWORD TopFromExecutionBuffer(ExecutionEnvironment *pEnv) {
		return *((DWORD *)pEnv->runtimeContext.execBuff);
	}

	bool ExecutionBufferEmpty(ExecutionEnvironment *pEnv) {
		return (DWORD)pEnv->executionBase == pEnv->runtimeContext.execBuff;
	}

	typedef DWORD (*NtTerminateProcessFunc)(
		DWORD ProcessHandle,
		DWORD ExitStatus
	);

	void __stdcall BranchHandler(ExecutionEnvironment *pEnv, ADDR_TYPE a) {
		//ExecutionRegs *currentRegs = (ExecutionRegs *)((&a) + 1);

		pEnv->runtimeContext.registers = (UINT_PTR)((&a) + 1);
		RiverBasicBlock *pCB;
		pEnv->runtimeContext.trackBuff = pEnv->runtimeContext.trackBase;

		if (pEnv->bForward) {
			PushToExecutionBuffer(pEnv, pEnv->lastFwBlock);
		}

		
		DWORD *stk = (DWORD *)pEnv->runtimeContext.virtualStack;
		BRANCHING_PRINT(PRINT_BRANCHING_DEBUG, "EIP: 0x%08x Stack :\n", a);
		BRANCHING_PRINT(PRINT_BRANCHING_DEBUG, "0x%08x: 0x%08x 0x%08x 0x%08x 0x%08x\n", stk + 0x00, stk[0], stk[1], stk[2], stk[3]);
		BRANCHING_PRINT(PRINT_BRANCHING_DEBUG, "EAX: 0x%08x  ECX: 0x%08x  EDX: 0x%08x  EBX: 0x%08x\n",
			((ExecutionRegs*)pEnv->runtimeContext.registers)->eax, 
			((ExecutionRegs*)pEnv->runtimeContext.registers)->ecx,
			((ExecutionRegs*)pEnv->runtimeContext.registers)->edx,
			((ExecutionRegs*)pEnv->runtimeContext.registers)->ebx);
		BRANCHING_PRINT(PRINT_BRANCHING_DEBUG, "ESP: 0x%08x  EBP: 0x%08x  ESI: 0x%08x  EDI: 0x%08x\n",
			((ExecutionRegs*)pEnv->runtimeContext.registers)->esp,
			((ExecutionRegs*)pEnv->runtimeContext.registers)->ebp,
			((ExecutionRegs*)pEnv->runtimeContext.registers)->esi,
			((ExecutionRegs*)pEnv->runtimeContext.registers)->edi);
		BRANCHING_PRINT(PRINT_BRANCHING_DEBUG, "Flags: 0x%08x\n",
			((ExecutionRegs*)pEnv->runtimeContext.registers)->eflags);

		DWORD dwDirection = revtracerImports.branchHandler(pEnv, pEnv->userContext, a);
		if (EXECUTION_BACKTRACK == dwDirection) {
			// go backwards
			DWORD addr = PopFromExecutionBuffer(pEnv);
			revtracerImports.dbgPrintFunc(PRINT_BRANCHING_INFO, "Going Backwards to %08X!!!\n", addr);

			revtracerImports.dbgPrintFunc(PRINT_BRANCHING_INFO, "Looking for block\n");
			pCB = pEnv->blockCache.FindBlock(addr);
			if (pCB) {
				revtracerImports.dbgPrintFunc(PRINT_BRANCHING_INFO, "Block found\n");
				pCB->MarkBackward();
				//pEnv->posHist -= 1;
				
				pEnv->bForward = 0;
				pEnv->runtimeContext.jumpBuff = (DWORD)pCB->pBkCode;
			}
			else {
				revtracerImports.dbgPrintFunc(PRINT_BRANCHING_ERROR, "No reverse block found!");
				DEBUG_BREAK;
			}

		} else if (EXECUTION_ADVANCE == dwDirection) {
			// go forwards
			revtracerImports.dbgPrintFunc(PRINT_BRANCHING_INFO, "Going Forwards from %08X!!!\n", a);
			revtracerImports.dbgPrintFunc(PRINT_BRANCHING_INFO, "Looking for block\n");
			pCB = pEnv->blockCache.FindBlock((UINT_PTR)a);
			if (pCB) {
				revtracerImports.dbgPrintFunc(PRINT_BRANCHING_INFO, "Block found\n");
			} else {
				revtracerImports.dbgPrintFunc(PRINT_BRANCHING_INFO, "Not Found\n");
				pCB = pEnv->blockCache.NewBlock((UINT_PTR)a);

				pEnv->codeGen.Translate(pCB, pEnv->generationFlags);

				TRANSLATE_PRINT(PRINT_BRANCHING_INFO, "= river saving code ===========================================================\n");
				for (DWORD i = 0; i < pEnv->codeGen.fwInstCount; ++i) {
					TRANSLATE_PRINT_INSTRUCTION(PRINT_BRANCHING_INFO, &pEnv->codeGen.fwRiverInst[i]);
				}
				TRANSLATE_PRINT(PRINT_BRANCHING_INFO, "===============================================================================\n");

				if (TRACER_FEATURE_REVERSIBLE & pEnv->generationFlags) {
					TRANSLATE_PRINT(PRINT_BRANCHING_INFO, "= river reversing code ========================================================\n");
					for (DWORD i = 0; i < pEnv->codeGen.bkInstCount; ++i) {
						TRANSLATE_PRINT_INSTRUCTION(PRINT_BRANCHING_INFO, &pEnv->codeGen.bkRiverInst[i]);
					}
					TRANSLATE_PRINT(PRINT_BRANCHING_INFO, "===============================================================================\n");
				}
			}
			pCB->MarkForward();
			pEnv->lastFwBlock = pCB->address;
			pEnv->bForward = 1;

			pEnv->runtimeContext.jumpBuff = (DWORD)pCB->pFwCode;
		} else if (EXECUTION_TERMINATE == dwDirection) {
			//revtracerImports.dbgPrintFunc(PRINT_INFO | PRINT_INSPECTION, " +++ Tainted addresses +++ \n");
			pEnv->ac.PrintAddreses();
			//revtracerImports.dbgPrintFunc(PRINT_INFO | PRINT_INSPECTION, " +++++++++++++++++++++++++ \n");
			
			//((NtTerminateProcessFunc)revtracerImports.lowLevel.ntTerminateProcess)(0xFFFFFFFF, 0);

			pEnv->runtimeContext.jumpBuff = pEnv->exitAddr;
		}
		
	}

	void __stdcall SysHandler(struct ExecutionEnvironment *pEnv) {
		revtracerImports.dbgPrintFunc(PRINT_BRANCHING_INFO, "SysHandler!!!\n");
	}
};


DWORD DllEntry() {
	return 1;
}