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

#define DEBUG_BREAK { \
	*(WORD *)revtracerConfig.mainModule = 'ZM'; \
	__asm int 3; \
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

	ADDR_TYPE TransferHandler(ExecutionEnvironment *pEnv, ADDR_TYPE a) {
		PushToExecutionBuffer(pEnv, pEnv->lastFwBlock);

		RiverBasicBlock *pCB = pEnv->blockCache.FindBlock((rev::UINT_PTR)a);

		pCB = pEnv->blockCache.FindBlock((rev::UINT_PTR)a);
		if (pCB) {
			revtracerAPI.dbgPrintFunc(PRINT_BRANCHING_INFO, "Block found\n");
		}
		else {
			revtracerAPI.dbgPrintFunc(PRINT_BRANCHING_INFO, "Not Found\n");
			pCB = pEnv->blockCache.NewBlock((rev::UINT_PTR)a);

			pEnv->codeGen.Translate(pCB, pEnv->generationFlags);
		}

		pCB->MarkForward();
		pEnv->lastFwBlock = pCB->address;
		pEnv->bForward = 1;

		return (ADDR_TYPE)pCB->pFwCode;
	}

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

		DWORD dwDirection = revtracerAPI.branchHandler(pEnv, pEnv->userContext, a);
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
			if (a == revtracerAPI.lowLevel.ntTerminateProcess) {
				((NtTerminateProcessFunc)revtracerAPI.lowLevel.ntTerminateProcess)(0xFFFFFFFF, 0);
			} else {
				pEnv->runtimeContext.jumpBuff = (rev::UINT_PTR)a;
			}
		}
		
	}

	void __stdcall SysHandler(struct ExecutionEnvironment *pEnv) {
		revtracerAPI.dbgPrintFunc(PRINT_BRANCHING_INFO, "SysHandler!!!\n");
		revtracerAPI.syscallControl(pEnv, pEnv->userContext);
	}

	void __stdcall ExceptionHandler(struct ExecutionEnvironment *pEnv, ADDR_TYPE a) {
		revtracerAPI.dbgPrintFunc(PRINT_BRANCHING_INFO, "ExceptionHandler!!!\n");
		pEnv->runtimeContext.execFlags |= RIVER_RUNTIME_EXCEPTION_FLAG;

		ADDR_TYPE exceptionIp = revtracerAPI.getExceptingIp(pEnv, pEnv->userContext, a);
		ADDR_TYPE originalIp = exceptionIp; //TODO: get original IP
		RiverBasicBlock *last = pEnv->blockCache.FindBlock(pEnv->lastFwBlock);
		DWORD off = (DWORD)exceptionIp - (DWORD)last->pFwCode;
		DWORD instrCount = *(DWORD *)last->pDisasmCode;
		RiverInstruction *disasm = (RiverInstruction *)&last->pDisasmCode[8];

		RiverInstruction *eInstr = nullptr;
		RiverInstruction *nInstr = nullptr;

		// TODO: replace with binary search
		for (int i = 0; i < instrCount; ++i) {
			if ((disasm[i].assembledOffset <= off) && (disasm[i].assembledOffset + disasm[i].assembledSize > off)) {
				// we have found it
				eInstr = &disasm[i];
				break;
			}
		}

		if (nullptr != eInstr) {
			revtracerAPI.dbgPrintFunc(PRINT_BRANCHING_INFO, "Found excepting instruction @%08x\n", eInstr->instructionAddress);
			originalIp = (ADDR_TYPE)eInstr->instructionAddress;


			for (int i = 0; i < instrCount; ++i) {
				if ((RIVER_FAMILY(disasm[i].family) == RIVER_FAMILY_NATIVE) && (disasm[i].instructionAddress == eInstr->instructionAddress)) {
					// we have found it
					nInstr = &disasm[i];
					break;
				}
			}

			if (nullptr != eInstr) {
				revtracerAPI.dbgPrintFunc(PRINT_BRANCHING_INFO, "Found native instruction ");
				RiverPrintInstruction(PRINT_BRANCHING_INFO, nInstr);
			}

			UINT_PTR *origStack = &pEnv->runtimeContext.exceptionStack;
			if (RIVER_FAMILY_NATIVE != RIVER_FAMILY(eInstr->family)) {
				switch (RIVER_FAMILY(eInstr->family)) {
				case RIVER_FAMILY_PRETRACK:
					origStack = &pEnv->runtimeContext.trackBuff;
					break;
				case RIVER_FAMILY_RIVER:
					origStack = &pEnv->runtimeContext.execBuff;
					break;
				}

				if (RIVER_FAMILY_FLAG_ORIG_xSP & eInstr->family) {
					__asm int 3;
				}
			}
			revtracerAPI.setExceptingIp(pEnv, pEnv->userContext, a, originalIp, (ADDR_TYPE *)origStack);


			// swap stacks back to their original states
			if (RIVER_FAMILY_NATIVE != RIVER_FAMILY(eInstr->family)) {
				UINT_PTR *swStack = nullptr;
				DWORD tmp;
				switch (RIVER_FAMILY(eInstr->family)) {
				case RIVER_FAMILY_PRETRACK:
					swStack = &pEnv->runtimeContext.trackBuff;
					break;
				case RIVER_FAMILY_RIVER:
					swStack = &pEnv->runtimeContext.execBuff;
					break;
				}

				if (RIVER_FAMILY_FLAG_ORIG_xSP & eInstr->family) {
					// swith the unused register (that contains the original esp value) with the stack, effectively restoring the register
					rev::BYTE reg = eInstr->GetUnusedRegister();

					DWORD *regs = (DWORD *)pEnv->runtimeContext.registers;

					tmp = regs[7 - reg];
					regs[7 - reg] = *swStack;
					*swStack = tmp;
				}

				// switch the exception stack with the initial stack, ensuring that exiting this function will restore the native stack
				tmp = (DWORD)pEnv->runtimeContext.exceptionStack;
				pEnv->runtimeContext.exceptionStack = *swStack;
				*swStack = tmp;
			}

			pEnv->runtimeContext.execFlags &= ~RIVER_RUNTIME_EXCEPTION_FLAG;

			if (revtracerConfig.featureFlags) {
				__asm int 3;

				// we do have to build a separate basic block (for handling reversibility and tracking
				for (RiverInstruction *i = disasm; i < eInstr; ++i) {

				}


				if (revtracerConfig.featureFlags & TRACER_FEATURE_REVERSIBLE) {
					// do some reversing magic here
					

				}

				if (revtracerConfig.featureFlags & TRACER_FEATURE_TRACKING) {
					// do some tracking magic here
				}
			}
		}
	}

};


DWORD DllEntry() {
	return 1;
}