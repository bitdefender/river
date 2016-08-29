#include <Windows.h>
#include <stdio.h>

#include "../Execution/Execution.h"
#include "SymbolicEnvironment.h"

#include "VariableTracker.h"
#include "TrackingCookie.h"

#include "RevSymbolicEnvironment.h"
#include "OverlappedRegisters.h"
#include "Z3SymbolicExecutor.h"

#include "z3.h"

ExecutionController *ctrl;

::HANDLE fDbg = ((::HANDLE)(::LONG_PTR)-1);

#define PRINTF(fmt, ...) \
	do { \
		printf(fmt, __VA_ARGS__); \
		fflush(stdout); \
	} while (false);

void DebugPrint(rev::DWORD printMask, const char *fmt, ...) {
	va_list va;
	char tmpBuff[512];

	static char lastChar = '\n';

	char pfxBuff[] = "[___|_____|___|_] ";

	const char messageTypes[][4] = {
		"___",
		"ERR",
		"INF",
		"DBG"
	};

	const char executionStages[][6] = 
	{
		"_____",
		"BRHND",
		"DIASM",
		"TRANS",
		"REASM",
		"RUNTM",
		"INSPT",
		"CNTNR"
	};

	const char codeTypes[][4] = {
		"___",
		"NAT",
		"RIV",
		"TRK",
		"SYM"
	};

	const char codeDirections[] = {
		'_', 'F', 'B'
	};

	_snprintf_s(
		pfxBuff,
		sizeof(pfxBuff)-1,
		"[%3s|%5s|%3s|%c] ",
		messageTypes[(printMask & PRINT_MESSAGE_MASK) >> PRINT_MESSAGE_SHIFT],
		executionStages[(printMask & PRINT_EXECUTION_MASK) >> PRINT_EXECUTION_SHIFT],
		codeTypes[(printMask & PRINT_CODE_TYPE_MASK) >> PRINT_CODE_TYPE_SHIFT],
		codeDirections[(printMask & PRINT_CODE_DIRECTION_MASK) >> PRINT_CODE_DIRECTION_SHIFT]
		);

	va_start(va, fmt);
	int sz = _vsnprintf_s(tmpBuff, sizeof(tmpBuff)-1, fmt, va);
	va_end(va);

	unsigned long wr;
	if ('\n' == lastChar) {
		WriteFile(fDbg, pfxBuff, sizeof(pfxBuff) - 1, &wr, NULL);
	}
	WriteFile(fDbg, tmpBuff, sz * sizeof(tmpBuff[0]), &wr, NULL);
	lastChar = tmpBuff[sz - 1];
}

Z3SymbolicExecutor *executor = nullptr;
OverlappedRegistersEnvironment *regEnv = nullptr;
RevSymbolicEnvironment *revEnv = nullptr;
::DWORD lastDirection = EXECUTION_ADVANCE;

struct CustomExecutionContext {
public :
	::DWORD executionPos;
	int backSteps;
	enum {
		EXPLORING,
		BACKTRACKING,
		PATCHING,
		RESTORING
	} executionState;

	struct {
		Z3_ast condition;
		::DWORD step;
	} condStack[64];
	::DWORD condCount;

	struct TrackedCondition {
		bool wasInverted;
	};

	TrackedCondition conditionPool[64];
	TrackedCondition *freeConditions[64];
	::DWORD freeCondCount;

	TrackedCondition *tvds[64];
	::DWORD revCount;

	void Init() {
		executionPos = 0;
		backSteps = 0;
		executionState = EXPLORING;
		condCount = 0;
		revCount = 0;

		freeCondCount = sizeof(conditionPool) / sizeof(conditionPool[0]);
		for (::DWORD i = 0; i < freeCondCount; ++i) {
			freeConditions[i] = &conditionPool[i];
		}
	}

	void Push(Z3_ast ast) {
		//PRINTF("Pushing ast %08p %d\n", ast, executionPos - 1);
		condStack[condCount].condition = ast;
		condStack[condCount].step = executionPos - 1;
		condCount++;
	}

	bool TryPop(Z3_ast &ast) {
		if (0 == condCount) {
			return false;
		}

		if (condStack[condCount - 1].step == executionPos) {
			condCount--;
			ast = condStack[condCount].condition;
			//PRINTF("Popping ast %08p %d\n", ast, executionPos);
			return true;
		}

		return false;
	}

	TrackedCondition *AllocCondition() {
		if (0 == freeCondCount) {
			return nullptr;
		}

		TrackedCondition *ret = freeConditions[--freeCondCount];
		ret->wasInverted = false;
		//PRINTF("Alloc condition %08p\n", ret);
		return ret;
	}

	static void DummyFreeCondition(void *) {
		__asm int 3;
	}

	void FreeCondition(TrackedCondition *cond) {
		//PRINTF("Free condition %08p\n", cond);
		freeConditions[freeCondCount++] = cond;
	}

	void Push(TrackedCondition *tvd) {
		//PRINTF("Tracking condition %08p\n", tvd);
		tvds[revCount] = tvd;
		revCount++;
	}

	TrackedCondition *Pop() {
		TrackedCondition *ret = tvds[--revCount];
		tvds[revCount] = nullptr;

		//PRINTF("Restoring condition %08p\n", ret);
		return ret;
	}

	void ResetConds() {
		for (::DWORD i = 0; i < revCount; ++i) {
			FreeCondition(tvds[i]);
		}

		revCount = 0;
	}
};

LARGE_INTEGER liFreq, liStart, liStop, liSymStart, liSymStop, liSymTotal, liBrStart, liBrStop, liBrTotal;

void BranchBegin() {
	QueryPerformanceCounter(&liBrStart);
}

void BranchEnd() {
	QueryPerformanceCounter(&liBrStop);
	liBrTotal.QuadPart += liBrStop.QuadPart - liBrStart.QuadPart;
}

// here are the bindings to the payload
extern "C" unsigned int buffer[];
int Payload();
int Check(unsigned int *ptr);

unsigned int newPass[9];

class SymbolicExecution : public ExecutionObserver {
private :
	CustomExecutionContext cec;
public:

	virtual unsigned int ExecutionBegin(void *ctx, rev::ADDR_TYPE addr) {
		static bool ctxInit = false;
		
		QueryPerformanceCounter(&liSymStart);
		if (!ctxInit) {
			revEnv = new RevSymbolicEnvironment(ctx);
			regEnv = new OverlappedRegistersEnvironment();
			regEnv->SetSubEnvironment(revEnv); //Init(env);
			executor = new Z3SymbolicExecutor(regEnv, &trackingCookieFuncs);


			cec.Init();
			ctrl->MarkMemoryValue(ctx, (rev::ADDR_TYPE)(&buffer[0]), (rev::DWORD)executor->CreateVariable("a[0]"));
			ctrl->MarkMemoryValue(ctx, (rev::ADDR_TYPE)(&buffer[1]), (rev::DWORD)executor->CreateVariable("a[1]"));
			ctrl->MarkMemoryValue(ctx, (rev::ADDR_TYPE)(&buffer[2]), (rev::DWORD)executor->CreateVariable("a[2]"));
			ctrl->MarkMemoryValue(ctx, (rev::ADDR_TYPE)(&buffer[3]), (rev::DWORD)executor->CreateVariable("a[3]"));
			ctrl->MarkMemoryValue(ctx, (rev::ADDR_TYPE)(&buffer[4]), (rev::DWORD)executor->CreateVariable("a[4]"));
			ctrl->MarkMemoryValue(ctx, (rev::ADDR_TYPE)(&buffer[5]), (rev::DWORD)executor->CreateVariable("a[5]"));
			ctrl->MarkMemoryValue(ctx, (rev::ADDR_TYPE)(&buffer[6]), (rev::DWORD)executor->CreateVariable("a[6]"));
			ctrl->MarkMemoryValue(ctx, (rev::ADDR_TYPE)(&buffer[7]), (rev::DWORD)executor->CreateVariable("a[7]"));

			ctxInit = true;
			QueryPerformanceCounter(&liStart);
			liSymTotal.QuadPart = 0;
		}

		rev::DWORD ret = EXECUTION_ADVANCE;

		if (cec.BACKTRACKING == cec.executionState) {
			QueryPerformanceCounter(&liStop);

			fprintf(stderr, "$$ Symbolic time: %lfms\n", 1000.0 * liSymTotal.QuadPart / liFreq.QuadPart);
			fprintf(stderr, "$$ Branching time: %lfms\n", 1000.0 * liBrTotal.QuadPart / liFreq.QuadPart);
			fprintf(stderr, "$$ Execution time: %lfms\n", 1000.0 * (liStop.QuadPart - liStart.QuadPart) / liFreq.QuadPart);
			ret = EXECUTION_TERMINATE;
		}

		QueryPerformanceCounter(&liSymStop);
		liSymTotal.QuadPart += liSymStop.QuadPart - liSymStart.QuadPart;
		return ret;
	}

	bool DonePatching(void *ctx, unsigned int *orig, unsigned int *newVal, unsigned int size) {
		bool ret = true;
		for (unsigned int i = 0; i < size; ++i) {
			if (orig[i] != newVal[i]) {
				Z3_ast ast = (Z3_ast)ctrl->GetMemoryInfo(ctx, &orig[i]);
				TrackedVariableData *tvd = (TrackedVariableData *)Z3_get_user_ptr(executor->context, ast);

				if (tvd->isLocked) {
					ret = false;
				}
				else {
					orig[i] = newVal[i];
				}
			}
		}

		return ret;
	}

	virtual unsigned int ExecutionControl(void *ctx, rev::ADDR_TYPE addr) {
		rev::DWORD ret = EXECUTION_ADVANCE;
		Z3_ast ast;

		QueryPerformanceCounter(&liSymStart);
		static const char c[][32] = {
			"EXPLORING",
			"BACKTRACKING",
			"PATCHING",
			"REVERTING"
		};

		if (EXECUTION_BACKTRACK == lastDirection) {
			cec.executionPos--;
			executor->StepBackward();

			switch (cec.executionState) {
			case CustomExecutionContext::BACKTRACKING:
				ret = EXECUTION_BACKTRACK;

				if (cec.TryPop(ast)) {
					CustomExecutionContext::TrackedCondition *cond = (CustomExecutionContext::TrackedCondition *)Z3_get_user_ptr(executor->context, ast);

					PRINTF("Trying to invert condition\n");

					if (!cond->wasInverted) {
						ast = Z3_simplify(
							executor->context,
							Z3_mk_not(
								executor->context,
								ast
							)
						);

						//PRINTF("Invert condition %08p\n", cond);
						ast = Z3_simplify(executor->context, ast);

						if (Z3_L_UNDEF == Z3_get_bool_value(executor->context, ast)) {
							Z3_solver_assert(executor->context, executor->solver, ast);

							PRINTF("(tryassert %s)\n\n", Z3_ast_to_string(executor->context, ast));

							//unsigned int actualPass[10] = { 0 };
							Z3_lbool ret = Z3_solver_check(executor->context, executor->solver);

							if (Z3_L_TRUE == ret) {
								PRINTF("==============================================================\n");
								Z3_model model = Z3_solver_get_model(executor->context, executor->solver);

								unsigned int cnt = Z3_model_get_num_consts(executor->context, model);

								for (unsigned int i = 0; i < 8; ++i) {
									newPass[i] = 0;
								}

								for (unsigned int i = 0; i < cnt; ++i) {
									Z3_func_decl c = Z3_model_get_const_decl(executor->context, model, i);

									//PRINTF("%s\n", Z3_func_decl_to_string(executor->context, c));

									Z3_ast ast = Z3_func_decl_to_ast(executor->context, c);
									Z3_symbol sName = Z3_get_decl_name(executor->context, c);
									Z3_string r = Z3_get_symbol_string(executor->context, sName);

									Z3_ast val;
									Z3_bool pEval = Z3_eval_func_decl(
										executor->context,
										model,
										c,
										&val
									);

									rev::DWORD ret;
									Z3_bool p = Z3_get_numeral_uint(executor->context, val, (unsigned int *)&ret);

									newPass[r[2] - '0'] = ret;
									//actualPass[r[2] - '0'] = ret;
								}

								PRINTF("NEW INPUT \"");
								for (int i = 0; newPass[i]; ++i) {
									PRINTF("%c", newPass[i]);
								}
								PRINTF("\" OUTPUT %04x\n", Check(newPass));

								fprintf(stderr, "NEW INPUT \"");
								for (int i = 0; newPass[i]; ++i) {
									fprintf(stderr, "%c", newPass[i]);
								}
								fprintf(stderr, "\" OUTPUT %04x\n", Check(newPass));

								cec.executionState = cec.PATCHING;
							}
							else if (Z3_L_FALSE == ret) {
							}
							else {
							}

							cond->wasInverted = true;
							cec.Push(cond);
						}
					}
				}

				break;
			case CustomExecutionContext::PATCHING:
				ret = EXECUTION_BACKTRACK;
				cec.backSteps++;
				if (cec.TryPop(ast)) {
					CustomExecutionContext::TrackedCondition *cond = (CustomExecutionContext::TrackedCondition *)Z3_get_user_ptr(executor->context, ast);
					cec.Push(cond);
				}

				if (DonePatching(ctx, buffer, newPass, 8)) {
					cec.executionState = cec.RESTORING;
					ret = EXECUTION_ADVANCE;
				}
				break;

			case CustomExecutionContext::EXPLORING:
			case CustomExecutionContext::RESTORING:
				__asm int 3;
			}

			PRINTF(
				"*** step %d; addr 0x%08p; state %s; backSteps %d, coundCount %d\n",
				cec.executionPos,
				addr,
				c[cec.executionState],
				cec.backSteps,
				cec.condCount
			);

			fflush(stdout);
		}
		else if (EXECUTION_ADVANCE == lastDirection) {
			PRINTF(
				"*** step %d; addr 0x%08p; state %s; backSteps %d, coundCount %d\n",
				cec.executionPos,
				addr,
				c[cec.executionState],
				cec.backSteps,
				cec.condCount
			);

			fflush(stdout);

			switch (cec.executionState) {
			case CustomExecutionContext::EXPLORING:
				if (nullptr != executor->lastCondition) {

					// revert last condition
					/*if (0x1769 == ((rev::DWORD)addr & 0xFFFF)) {
					executor->lastCondition = Z3_mk_not(
					executor->context,
					executor->lastCondition
					);
					}*/

					if (Z3_L_UNDEF == Z3_get_bool_value(executor->context, executor->lastCondition)) {
						CustomExecutionContext::TrackedCondition *cond = cec.AllocCondition();
						cond->wasInverted = false;
						Z3_set_user_ptr(executor->context, executor->lastCondition, cond, CustomExecutionContext::DummyFreeCondition);

						cec.Push(executor->lastCondition);
						Z3_solver_assert(executor->context, executor->solver, executor->lastCondition);

						PRINTF("(assert %s)\n\n", Z3_ast_to_string(executor->context, executor->lastCondition));

						executor->lastCondition = nullptr;
					}
				}
				break;

			case CustomExecutionContext::RESTORING:
				if (nullptr != executor->lastCondition) {

					if (Z3_L_UNDEF == Z3_get_bool_value(executor->context, executor->lastCondition)) {
						CustomExecutionContext::TrackedCondition *cond = cec.Pop();
						Z3_set_user_ptr(executor->context, executor->lastCondition, cond, CustomExecutionContext::DummyFreeCondition);

						cec.Push(executor->lastCondition);
						Z3_solver_assert(executor->context, executor->solver, executor->lastCondition);

						PRINTF("(assert %s)\n\n", Z3_ast_to_string(executor->context, executor->lastCondition));

						executor->lastCondition = nullptr;
					}
				}

				cec.backSteps--;
				if (0 > cec.backSteps) {
					cec.executionState = cec.EXPLORING;
					cec.ResetConds();
					cec.backSteps = 0;
				}
				break;
			case CustomExecutionContext::BACKTRACKING:
			case CustomExecutionContext::PATCHING:
				__asm int 3;
			}
		}



		lastDirection = ret;

		if (EXECUTION_ADVANCE == ret) {
			executor->StepForward();
			cec.executionPos++;
		}

		QueryPerformanceCounter(&liSymStop);
		liSymTotal.QuadPart += liSymStop.QuadPart - liSymStart.QuadPart;
		return ret;
	}

	virtual unsigned int ExecutionEnd(void *ctx) {
		QueryPerformanceCounter(&liSymStart);

		if (cec.EXPLORING != cec.executionState) {
			__asm int 3;
		}

		fprintf(stderr, "Check(\"");
		for (int i = 0; buffer[i]; ++i) {
			fprintf(stderr, "%c", buffer[i]);
		}
		fprintf(stderr, "\") = %04x\n", Check(buffer));

		PRINTF("Check(\"");
		for (int i = 0; buffer[i]; ++i) {
			PRINTF("%c", buffer[i]);
		}
		PRINTF("\") = %04x\n", Check(buffer));

		//unsigned int actualPass[10] = { 0 };
		/*Z3_lbool ret = Z3_solver_check(executor->context, executor->solver);

		if (Z3_L_TRUE == ret) {
			Z3_model model = Z3_solver_get_model(executor->context, executor->solver);
			fprintf(stderr, "%s\n", Z3_model_to_string(executor->context, model));

			unsigned int cnt = Z3_model_get_num_consts(executor->context, model);

			for (unsigned int i = 0; i < cnt; ++i) {
				Z3_func_decl c = Z3_model_get_const_decl(executor->context, model, i);

				//PRINTF("%s\n", Z3_func_decl_to_string(executor->context, c));

				Z3_ast ast = Z3_func_decl_to_ast(executor->context, c);
				Z3_symbol sName = Z3_get_decl_name(executor->context, c);
				Z3_string r = Z3_get_symbol_string(executor->context, sName);

				Z3_ast val;
				Z3_bool pEval = Z3_eval_func_decl(
					executor->context,
					model,
					c,
					&val
				);

				rev::DWORD ret;
				Z3_bool p = Z3_get_numeral_uint(executor->context, val, (unsigned int *)&ret);

				newPass[r[2] - '0'] = ret;
				//actualPass[r[2] - '0'] = ret;
			}
		}
		else if (Z3_L_FALSE == ret) {
			PRINTF("unsat\n");
		}
		else { // undef
			PRINTF("undef\n");
		}*/

		lastDirection = EXECUTION_BACKTRACK;
		cec.executionState = cec.BACKTRACKING;
		//cExt->executionPos--;
		//executor->StepBackward();


		QueryPerformanceCounter(&liSymStop);
		liSymTotal.QuadPart += liSymStop.QuadPart - liSymStart.QuadPart;

		return EXECUTION_BACKTRACK;
		//return EXECUTION_TERMINATE;
	}

	virtual void TerminationNotification(void *ctx) { }

} symbolicExecution;

sym::SymbolicExecutor *SymExeConstructor(sym::SymbolicEnvironment *env) {
	regEnv = new OverlappedRegistersEnvironment();
	regEnv->SetSubEnvironment(env); //Init(env);

	executor = new Z3SymbolicExecutor(regEnv, &trackingCookieFuncs);
	//variableTracker = new VariableTracker<Z3_ast>(executor, IsLocked, Lock, Unlock);
	return executor;
}

void TrackCallback(rev::DWORD value, rev::DWORD address, rev::DWORD segSel) {
	if (EXECUTION_ADVANCE == lastDirection) {
		Z3_ast t = (Z3_ast)value;

		if (nullptr != Z3_get_user_ptr(executor->context, t)) {
			//variableTracker->Lock(&t);
			executor->Lock(t);
		}
	}
}

void MarkCallback(rev::DWORD oldValue, rev::DWORD newValue, rev::DWORD address, rev::DWORD segSel) {
	//__asm int 3;
}

/*void InitializeRevtracer(rev::ADDR_TYPE entryPoint) {
	HMODULE hNtDll = GetModuleHandle(L"ntdll.dll");
	rev::RevtracerAPI *api = &rev::revtracerAPI;

	api->dbgPrintFunc = DebugPrint;

	api->lowLevel.ntAllocateVirtualMemory = GetProcAddress(hNtDll, "NtAllocateVirtualMemory");
	api->lowLevel.ntFreeVirtualMemory = GetProcAddress(hNtDll, "NtFreeVirtualMemory");

	api->lowLevel.ntQueryInformationThread = GetProcAddress(hNtDll, "NtQueryInformationThread");
	api->lowLevel.ntTerminateProcess = GetProcAddress(hNtDll, "NtTerminateProcess");

	api->lowLevel.rtlNtStatusToDosError = GetProcAddress(hNtDll, "RtlNtStatusToDosError");
	api->lowLevel.vsnprintf_s = GetProcAddress(hNtDll, "_vsnprintf_s");

	api->branchHandler = CustomBranchHandler;

	api->trackCallback = TrackCallback;
	api->markCallback = MarkCallback;

	rev::RevtracerConfig *config = &rev::revtracerConfig;
	config->contextSize = sizeof(CustomExecutionContext);
	config->entryPoint = entryPoint;
	config->featureFlags = TRACER_FEATURE_SYMBOLIC | TRACER_FEATURE_REVERSIBLE;
	config->sCons = SymExeConstructor;

	rev::Initialize();
}*/

void __stdcall SymbolicHandler(void *ctx, void *offset, void *addr) {
	RiverInstruction *instr = (RiverInstruction *)addr;
	
	regEnv->SetCurrentInstruction(instr, offset);
	executor->Execute(instr);
}

int main(int argc, char *argv[]) {
	
	HMODULE hNtDll = GetModuleHandleW(L"ntdll.dll");
	//TmpPrint("ntdll.dll @ 0x%08x\n", (DWORD)hNtDll);

	QueryPerformanceFrequency(&liFreq);

	QueryPerformanceCounter(&liStart);
	Payload();
	QueryPerformanceCounter(&liStop);

	fprintf(stderr, "$$ Native time: %lfms\n", 1000.0 * (liStop.QuadPart - liStart.QuadPart) / liFreq.QuadPart);

	ctrl = NewExecutionController(EXECUTION_INPROCESS);
	ctrl->SetEntryPoint(Payload);

	ctrl->SetExecutionFeatures(EXECUTION_FEATURE_REVERSIBLE | EXECUTION_FEATURE_SYMBOLIC);
	ctrl->SetExecutionObserver(&symbolicExecution);
	ctrl->SetSymbolicHandler(SymbolicHandler);
	ctrl->SetTrackingObserver(TrackCallback, MarkCallback);

	ctrl->Execute();
	ctrl->WaitForTermination();

	return 0;
}
