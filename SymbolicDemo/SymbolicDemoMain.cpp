#include <Windows.h>
#include <stdio.h>

#include "../revtracer/revtracer.h"
#include "../revtracer/SymbolicEnvironment.h"

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

rev::DWORD CustomExecutionController(void *ctx, rev::ADDR_TYPE addr, void *cbCtx) {
	rev::DWORD ret = EXECUTION_ADVANCE;

	return ret;
}

void *CreateVariable(void *ctx, const char *name);
void Execute(void *ctx, RiverInstruction *instruction);

SymbolicExecutorFuncs sFuncs = {
	CreateVariable,
	Execute,
	nullptr,
	nullptr
};

class MySymbolicExecutor : public SymbolicExecutor {
public:
	int symIndex;

	MySymbolicExecutor(SymbolicEnvironment *e) : SymbolicExecutor(e, &sFuncs) {
		symIndex = 2;
	}
};

void *CreateVariable(void *ctx, const char *name) {
	__asm int 3;
	return NULL;
}

void Execute(void *ctx, RiverInstruction *instruction) {
	MySymbolicExecutor *_this = (MySymbolicExecutor *)ctx;
	rev::BOOL isTrackedOp[4], isTrackedFlg[7];
	rev::DWORD concreteValuesOp[4];
	rev::BYTE concreteValuesFlg[7];
	void *symbolicValuesOp[4], *symbolicValuesFlg[7];

	for (rev::DWORD i = 0; i < 7; i++) {
		if ((1 << i) & instruction->testFlags) {
			_this->env->GetFlgValue((1 << i), isTrackedFlg[i], concreteValuesFlg[i], symbolicValuesFlg[i]);
		}
	}


	for (int i = 0; i < 4; ++i) {
		_this->env->GetOperand(i, isTrackedOp[i], concreteValuesOp[i], symbolicValuesOp[i]);
	}

	// do some actual work

	for (int i = 0; i < 4; ++i) {
		if (RIVER_SPEC_MODIFIES_OP(i) & instruction->specifiers) {
			void *tmp = (void *)_this->symIndex;
			_this->env->SetOperand(i, tmp);
			_this->symIndex++;
		}
	}

	for (rev::DWORD i = 0; i < 7; i++) {
		if ((1 << i) & instruction->modFlags) {
			void *tmp = (void *)_this->symIndex;
			_this->env->SetFlgValue(i, tmp);
			_this->symIndex++;
		}
	}
}

SymbolicExecutor *SymExeConstructor(SymbolicEnvironment *env) {
	SymbolicExecutor *ret = new MySymbolicExecutor(env);
	return ret;
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


	rev::RevtracerConfig *config = &rev::revtracerConfig;
	config->contextSize = 4;
	config->entryPoint = entryPoint;
	config->featureFlags = TRACER_FEATURE_SYMBOLIC | TRACER_FEATURE_REVERSIBLE;
	config->sCons = SymExeConstructor;

	rev::Initialize();
}



// here are the bindings to the payload
extern "C" int fibo0, fibo1;
int Payload();

int main(int argc, char *argv[]) {
	

	HMODULE hNtDll = GetModuleHandleW(L"ntdll.dll");
	//TmpPrint("ntdll.dll @ 0x%08x\n", (DWORD)hNtDll);

	fDbg = CreateFileA("symb.log", GENERIC_WRITE, FILE_SHARE_READ, NULL, CREATE_ALWAYS, 0, NULL);

	InitializeRevtracer((rev::ADDR_TYPE)(&Payload));
	rev::MarkMemoryName((rev::ADDR_TYPE)(&fibo0), "a");
	rev::MarkMemoryName((rev::ADDR_TYPE)(&fibo1), "b");

	rev::Execute(argc, argv);

	CloseHandle(fDbg);
	return 0;
}
