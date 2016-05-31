#include <Windows.h>
#include <stdio.h>

#include "../revtracer/revtracer.h"
#include "../revtracer/SymbolicEnvironment.h"

#include "VariableTracker.h"
#include "TrackingCookie.h"
#include "Z3SymbolicExecutor.h"

#include "z3.h"

::HANDLE fDbg = ((::HANDLE)(::LONG_PTR)-1);

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

	const char executionStages[][6] = {
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
		printf("Pushing ast %08p %d\n", ast, executionPos - 1);
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
			printf("Popping ast %08p %d\n", ast, executionPos);
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
		printf("Alloc condition %08p\n", ret);
		return ret;
	}

	static void DummyFreeCondition(void *) {
		__asm int 3;
	}

	void FreeCondition(TrackedCondition *cond) {
		printf("Free condition %08p\n", cond);
		freeConditions[freeCondCount++] = cond;
	}

	void Push(TrackedCondition *tvd) {
		printf("Tracking condition %08p\n", tvd);
		tvds[revCount] = tvd;
		revCount++;
	}

	TrackedCondition *Pop() {
		TrackedCondition *ret = tvds[--revCount];
		tvds[revCount] = nullptr;

		printf("Restoring condition %08p\n", ret);
		return ret;
	}

	void ResetConds() {
		for (::DWORD i = 0; i < revCount; ++i) {
			FreeCondition(tvds[i]);
		}

		revCount = 0;
	}
};

rev::DWORD CustomExecutionBegin(void *ctx, rev::ADDR_TYPE addr, void *cbCtx) {
	static bool ctxInit = false;
	CustomExecutionContext *cExt = (CustomExecutionContext *)ctx;

	if (!ctxInit) {
		cExt->Init();
		ctxInit = true;
	}


	if (cExt->BACKTRACKING == cExt->executionState) {
		return EXECUTION_TERMINATE;
	}
	return EXECUTION_ADVANCE;
}

// here are the bindings to the payload
extern "C" unsigned int buffer[];
int Payload();
int Check(unsigned int *ptr);

unsigned int newPass[9];


bool DonePatching(void *ctx, unsigned int *orig, unsigned int *newVal, unsigned int size) {
	bool ret = true;
	for (unsigned int i = 0; i < size; ++i) {
		if (orig[i] != newVal[i]) {
			Z3_ast ast = (Z3_ast)rev::GetMemoryInfo(ctx, &orig[i]);
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

rev::DWORD CustomExecutionController(void *ctx, rev::ADDR_TYPE addr, void *cbCtx) {
	CustomExecutionContext *cExt = (CustomExecutionContext *)ctx;
	rev::DWORD ret = EXECUTION_ADVANCE;
	Z3_ast ast;

	static const char c[][32] = {
		"EXPLORING",
		"BACKTRACKING",
		"PATCHING",
		"REVERTING"
	};

	if (EXECUTION_BACKTRACK == lastDirection) {
		cExt->executionPos--;
		executor->StepBackward();

		switch (cExt->executionState) {
			case CustomExecutionContext::BACKTRACKING:
				ret = EXECUTION_BACKTRACK;

				if (cExt->TryPop(ast)) {
					CustomExecutionContext::TrackedCondition *cond = (CustomExecutionContext::TrackedCondition *)Z3_get_user_ptr(executor->context, ast);

					if (!cond->wasInverted) {
						ast = Z3_simplify(
							executor->context,
							Z3_mk_not(
								executor->context,
								ast
							)
						);

						printf("Invert condition %08p\n", cond);
						ast = Z3_simplify(executor->context, ast);

						if (Z3_L_UNDEF == Z3_get_bool_value(executor->context, ast)) {
							Z3_solver_assert(executor->context, executor->solver, ast);

							printf("(assert %s)\n\n", Z3_ast_to_string(executor->context, ast));

							unsigned int actualPass[10] = { 0 };
							Z3_lbool ret = Z3_solver_check(executor->context, executor->solver);

							if (Z3_L_TRUE == ret) {
								Z3_model model = Z3_solver_get_model(executor->context, executor->solver);

								unsigned int cnt = Z3_model_get_num_consts(executor->context, model);

								for (unsigned int i = 0; i < cnt; ++i) {
									Z3_func_decl c = Z3_model_get_const_decl(executor->context, model, i);

									//printf("%s\n", Z3_func_decl_to_string(executor->context, c));

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
									actualPass[r[2] - '0'] = ret;
								}

								/*printf("Check(\"");
								for (int i = 0; actualPass[i]; ++i) {
									printf("%c", actualPass[i]);
								}
								printf("\") = %04x\n", Check(actualPass));*/

								cExt->executionState = cExt->PATCHING;
							}
							else if (Z3_L_FALSE == ret) {
							}
							else {
							}

							cond->wasInverted = true;
							cExt->Push(cond);
						}
					}
				}

				break;
			case CustomExecutionContext::PATCHING:
				ret = EXECUTION_BACKTRACK;
				cExt->backSteps++;
				if (cExt->TryPop(ast)) {
					CustomExecutionContext::TrackedCondition *cond = (CustomExecutionContext::TrackedCondition *)Z3_get_user_ptr(executor->context, ast);
					cExt->Push(cond);
				}

				if (DonePatching(cbCtx, buffer, newPass, 8)) {
					cExt->executionState = cExt->RESTORING;
					ret = EXECUTION_ADVANCE;
				}
				break;

			case CustomExecutionContext::EXPLORING:
			case CustomExecutionContext::RESTORING:
				__asm int 3;
		}

		printf(
			"*** step %d; addr 0x%08p; state %s; backSteps %d, coundCount %d\n",
			cExt->executionPos,
			addr,
			c[cExt->executionState],
			cExt->backSteps,
			cExt->condCount
		);

		fflush(stdout);
	} else if (EXECUTION_ADVANCE == lastDirection) {
		printf(
			"*** step %d; addr 0x%08p; state %s; backSteps %d, coundCount %d\n",
			cExt->executionPos,
			addr,
			c[cExt->executionState],
			cExt->backSteps,
			cExt->condCount
		);

		fflush(stdout);

		switch (cExt->executionState) {
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
						CustomExecutionContext::TrackedCondition *cond = cExt->AllocCondition();
						Z3_set_user_ptr(executor->context, executor->lastCondition, cond, CustomExecutionContext::DummyFreeCondition);

						cExt->Push(executor->lastCondition);
						Z3_solver_assert(executor->context, executor->solver, executor->lastCondition);

						printf("(assert %s)\n\n", Z3_ast_to_string(executor->context, executor->lastCondition));

						executor->lastCondition = nullptr;
					}
				}
				break;

			case CustomExecutionContext::RESTORING:
				if (nullptr != executor->lastCondition) {

					if (Z3_L_UNDEF == Z3_get_bool_value(executor->context, executor->lastCondition)) {
						CustomExecutionContext::TrackedCondition *cond = cExt->Pop();
						Z3_set_user_ptr(executor->context, executor->lastCondition, cond, CustomExecutionContext::DummyFreeCondition);

						cExt->Push(executor->lastCondition);
						Z3_solver_assert(executor->context, executor->solver, executor->lastCondition);

						printf("(assert %s)\n\n", Z3_ast_to_string(executor->context, executor->lastCondition));

						executor->lastCondition = nullptr;
					}
				}

				cExt->backSteps--;
				if (0 > cExt->backSteps) {
					cExt->executionState = cExt->EXPLORING;
					cExt->ResetConds();
					cExt->backSteps = 0;
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
		cExt->executionPos++;
	}

	return ret;
}

rev::DWORD CustomExecutionEnd(void *ctx, void *cbCtx) {
	CustomExecutionContext *cExt = (CustomExecutionContext *)ctx;
	if (cExt->EXPLORING != cExt->executionState) {
		__asm int 3;
	}

	unsigned int actualPass[10] = { 0 };
	Z3_lbool ret = Z3_solver_check(executor->context, executor->solver);

	if (Z3_L_TRUE == ret) {
		Z3_model model = Z3_solver_get_model(executor->context, executor->solver);

		unsigned int cnt = Z3_model_get_num_consts(executor->context, model);

		for (unsigned int i = 0; i < cnt; ++i) {
			Z3_func_decl c = Z3_model_get_const_decl(executor->context, model, i);

			//printf("%s\n", Z3_func_decl_to_string(executor->context, c));

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
			actualPass[r[2] - '0'] = ret;
		}

		printf("Check(\"");
		for (int i = 0; actualPass[i]; ++i) {
			printf("%c", actualPass[i]);
		}
		printf("\") = %04x\n", Check(actualPass));
	}
	else if (Z3_L_FALSE == ret) {
		printf("unsat\n");
	}
	else { // undef
		printf("undef\n");
	}

	lastDirection = EXECUTION_BACKTRACK;
	cExt->executionState = cExt->BACKTRACKING;
	//cExt->executionPos--;
	//executor->StepBackward();
	return EXECUTION_BACKTRACK;
	//return EXECUTION_TERMINATE;
}

SymbolicExecutor *SymExeConstructor(SymbolicEnvironment *env) {
	executor = new Z3SymbolicExecutor(env, &trackingCookieFuncs);
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

void InitializeRevtracer(rev::ADDR_TYPE entryPoint) {
	HMODULE hNtDll = GetModuleHandle(L"ntdll.dll");
	rev::RevtracerAPI *api = &rev::revtracerAPI;

	api->dbgPrintFunc = DebugPrint;

	api->lowLevel.ntAllocateVirtualMemory = GetProcAddress(hNtDll, "NtAllocateVirtualMemory");
	api->lowLevel.ntFreeVirtualMemory = GetProcAddress(hNtDll, "NtFreeVirtualMemory");

	api->lowLevel.ntQueryInformationThread = GetProcAddress(hNtDll, "NtQueryInformationThread");
	api->lowLevel.ntTerminateProcess = GetProcAddress(hNtDll, "NtTerminateProcess");

	api->lowLevel.rtlNtStatusToDosError = GetProcAddress(hNtDll, "RtlNtStatusToDosError");
	api->lowLevel.vsnprintf_s = GetProcAddress(hNtDll, "_vsnprintf_s");

	api->executionBegin = CustomExecutionBegin;
	api->executionControl = CustomExecutionController;
	api->executionEnd = CustomExecutionEnd;

	api->trackCallback = TrackCallback;
	api->markCallback = MarkCallback;


	rev::RevtracerConfig *config = &rev::revtracerConfig;
	config->contextSize = sizeof(CustomExecutionContext);
	config->entryPoint = entryPoint;
	config->featureFlags = TRACER_FEATURE_SYMBOLIC | TRACER_FEATURE_REVERSIBLE;
	config->sCons = SymExeConstructor;

	rev::Initialize();
}



int main(int argc, char *argv[]) {
	
	HMODULE hNtDll = GetModuleHandleW(L"ntdll.dll");
	//TmpPrint("ntdll.dll @ 0x%08x\n", (DWORD)hNtDll);

	Payload();

	fDbg = CreateFileA("symb.log", GENERIC_WRITE, FILE_SHARE_READ, NULL, CREATE_ALWAYS, 0, NULL);

	InitializeRevtracer((rev::ADDR_TYPE)(&Payload));
	rev::MarkMemoryName((rev::ADDR_TYPE)(&buffer[0]), "a[0]");
	rev::MarkMemoryName((rev::ADDR_TYPE)(&buffer[1]), "a[1]");
	rev::MarkMemoryName((rev::ADDR_TYPE)(&buffer[2]), "a[2]");
	rev::MarkMemoryName((rev::ADDR_TYPE)(&buffer[3]), "a[3]");
	rev::MarkMemoryName((rev::ADDR_TYPE)(&buffer[4]), "a[4]");
	rev::MarkMemoryName((rev::ADDR_TYPE)(&buffer[5]), "a[5]");
	rev::MarkMemoryName((rev::ADDR_TYPE)(&buffer[6]), "a[6]");
	rev::MarkMemoryName((rev::ADDR_TYPE)(&buffer[7]), "a[7]");

	rev::Execute(argc, argv);

	Z3_close_log();

	CloseHandle(fDbg);
	return 0;
}
