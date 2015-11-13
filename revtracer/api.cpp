#include "revtracer.h"

#include "execenv.h"
#include "river.h"

#include "AddressContainer.h"

namespace rev {
	AddressContainer ac;

	DWORD __stdcall TrackAddr(struct ::_exec_env *pEnv, DWORD dwAddr, DWORD segSel) {
		DWORD ret = ac.Get(dwAddr + revtracerConfig.segmentOffsets[segSel & 0xFFFF]);
		revtracerAPI.dbgPrintFunc("TrackAddr 0x%08x => %d\n", dwAddr + revtracerConfig.segmentOffsets[segSel & 0xFFFF], ret);
		return ret;
	}

	DWORD __stdcall MarkAddr(struct ::_exec_env *pEnv, DWORD dwAddr, DWORD value, DWORD segSel) {
		revtracerAPI.dbgPrintFunc("MarkAddr 0x%08x <= %d\n", dwAddr + revtracerConfig.segmentOffsets[segSel & 0xFFFF], value);
		return ac.Set(dwAddr + revtracerConfig.segmentOffsets[segSel & 0xFFFF], value);
	}
};

DWORD dwAddressTrackHandler = (DWORD)&rev::TrackAddr;
DWORD dwAddressMarkHandler = (DWORD)&rev::MarkAddr;

void RiverPrintInstruction(struct RiverInstruction *ri);

extern "C" {
	void PushToExecutionBuffer(struct _exec_env *pEnv, DWORD value) {
		pEnv->runtimeContext.execBuff -= 4;
		*((DWORD *)pEnv->runtimeContext.execBuff) = value;
	}

	DWORD PopFromExecutionBuffer(struct _exec_env *pEnv) {
		DWORD ret = *((DWORD *)pEnv->runtimeContext.execBuff);
		pEnv->runtimeContext.execBuff += 4;
		return ret;
	}

	typedef void(*TrackFunc)(DWORD trackBuffer);

	typedef DWORD (*NtTerminateProcessFunc)(
		DWORD ProcessHandle,
		DWORD ExitStatus
	);

	void __stdcall BranchHandler(struct _exec_env *pEnv, ADDR_TYPE a) {
		//ExecutionRegs *currentRegs = (ExecutionRegs *)((&a) + 1);
		pEnv->runtimeContext.registers = (UINT_PTR)((&a) + 1);
		RiverBasicBlock *pCB;
		pEnv->runtimeContext.trackBuff = pEnv->runtimeContext.trackBase;

		/* Do not execute tracking code for now! */
		/*DWORD dwLastBlock = pEnv->lastFwBlock; //TopFromExecutionBuffer(pEnv);
		RiverBasicBlock *pLast = pEnv->blockCache.FindBlock(dwLastBlock);
		if (NULL != pLast) {
			((TrackFunc)pLast->pTrackCode)(pEnv->runtimeContext.trackBase - 4);
		}*/


		if (a == revtracerAPI.lowLevel.ntTerminateProcess) {
			//PrintStats(pEnv);

			// TODO: verify parameters

			revtracerAPI.dbgPrintFunc(" +++ Tainted addresses +++ \n");
			ac.PrintAddreses();
			revtracerAPI.dbgPrintFunc(" +++++++++++++++++++++++++ \n");
			((NtTerminateProcessFunc)revtracerAPI.lowLevel.ntTerminateProcess)(0xFFFFFFFF, 0);
		}

		if (pEnv->bForward) {
			PushToExecutionBuffer(pEnv, pEnv->lastFwBlock);
		} else {
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

		DWORD *stk = (DWORD *)pEnv->runtimeContext.virtualStack;
		revtracerAPI.dbgPrintFunc("EIP: 0x%08x Stack :\n", a);
		revtracerAPI.dbgPrintFunc("0x%08x: 0x%08x 0x%08x 0x%08x 0x%08x\n", stk + 0x00, stk[0], stk[1], stk[2], stk[3]);
		revtracerAPI.dbgPrintFunc("EAX: 0x%08x  ECX: 0x%08x  EDX: 0x%08x  EBX: 0x%08x\n", 
			((ExecutionRegs*)pEnv->runtimeContext.registers)->eax, 
			((ExecutionRegs*)pEnv->runtimeContext.registers)->ecx,
			((ExecutionRegs*)pEnv->runtimeContext.registers)->edx,
			((ExecutionRegs*)pEnv->runtimeContext.registers)->ebx);
		revtracerAPI.dbgPrintFunc("ESP: 0x%08x  EBP: 0x%08x  ESI: 0x%08x  EDI: 0x%08x\n", 
			((ExecutionRegs*)pEnv->runtimeContext.registers)->esp,
			((ExecutionRegs*)pEnv->runtimeContext.registers)->ebp,
			((ExecutionRegs*)pEnv->runtimeContext.registers)->esi,
			((ExecutionRegs*)pEnv->runtimeContext.registers)->edi);
		revtracerAPI.dbgPrintFunc("Flags: 0x%08x\n", 
			((ExecutionRegs*)pEnv->runtimeContext.registers)->eflags);

		/*revtracerAPI.dbgPrintFunc("Tainted registers :\n");
		revtracerAPI.dbgPrintFunc("EAX: 0x%08x  ECX: 0x%08x  EDX: 0x%08x  EBX: 0x%08x\n", pEnv->runtimeContext.taintedRegisters[0], pEnv->runtimeContext.taintedRegisters[1], pEnv->runtimeContext.taintedRegisters[2], pEnv->runtimeContext.taintedRegisters[3]);
		revtracerAPI.dbgPrintFunc("ESP: 0x%08x  EBP: 0x%08x  ESI: 0x%08x  EDI: 0x%08x\n", pEnv->runtimeContext.taintedRegisters[4], pEnv->runtimeContext.taintedRegisters[5], pEnv->runtimeContext.taintedRegisters[6], pEnv->runtimeContext.taintedRegisters[7]);
		revtracerAPI.dbgPrintFunc("CF: 0x%08x  PF: 0x%08x  AF: 0x%08x  ZF: 0x%08x  SF: 0x%08x  OF: 0x%08x  DF: 0x%08x\n",
			pEnv->runtimeContext.taintedFlags[0],
			pEnv->runtimeContext.taintedFlags[1],
			pEnv->runtimeContext.taintedFlags[2],
			pEnv->runtimeContext.taintedFlags[3],
			pEnv->runtimeContext.taintedFlags[4],
			pEnv->runtimeContext.taintedFlags[5],
			pEnv->runtimeContext.taintedFlags[6]
		);*/

		DWORD dwDirection = revtracerAPI.executionControl(pEnv->userContext, a, pEnv);

		if (EXECUTION_BACKTRACK == dwDirection) {
			// go backwards
			DWORD addr = PopFromExecutionBuffer(pEnv);
			revtracerAPI.dbgPrintFunc("Going Backwards to %08X!!!\n", addr);

			revtracerAPI.dbgPrintFunc("Looking for block\n");
			pCB = pEnv->blockCache.FindBlock(addr);
			if (pCB) {
				revtracerAPI.dbgPrintFunc("Block found\n");
				pCB->MarkBackward();
				//pEnv->posHist -= 1;
				pEnv->bForward = 0;
				pEnv->runtimeContext.jumpBuff = (DWORD)pCB->pBkCode;
			}
			else {
				revtracerAPI.dbgPrintFunc("No reverse block found!");
				__asm int 3;
			}

		} else if (EXECUTION_ADVANCE == dwDirection) {
			// go forwards
			revtracerAPI.dbgPrintFunc("Going Forwards from %08X!!!\n", a);
			//__try {
				revtracerAPI.dbgPrintFunc("Looking for block\n");
				pCB = pEnv->blockCache.FindBlock((rev::UINT_PTR)a);
				if (pCB) {
					revtracerAPI.dbgPrintFunc("Block found\n");
				}
				else {
					revtracerAPI.dbgPrintFunc("Not Found\n");
					pCB = pEnv->blockCache.NewBlock((rev::UINT_PTR)a);

					pEnv->codeGen.Translate(pCB, 0);

					revtracerAPI.dbgPrintFunc("= river saving code ===========================================================\n");
					for (DWORD i = 0; i < pEnv->codeGen.fwInstCount; ++i) {
						RiverPrintInstruction(&pEnv->codeGen.fwRiverInst[i]);
					}
					revtracerAPI.dbgPrintFunc("===============================================================================\n");

					revtracerAPI.dbgPrintFunc("= river reversing code ========================================================\n");
					for (DWORD i = 0; i < pEnv->codeGen.bkInstCount; ++i) {
						RiverPrintInstruction(&pEnv->codeGen.bkRiverInst[i]);
					}
					revtracerAPI.dbgPrintFunc("===============================================================================\n");
				}
				pCB->MarkForward();
				//pEnv->jumpBuff = (DWORD)pCB->pCode;
				//TouchBlock(pEnv, pCB);
				pEnv->lastFwBlock = pCB->address;
				pEnv->bForward = 1;

				/*switch (ctx->callCount % 5) {
				case 1:
					//copy the registers to validate them later
					memcpy(&regClone[0], currentRegs, sizeof(regClone[0]));
					execStack[0] = pEnv->runtimeContext.virtualStack;
					break;
				case 2:
					//copy the registers to validate them later
					memcpy(&regClone[1], currentRegs, sizeof(regClone[1]));
					execStack[1] = pEnv->runtimeContext.virtualStack;
					break;
				}*/


				pEnv->runtimeContext.jumpBuff = (DWORD)pCB->pFwCode;
			//}
			//__except (1) { //EXCEPTION_EXECUTE_HANDLER
			//	pEnv->runtimeContext.jumpBuff = (rev::UINT_PTR)a;
			//}
		}

	}

	void __stdcall SysHandler(struct _exec_env *pEnv) {
		//__asm int 3;

		revtracerAPI.dbgPrintFunc("SysHandler!!!\n");
	}
};


DWORD DllEntry() {
	return 1;
}