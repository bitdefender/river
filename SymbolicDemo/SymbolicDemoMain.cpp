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

rev::DWORD CustomExecutionController(void *ctx, rev::ADDR_TYPE addr, void *cbCtx) {
	static rev::DWORD tmp = 0;
	rev::DWORD ret = EXECUTION_ADVANCE;

	if (2 == (tmp % 3)) {
		ret = EXECUTION_BACKTRACK;
	}
	tmp++;

	/*Z3_solver_push(executor->context, executor->solver); */

	if (nullptr != executor->lastCondition) {

		// revert last condition
		if (0x1769 == ((rev::DWORD)addr & 0xFFFF)) {
			executor->lastCondition = Z3_mk_not(
				executor->context,
				executor->lastCondition
			);
		}
		
		Z3_solver_assert(executor->context, executor->solver, executor->lastCondition);

		printf("(assert %s)\n\n", Z3_ast_to_string(executor->context, executor->lastCondition));

		executor->lastCondition = nullptr;
	}

	lastDirection = ret;
	switch (ret) {
		case EXECUTION_ADVANCE :
			executor->StepForward();
			break;
		case EXECUTION_BACKTRACK :
			printf("Backtrack!\n");
			executor->StepBackward();
			break;
	}
	return ret;
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

	api->executionControl = CustomExecutionController;

	api->trackCallback = TrackCallback;
	api->markCallback = MarkCallback;


	rev::RevtracerConfig *config = &rev::revtracerConfig;
	config->contextSize = 4;
	config->entryPoint = entryPoint;
	config->featureFlags = TRACER_FEATURE_SYMBOLIC | TRACER_FEATURE_REVERSIBLE;
	config->sCons = SymExeConstructor;

	rev::Initialize();
}



// here are the bindings to the payload
extern "C" unsigned int buffer[];
int Payload();
int Check(unsigned int *ptr);

int main(int argc, char *argv[]) {
	
	unsigned int actualPass[10] = { 0 };
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
	//rev::MarkMemoryName((rev::ADDR_TYPE)(&buffer[5]), "a[5]");
	//rev::MarkMemoryName((rev::ADDR_TYPE)(&buffer[6]), "a[6]");
	//rev::MarkMemoryName((rev::ADDR_TYPE)(&buffer[7]), "a[7]");

	rev::Execute(argc, argv);

	Z3_lbool ret = Z3_solver_check(executor->context, executor->solver);

	if (Z3_L_TRUE == ret) {
		Z3_model model = Z3_solver_get_model(executor->context, executor->solver);
		
		//printf("%s\n", Z3_model_to_string(executor->context, model));

		unsigned int cnt = Z3_model_get_num_consts(executor->context, model);

		for (unsigned int i = 0; i < cnt; ++i) {
			Z3_func_decl c = Z3_model_get_const_decl(executor->context, model, i);

			printf("%s\n", Z3_func_decl_to_string(executor->context, c));

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

	Z3_close_log();

	CloseHandle(fDbg);
	return 0;
}
