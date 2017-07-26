#include "../revtracer/common.h"
#include "../revtracer/revtracer.h"
#include "../revtracer/execenv.h"
#include "../revtracer/DebugPrintFlags.h"

#include "Execution.h"

#define PRINT_RUNTIME_TRACKING	PRINT_INFO | PRINT_RUNTIME | PRINT_TRACKING
#define PRINT_BRANCHING_ERROR	PRINT_ERROR | PRINT_BRANCH_HANDLER
#define PRINT_BRANCHING_DEBUG	PRINT_DEBUG | PRINT_BRANCH_HANDLER
#define PRINT_BRANCHING_INFO	PRINT_INFO | PRINT_BRANCH_HANDLER

static nodep::DWORD TopFromExecutionBuffer(ExecutionEnvironment *pEnv) {
	return *((nodep::DWORD *)pEnv->runtimeContext.execBuff);
}

static bool ExecutionBufferEmpty(ExecutionEnvironment *pEnv) {
	return (nodep::DWORD)pEnv->executionBase == pEnv->runtimeContext.execBuff;
}

typedef void(*SymbolicPayloadFunc)(nodep::DWORD trackBuffer);

nodep::DWORD BranchHandlerFunc(void *context, void *userContext, rev::ADDR_TYPE nextInstruction) {
	ExecutionEnvironment *pEnv = (ExecutionEnvironment *)context;
	ExecutionController *exec = (ExecutionController *)userContext;
	rev::ADDR_TYPE termCode = exec->GetTerminationCode();

	exec->DebugPrintf(PRINT_BRANCHING_INFO, "[BranchHandler] next instr %p\n", nextInstruction);

	if (pEnv->generationFlags & TRACER_FEATURE_TRACKING) {
		if (pEnv->bForward) {
			nodep::DWORD dwLastBlock = pEnv->lastFwBlock; //TopFromExecutionBuffer(pEnv);
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

	nodep::DWORD dwDirection = EXECUTION_ADVANCE;
	if ((nextInstruction == termCode) || (nextInstruction == (rev::ADDR_TYPE)pEnv->exitAddr)) {
		// TODO: verify parameters
		dwDirection = exec->ExecutionEnd(pEnv);
	}
	else if (ExecutionBufferEmpty(pEnv)) {
		dwDirection = exec->ExecutionBegin(nextInstruction, pEnv);
	}
	else {
		dwDirection = exec->ExecutionControl(nextInstruction, pEnv);
	}

	// for now: we only accept resets at execution end
	if ((EXECUTION_RESTART == dwDirection) && (nextInstruction != (rev::ADDR_TYPE)pEnv->exitAddr)) {
		dwDirection = EXECUTION_TERMINATE;
	}

	if ((EXECUTION_BACKTRACK == dwDirection) && (0 == (pEnv->generationFlags & TRACER_FEATURE_REVERSIBLE))) {
		dwDirection = EXECUTION_ADVANCE;
	}


	if (pEnv->generationFlags & TRACER_FEATURE_TRACKING) {
		if (EXECUTION_BACKTRACK == dwDirection) {
			// go backwards
			nodep::DWORD addr = TopFromExecutionBuffer(pEnv);
			RiverBasicBlock *pCB = pEnv->blockCache.FindBlock(addr);
			if (pCB) {
				exec->DebugPrintf(PRINT_BRANCHING_INFO, "Block found\n");
				((SymbolicPayloadFunc)pCB->pRevTrackCode)(pEnv->runtimeContext.trackBase - 4);
			}
			else {
				exec->DebugPrintf(PRINT_BRANCHING_ERROR, "No reverse block found!");
				DEBUG_BREAK;
			}
		}
	}

	exec->DebugPrintf(PRINT_BRANCHING_INFO, "[Parent] BH direction %s\n", dwDirection == EXECUTION_ADVANCE ? "advance" : "other");
	return dwDirection;
}

nodep::DWORD ErrorHandlerFunc(void *context, void *userContext, rev::RevtracerError *rerror) {
	ExecutionEnvironment *pEnv = (ExecutionEnvironment *)context;
	ExecutionController *exec = (ExecutionController *)userContext;

	if (rerror->errorCode == RERROR_UNK_INSTRUCTION) {
		exec->DebugPrintf(PRINT_ERROR, "Disassembling unknown instruction %02x %02x at address %08x\n",
				rerror->prefix, rerror->opcode, rerror->instructionAddress);

		switch(rerror->translatorId) {
			case RIVER_META_TRANSLATOR_ID:
				exec->DebugPrintf(PRINT_ERROR, "Translation step: MetaTrasnlator\n");
				break;
			case RIVER_DISASSEMBLER_ID:
				exec->DebugPrintf(PRINT_ERROR, "Translation step: Disassembler\n");
				break;
			default:
				exec->DebugPrintf(PRINT_ERROR, "Translation step: unknown\n");

		}
	}

	auto direction = exec->TranslationError((void*)rerror->instructionAddress, pEnv);

	return direction;
}

void SyscallControlFunc(void *context, void *userContext) {
	// *_this = (InprocessExecutionController *)context;
}
