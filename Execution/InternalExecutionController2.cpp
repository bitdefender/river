#include "../revtracer/common.h"
#include "../revtracer/revtracer.h"
#include "../revtracer/execenv.h"
#include "../revtracer/DebugPrintFlags.h"


#include "Execution.h"


#define PRINT_RUNTIME_TRACKING	PRINT_INFO | PRINT_RUNTIME | PRINT_TRACKING
#define PRINT_BRANCHING_ERROR	PRINT_ERROR | PRINT_BRANCH_HANDLER
#define PRINT_BRANCHING_DEBUG	PRINT_DEBUG | PRINT_BRANCH_HANDLER
#define PRINT_BRANCHING_INFO	PRINT_INFO | PRINT_BRANCH_HANDLER

DWORD TopFromExecutionBuffer(ExecutionEnvironment *pEnv) {
	return *((DWORD *)pEnv->runtimeContext.execBuff);
}

bool ExecutionBufferEmpty(ExecutionEnvironment *pEnv) {
	return (DWORD)pEnv->executionBase == pEnv->runtimeContext.execBuff;
}

DWORD BranchHandler(void *context, rev::ADDR_TYPE a, void *controller) {
	ExecutionEnvironment *pEnv = (ExecutionEnvironment *)context;
	ExecutionController *exec = (ExecutionController *)controller;

	if (pEnv->generationFlags & TRACER_FEATURE_TRACKING) {
		if (pEnv->bForward) {
			DWORD dwLastBlock = pEnv->lastFwBlock; //TopFromExecutionBuffer(pEnv);
			RiverBasicBlock *pLast = pEnv->blockCache.FindBlock(dwLastBlock);
			if (NULL != pLast) {
				((SymbolicPayloadFunc)pLast->pTrackCode)(pEnv->runtimeContext.trackBase - 4);
			}
		}
	}

	if (TRACER_FEATURE_TRACKING & pEnv->generationFlags) {
		LIB_TRACKING_PRINT(PRINT_RUNTIME_TRACKING, "Tainted registers :\n");
		LIB_TRACKING_PRINT(PRINT_RUNTIME_TRACKING, "EAX: 0x%08x  ECX: 0x%08x  EDX: 0x%08x  EBX: 0x%08x\n", pEnv->runtimeContext.taintedRegisters[0], pEnv->runtimeContext.taintedRegisters[1], pEnv->runtimeContext.taintedRegisters[2], pEnv->runtimeContext.taintedRegisters[3]);
		LIB_TRACKING_PRINT(PRINT_RUNTIME_TRACKING, "ESP: 0x%08x  EBP: 0x%08x  ESI: 0x%08x  EDI: 0x%08x\n", pEnv->runtimeContext.taintedRegisters[4], pEnv->runtimeContext.taintedRegisters[5], pEnv->runtimeContext.taintedRegisters[6], pEnv->runtimeContext.taintedRegisters[7]);
		LIB_TRACKING_PRINT(PRINT_RUNTIME_TRACKING, "CF: 0x%08x  PF: 0x%08x  AF: 0x%08x  ZF: 0x%08x  SF: 0x%08x  OF: 0x%08x  DF: 0x%08x\n",
			pEnv->runtimeContext.taintedFlags[0],
			pEnv->runtimeContext.taintedFlags[1],
			pEnv->runtimeContext.taintedFlags[2],
			pEnv->runtimeContext.taintedFlags[3],
			pEnv->runtimeContext.taintedFlags[4],
			pEnv->runtimeContext.taintedFlags[5],
			pEnv->runtimeContext.taintedFlags[6]
		);
	}

	DWORD dwDirection = EXECUTION_ADVANCE;
	if (/*(a == revtracerAPI.lowLevel.ntTerminateProcess) ||*/ (a == (ADDR_TYPE)pEnv->exitAddr)) {
		// TODO: verify parameters
		dwDirection = exec->ExecutionEnd(pEnv);
	} else if (ExecutionBufferEmpty(pEnv)) {
		dwDirection = exec->ExecutionBegin(a, pEnv);
	} else {
		dwDirection = exec->ExecutionControl(a, pEnv);
	}

	if ((EXECUTION_BACKTRACK == dwDirection) && (0 == (pEnv->generationFlags & TRACER_FEATURE_REVERSIBLE))) {
		dwDirection = EXECUTION_ADVANCE;
	}


	if (pEnv->generationFlags & TRACER_FEATURE_TRACKING) {
		if (EXECUTION_BACKTRACK == dwDirection) {
			// go backwards
			DWORD addr = TopFromExecutionBuffer(pEnv);
			RiverBasicBlock *pCB = pEnv->blockCache.FindBlock(addr);
			if (pCB) {
				exec->DebugPrintf(PRINT_BRANCHING_INFO, "Block found\n");
				((SymbolicPayloadFunc)pCB->pRevTrackCode)(pEnv->runtimeContext.trackBase - 4);
			}
			else {
				exec->DebugPrintf(PRINT_BRANCHING_ERROR, "No reverse block found!");
				__asm int 3;
			}
		}
	}

	return dwDirection;
}
