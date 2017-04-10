#include "revtracer.h"

#include "common.h"
#include "execenv.h"
#include "river.h"

#include "AddressContainer.h"
#include "Tracking.h"

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
void DirectionHandler(DWORD dwDirection, ExecutionEnvironment *pEnv, ADDR_TYPE addr);

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

	void ClearExecutionBuffer(ExecutionEnvironment *pEnv) {
		pEnv->runtimeContext.execBuff = pEnv->executionBase;
	}

	typedef DWORD (*NtTerminateProcessFunc)(
		DWORD ProcessHandle,
		DWORD ExitStatus
	);


	void __stdcall BranchHandler(ExecutionEnvironment *pEnv, ADDR_TYPE a) {
		//ExecutionRegs *currentRegs = (ExecutionRegs *)((&a) + 1);

		pEnv->runtimeContext.registers = (UINT_PTR)((&a) + 1);
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

		DWORD dwDirection = revtracerAPI.branchHandler(pEnv, pEnv->userContext, a);
		DirectionHandler(dwDirection, pEnv, a);
	}

	void __stdcall SysHandler(struct ExecutionEnvironment *pEnv) {
		revtracerAPI.dbgPrintFunc(PRINT_BRANCHING_INFO, "SysHandler!!!\n");
	}
};

template<DWORD Direction>
bool ProcessDirection(ExecutionEnvironment *pEnv, ADDR_TYPE nextInstruction);

template<>
bool ProcessDirection<EXECUTION_TERMINATE>(ExecutionEnvironment *pEnv, ADDR_TYPE nextInstruction) {
	//revtracerAPI.dbgPrintFunc(PRINT_INFO | PRINT_INSPECTION, " +++ Tainted addresses +++ \n");
	pEnv->ac.PrintAddreses();
	//revtracerAPI.dbgPrintFunc(PRINT_INFO | PRINT_INSPECTION, " +++++++++++++++++++++++++ \n");

	//((NtTerminateProcessFunc)revtracerAPI.lowLevel.ntTerminateProcess)(0xFFFFFFFF, 0);

	pEnv->runtimeContext.jumpBuff = pEnv->exitAddr;
	return true;
}

template<>
bool ProcessDirection<EXECUTION_RESTART>(ExecutionEnvironment *pEnv, ADDR_TYPE nextInstruction) {
	BRANCHING_PRINT(PRINT_BRANCHING_INFO, "RESTART Requested\n", revtracerConfig.entryPoint);
	pEnv->lastFwBlock = 0;
	ClearExecutionBuffer(pEnv);

	// need to push the return address again
	DWORD nextDirection = revtracerAPI.branchHandler(pEnv, pEnv->userContext, revtracerConfig.entryPoint);
	if (nextDirection == EXECUTION_ADVANCE) {
		pEnv->runtimeContext.virtualStack -= 4;
		*((ADDR_TYPE *)pEnv->runtimeContext.virtualStack) = nextInstruction;
		pEnv->lastFwBlock = (UINT_PTR)revtracerConfig.entryPoint;
		pEnv->bForward = 1;
		DirectionHandler(nextDirection, pEnv, revtracerConfig.entryPoint);
	} else {
		pEnv->runtimeContext.jumpBuff = (UINT_PTR)revtracerAPI.lowLevel.ntTerminateProcess;
	}
	return true;
}

template<>
bool ProcessDirection<EXECUTION_ADVANCE>(ExecutionEnvironment *pEnv, ADDR_TYPE nextInstruction) {
	RiverBasicBlock *pCB;
	// go forwards
	TRANSLATE_PRINT(PRINT_BRANCHING_INFO, "Going Forwards from %08X!!!\n", nextInstruction);
	TRANSLATE_PRINT(PRINT_BRANCHING_INFO, "Looking for block\n");

	pCB = pEnv->blockCache.FindBlock((rev::UINT_PTR)nextInstruction);

	if (pCB) {
		TRANSLATE_PRINT(PRINT_BRANCHING_INFO, "Block found\n");
	} else {
		TRANSLATE_PRINT(PRINT_BRANCHING_INFO, "Not Found\n");
		pCB = pEnv->blockCache.NewBlock((rev::UINT_PTR)nextInstruction);

		RevtracerError rerror;
		bool ret = pEnv->codeGen.Translate(pCB, pEnv->generationFlags, &rerror);

		if (!ret && rerror.errorCode != RERROR_OK) {
			auto nextDirection = revtracerAPI.errorHandler(pEnv, pEnv->userContext, &rerror);
			if (nextDirection == EXECUTION_RESTART) {
				ProcessDirection<EXECUTION_RESTART>(pEnv, nextInstruction);
			} else {
				ProcessDirection<EXECUTION_TERMINATE>(pEnv, nextInstruction);
			}
			return false;
		}

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
	return true;
}

template<>
bool ProcessDirection<EXECUTION_BACKTRACK>(ExecutionEnvironment *pEnv, ADDR_TYPE nextInstruction) {
	RiverBasicBlock *pCB;

	// go backwards
	DWORD addr = PopFromExecutionBuffer(pEnv);
	revtracerAPI.dbgPrintFunc(PRINT_BRANCHING_INFO, "Going Backwards to %08X!!!\n", addr);

	revtracerAPI.dbgPrintFunc(PRINT_BRANCHING_INFO, "Looking for block\n");
	pCB = pEnv->blockCache.FindBlock(addr);
	if (pCB) {
		revtracerAPI.dbgPrintFunc(PRINT_BRANCHING_INFO, "Block found\n");
		pCB->MarkBackward();
		//pEnv->posHist -= 1;

		pEnv->bForward = 0;
		pEnv->runtimeContext.jumpBuff = (DWORD)pCB->pBkCode;
	}
	else {
		revtracerAPI.dbgPrintFunc(PRINT_BRANCHING_ERROR, "No reverse block found!");
		return false;
	}
	return true;
}

void DirectionHandler(DWORD dwDirection, ExecutionEnvironment *pEnv, ADDR_TYPE addr) {
	if (EXECUTION_BACKTRACK == dwDirection) {
		ProcessDirection<EXECUTION_BACKTRACK>(pEnv, addr);
	} else if (EXECUTION_ADVANCE == dwDirection) {
		ProcessDirection<EXECUTION_ADVANCE>(pEnv, addr);
	} else if (EXECUTION_TERMINATE == dwDirection) {
		ProcessDirection<EXECUTION_TERMINATE>(pEnv, addr);
	} else if (EXECUTION_RESTART == dwDirection) {
		ProcessDirection<EXECUTION_RESTART>(pEnv, addr);
	}
}


DWORD DllEntry() {
	return 1;
}
