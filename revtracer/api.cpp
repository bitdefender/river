#include "revtracer.h"

#include "common.h"
#include "execenv.h"
#include "river.h"

#include "AddressContainer.h"
#include "TrackedValues.h"
#include "TrackingItem.h"

#define PRINT_RUNTIME_TRACKING	PRINT_INFO | PRINT_RUNTIME | PRINT_TRACKING
#define PRINT_BRANCHING_ERROR	PRINT_ERROR | PRINT_BRANCH_HANDLER
#define PRINT_BRANCHING_DEBUG	PRINT_DEBUG | PRINT_BRANCH_HANDLER
#define PRINT_BRANCHING_INFO	PRINT_INFO | PRINT_BRANCH_HANDLER

#define DEBUG_BREAK { \
	*(WORD *)revtracerConfig.mainModule = 'ZM'; \
	__asm int 3; \
	*(WORD *)revtracerConfig.mainModule = '  '; \
}

namespace rev {
	TrackingTable<Temp, 128> trItem;

	DWORD __stdcall TrackAddr(struct ::ExecutionEnvironment *pEnv, DWORD dwAddr, DWORD segSel) {
		DWORD ret = pEnv->ac.Get(dwAddr + revtracerConfig.segmentOffsets[segSel & 0xFFFF]);

		TRACKING_PRINT(PRINT_RUNTIME_TRACKING, "TrackAddr 0x%08x => %d\n", dwAddr + revtracerConfig.segmentOffsets[segSel & 0xFFFF], ret);

		if (0 != ret) {
			revtracerAPI.trackCallback(ret, dwAddr, segSel);
		}

		return ret;
	}

	DWORD __stdcall MarkAddr(struct ::ExecutionEnvironment *pEnv, DWORD dwAddr, DWORD value, DWORD segSel) {
		TRACKING_PRINT(PRINT_RUNTIME_TRACKING, "MarkAddr 0x%08x <= %d\n", dwAddr + revtracerConfig.segmentOffsets[segSel & 0xFFFF], value);
		DWORD ret = pEnv->ac.Set(dwAddr + revtracerConfig.segmentOffsets[segSel & 0xFFFF], value);

		if (0 != ret) {
			revtracerAPI.markCallback(ret, value, dwAddr, segSel);
		}

		return ret;
	}
};

DWORD dwAddressTrackHandler = (DWORD)&rev::TrackAddr;
DWORD dwAddressMarkHandler = (DWORD)&rev::MarkAddr;

void RiverPrintInstruction(DWORD printMask, struct RiverInstruction *ri);

extern "C" {
	void PushToExecutionBuffer(struct ExecutionEnvironment *pEnv, DWORD value) {
		pEnv->runtimeContext.execBuff -= 4;
		*((DWORD *)pEnv->runtimeContext.execBuff) = value;
	}

	DWORD PopFromExecutionBuffer(struct ExecutionEnvironment *pEnv) {
		DWORD ret = *((DWORD *)pEnv->runtimeContext.execBuff);
		pEnv->runtimeContext.execBuff += 4;
		return ret;
	}

	bool ExecutionBufferEmpty(struct ExecutionEnvironment *pEnv) {
		return (DWORD)pEnv->executionBase == pEnv->runtimeContext.execBuff;
	}

	typedef void(*TrackFunc)(DWORD trackBuffer);

	typedef DWORD (*NtTerminateProcessFunc)(
		DWORD ProcessHandle,
		DWORD ExitStatus
	);

	void __stdcall BranchHandler(struct ExecutionEnvironment *pEnv, ADDR_TYPE a) {
		//ExecutionRegs *currentRegs = (ExecutionRegs *)((&a) + 1);
		pEnv->runtimeContext.registers = (UINT_PTR)((&a) + 1);
		RiverBasicBlock *pCB;
		pEnv->runtimeContext.trackBuff = pEnv->runtimeContext.trackBase;

		if (pEnv->generationFlags & TRACER_FEATURE_TRACKING) {
			if (pEnv->bForward) {
				DWORD dwLastBlock = pEnv->lastFwBlock; //TopFromExecutionBuffer(pEnv);
				RiverBasicBlock *pLast = pEnv->blockCache.FindBlock(dwLastBlock);
				if (NULL != pLast) {
					((TrackFunc)pLast->pTrackCode)(pEnv->runtimeContext.trackBase - 4);
				}
			}
		}


		if (pEnv->bForward) {
			PushToExecutionBuffer(pEnv, pEnv->lastFwBlock);
		}
		else {
			/*switch (ctx->callCount % 5) {
			case 3:
			//copy the registers to validate them later
			if (!RegCheck(pEnv, *currentRegs, regClone[1])) {
			__asm int 3;
			}

			if (execStack[1] != pEnv->runtimeContext.virtualStack) {
			//__asm int 3
			}
			break;
			case 4:
			//copy the registers to validate them later
			if (!RegCheck(pEnv, *currentRegs, regClone[0])) {
			__asm int 3;
			}

			if (execStack[0] != pEnv->runtimeContext.virtualStack) {
			//__asm int 3
			}
			break;
			}*/
		}

		DWORD dwDirection = EXECUTION_ADVANCE;  
		if ((a == revtracerAPI.lowLevel.ntTerminateProcess) || (a == (ADDR_TYPE)pEnv->exitAddr)) {
			// TODO: verify parameters
			dwDirection = revtracerAPI.executionEnd(pEnv->userContext, pEnv);
		} else if (ExecutionBufferEmpty(pEnv)) {
			dwDirection = revtracerAPI.executionBegin(pEnv->userContext, a, pEnv);
		} else {
			dwDirection = revtracerAPI.executionControl(pEnv->userContext, a, pEnv);
		}

		DWORD *stk = (DWORD *)pEnv->runtimeContext.virtualStack;
		revtracerAPI.dbgPrintFunc(PRINT_BRANCHING_DEBUG, "EIP: 0x%08x Stack :\n", a);
		revtracerAPI.dbgPrintFunc(PRINT_BRANCHING_DEBUG, "0x%08x: 0x%08x 0x%08x 0x%08x 0x%08x\n", stk + 0x00, stk[0], stk[1], stk[2], stk[3]);
		revtracerAPI.dbgPrintFunc(PRINT_BRANCHING_DEBUG, "EAX: 0x%08x  ECX: 0x%08x  EDX: 0x%08x  EBX: 0x%08x\n",
			((ExecutionRegs*)pEnv->runtimeContext.registers)->eax, 
			((ExecutionRegs*)pEnv->runtimeContext.registers)->ecx,
			((ExecutionRegs*)pEnv->runtimeContext.registers)->edx,
			((ExecutionRegs*)pEnv->runtimeContext.registers)->ebx);
		revtracerAPI.dbgPrintFunc(PRINT_BRANCHING_DEBUG, "ESP: 0x%08x  EBP: 0x%08x  ESI: 0x%08x  EDI: 0x%08x\n",
			((ExecutionRegs*)pEnv->runtimeContext.registers)->esp,
			((ExecutionRegs*)pEnv->runtimeContext.registers)->ebp,
			((ExecutionRegs*)pEnv->runtimeContext.registers)->esi,
			((ExecutionRegs*)pEnv->runtimeContext.registers)->edi);
		revtracerAPI.dbgPrintFunc(PRINT_BRANCHING_DEBUG, "Flags: 0x%08x\n",
			((ExecutionRegs*)pEnv->runtimeContext.registers)->eflags);

		if (TRACER_FEATURE_TRACKING & pEnv->generationFlags) {
			revtracerAPI.dbgPrintFunc(PRINT_RUNTIME_TRACKING, "Tainted registers :\n");
			revtracerAPI.dbgPrintFunc(PRINT_RUNTIME_TRACKING, "EAX: 0x%08x  ECX: 0x%08x  EDX: 0x%08x  EBX: 0x%08x\n", pEnv->runtimeContext.taintedRegisters[0], pEnv->runtimeContext.taintedRegisters[1], pEnv->runtimeContext.taintedRegisters[2], pEnv->runtimeContext.taintedRegisters[3]);
			revtracerAPI.dbgPrintFunc(PRINT_RUNTIME_TRACKING, "ESP: 0x%08x  EBP: 0x%08x  ESI: 0x%08x  EDI: 0x%08x\n", pEnv->runtimeContext.taintedRegisters[4], pEnv->runtimeContext.taintedRegisters[5], pEnv->runtimeContext.taintedRegisters[6], pEnv->runtimeContext.taintedRegisters[7]);
			revtracerAPI.dbgPrintFunc(PRINT_RUNTIME_TRACKING, "CF: 0x%08x  PF: 0x%08x  AF: 0x%08x  ZF: 0x%08x  SF: 0x%08x  OF: 0x%08x  DF: 0x%08x\n",
				pEnv->runtimeContext.taintedFlags[0],
				pEnv->runtimeContext.taintedFlags[1],
				pEnv->runtimeContext.taintedFlags[2],
				pEnv->runtimeContext.taintedFlags[3],
				pEnv->runtimeContext.taintedFlags[4],
				pEnv->runtimeContext.taintedFlags[5],
				pEnv->runtimeContext.taintedFlags[6]
			);
		}

		if ((EXECUTION_BACKTRACK == dwDirection) && (0 == (pEnv->generationFlags & TRACER_FEATURE_REVERSIBLE))) {
			dwDirection = EXECUTION_ADVANCE;
		}

		if (EXECUTION_BACKTRACK == dwDirection) {
			// go backwards
			DWORD addr = PopFromExecutionBuffer(pEnv);
			revtracerAPI.dbgPrintFunc(PRINT_BRANCHING_INFO, "Going Backwards to %08X!!!\n", addr);

			revtracerAPI.dbgPrintFunc(PRINT_BRANCHING_INFO, "Looking for block\n");
			pCB = pEnv->blockCache.FindBlock(addr);
			if (pCB) {
				revtracerAPI.dbgPrintFunc(PRINT_BRANCHING_INFO, "Block found\n");
				pCB->MarkBackward();
				//pEnv->posHist -= 1;
				
				if (pEnv->generationFlags & TRACER_FEATURE_TRACKING) {
					((TrackFunc)pCB->pRevTrackCode)(pEnv->runtimeContext.trackBase - 4);
				}
		
				pEnv->bForward = 0;
				pEnv->runtimeContext.jumpBuff = (DWORD)pCB->pBkCode;
			}
			else {
				revtracerAPI.dbgPrintFunc(PRINT_BRANCHING_ERROR, "No reverse block found!");
				__asm int 3;
			}

		} else if (EXECUTION_ADVANCE == dwDirection) {
			// go forwards
			revtracerAPI.dbgPrintFunc(PRINT_BRANCHING_INFO, "Going Forwards from %08X!!!\n", a);
			revtracerAPI.dbgPrintFunc(PRINT_BRANCHING_INFO, "Looking for block\n");
			pCB = pEnv->blockCache.FindBlock((rev::UINT_PTR)a);
			if (pCB) {
				revtracerAPI.dbgPrintFunc(PRINT_BRANCHING_INFO, "Block found\n");
			} else {
				revtracerAPI.dbgPrintFunc(PRINT_BRANCHING_INFO, "Not Found\n");
				pCB = pEnv->blockCache.NewBlock((rev::UINT_PTR)a);

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
			revtracerAPI.dbgPrintFunc(PRINT_INFO | PRINT_INSPECTION, " +++ Tainted addresses +++ \n");
			pEnv->ac.PrintAddreses();
			revtracerAPI.dbgPrintFunc(PRINT_INFO | PRINT_INSPECTION, " +++++++++++++++++++++++++ \n");
			((NtTerminateProcessFunc)revtracerAPI.lowLevel.ntTerminateProcess)(0xFFFFFFFF, 0);
		}

	}

	void __stdcall SysHandler(struct ExecutionEnvironment *pEnv) {
		revtracerAPI.dbgPrintFunc(PRINT_BRANCHING_INFO, "SysHandler!!!\n");
	}
};


DWORD DllEntry() {
	return 1;
}