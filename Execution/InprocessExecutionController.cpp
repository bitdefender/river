#include "InprocessExecutionController.h"
#include "RiverStructs.h"

bool InprocessExecutionController::SetPath() {
	return false;
}

bool InprocessExecutionController::SetCmdLine() {
	return false;
}

void *InprocessExecutionController::GetProcessHandle() {
	return GetCurrentProcess();
}

bool InprocessExecutionController::GetProcessVirtualMemory(VirtualMemorySection *&sections, int &sectionCount) {
	return false;
}

bool InprocessExecutionController::GetModules(ModuleInfo *& modules, int & moduleCount) {
	return false;
}

bool InprocessExecutionController::ReadProcessMemory(unsigned int base, unsigned int size, unsigned char * buff) {
	memcpy(buff, (LPCVOID)base, size);
	return true;
}

DWORD __stdcall ThreadProc(LPVOID p) {
	InprocessExecutionController *_this = (InprocessExecutionController *)p;
	return _this->ControlThread();
}

//unsigned int BranchHandlerFunc(void *context, void *userContext, void *nextInstruction);
void SyscallControlFunc(void *context, void *userContext);

rev::ADDR_TYPE GetExceptingIp(void *context, void *userContext, rev::ADDR_TYPE hookAddress);
void SetExceptingIp(void *context, void *userContext, rev::ADDR_TYPE hookAddress, rev::ADDR_TYPE newIp, rev::ADDR_TYPE *newStack);
void ApplyHook(void *context, void *userContext, rev::ADDR_TYPE originalAddr, rev::ADDR_TYPE hookedAddr);


bool InprocessExecutionController::Execute() {
	HMODULE hNtDll = GetModuleHandleW(L"ntdll.dll");
	HMODULE hRevTracer = LoadLibraryW(L"revtracer.dll");

	rev::RevtracerAPI *api = (rev::RevtracerAPI *)GetProcAddress(hRevTracer, "revtracerAPI");
	revCfg = (rev::RevtracerConfig *)GetProcAddress(hRevTracer, "revtracerConfig");

	api->dbgPrintFunc = ::DebugPrintf;

	api->branchHandler = BranchHandlerFunc;
	api->syscallControl = SyscallControlFunc;
	api->setExceptingIp = ::SetExceptingIp;
	api->getExceptingIp = ::GetExceptingIp;
	api->applyHook = ::ApplyHook;


	if (nullptr != trackCb) {
		api->trackCallback = trackCb;
	}

	if (nullptr != markCb) {
		api->markCallback = markCb;
	}

	if (nullptr != symbCb) {
		api->symbolicHandler = symbCb;
	}

	api->lowLevel.ntAllocateVirtualMemory = GetProcAddress(hNtDll, "NtAllocateVirtualMemory");
	api->lowLevel.ntFreeVirtualMemory = GetProcAddress(hNtDll, "NtFreeVirtualMemory");

	api->lowLevel.ntQueryInformationThread = GetProcAddress(hNtDll, "NtQueryInformationThread");
	api->lowLevel.ntTerminateProcess = GetProcAddress(hNtDll, "NtTerminateProcess");

	api->lowLevel.rtlNtStatusToDosError = GetProcAddress(hNtDll, "RtlNtStatusToDosError");
	api->lowLevel.vsnprintf_s = GetProcAddress(hNtDll, "_vsnprintf_s");

	gcr = (GetCurrentRegistersFunc)GetProcAddress(hRevTracer, "GetCurrentRegisters");
	gmi = (GetMemoryInfoFunc)GetProcAddress(hRevTracer, "GetMemoryInfo");
	mmv = (MarkMemoryValueFunc)GetProcAddress(hRevTracer, "MarkMemoryValue");
	
	if ((nullptr == gcr) || (nullptr == gmi) || (nullptr == mmv)) {
		__asm int 3;
		return false;
	}

	//config->contextSize = sizeof(CustomExecutionContext);
	revCfg->entryPoint = entryPoint;
	revCfg->featureFlags = featureFlags;
	revCfg->context = this;

	revCfg->hooks[0].originalAddr = GetProcAddress(GetModuleHandle("ntdll.dll"), "KiUserExceptionDispatcher");
	revCfg->hooks[0].detourAddr = nullptr;
	revCfg->hooks[0].hookFlags = HOOK_NATIVE;
	revCfg->hookCount = 1;

	//revCfg->sCons = symbolicConstructor;


	//config->sCons = SymExeConstructor;

	hThread = CreateThread(
		NULL,
		0,
		ThreadProc,
		this,
		0,
		NULL
	);

	InitSegments(hThread, revCfg->segmentOffsets);
	
	return NULL != hThread;
}

typedef void(*InitializeFunc)();
typedef void(*ExecuteFunc)(int argc, char *argv[]);

DWORD InprocessExecutionController::ControlThread() {
	HMODULE hRevTracer = GetModuleHandleW(L"revtracer.dll");

	InitializeFunc revtracerInitialize = (InitializeFunc)GetProcAddress(hRevTracer, "Initialize");
	ExecuteFunc revtraceExecute = (ExecuteFunc)GetProcAddress(hRevTracer, "Execute");

	revtracerInitialize();
	revtraceExecute(0, nullptr);

	execState = TERMINATED;
	observer->TerminationNotification(context);
	return 0;
}

bool InprocessExecutionController::WaitForTermination() {
	WaitForSingleObject(hThread, INFINITE);
	return true;
}

#include <Windows.h>
#include <intrin.h>
rev::ADDR_TYPE InprocessExecutionController::GetExceptingIp(rev::ADDR_TYPE hookAddr, void *cbCtx) {
	rev::ExecutionRegs regs;
	
	GetCurrentRegisters(cbCtx, &regs);
	EXCEPTION_RECORD *exCtx = *(EXCEPTION_RECORD **)(regs.esp + 0);
	return (rev::ADDR_TYPE)exCtx->ExceptionAddress;
}

void CopyException(DWORD &dstSp, DWORD &srcSp, rev::ADDR_TYPE newIp) {
	PCONTEXT pCtx = (PCONTEXT)((DWORD *)srcSp)[1];
	DWORD copySize = pCtx->Esp - srcSp;

	dstSp -= copySize;
	memcpy((void *)dstSp, (void *)srcSp, copySize);
	((DWORD *)dstSp)[0] += dstSp - srcSp;
	((DWORD *)dstSp)[1] += dstSp - srcSp;
	srcSp += copySize;


	// we can switch stacks now
	pCtx = (PCONTEXT)((DWORD *)dstSp)[1];
	pCtx->Esp = dstSp + copySize;
	pCtx->Eip = (DWORD)newIp;
}

void InprocessExecutionController::SetExceptingIp(rev::ADDR_TYPE hookAddr, rev::ADDR_TYPE newIp, rev::ADDR_TYPE *newStack, void *cbCtx) {
	rev::ExecutionRegs regs;

	GetCurrentRegisters(cbCtx, &regs);

	CopyException((DWORD &)*newStack, regs.esp, newIp);
	//(*(PCONTEXT *)((DWORD)newStack + 4))->Eip = (DWORD)newIp;
	//(*(CONTEXT **)(regs.esp + 4))->Esp = (DWORD)newStack;
	(*(EXCEPTION_RECORD **)(*newStack))->ExceptionAddress = (PVOID)newIp;
}

void InprocessExecutionController::ApplyHook(rev::ADDR_TYPE origAddr, rev::ADDR_TYPE hookedAddr, void *cbCtx) {
	union _TMP {
		LONG64 asLongLong;
		BYTE asBytes[8];
	} llPatch, llOrig, *llKiUsr = (_TMP *)origAddr;
	DWORD oldProtect;

	llPatch.asLongLong = llOrig.asLongLong = llKiUsr->asLongLong;
	llPatch.asBytes[0] = 0xe9;
	*(DWORD *)&llPatch.asBytes[1] = (BYTE *)hookedAddr - ((BYTE *)origAddr + 5);

	VirtualProtect(origAddr, 5, PAGE_EXECUTE_READWRITE, &oldProtect);
	InterlockedCompareExchange64((volatile LONG64 *)origAddr, llPatch.asLongLong, llOrig.asLongLong);
	VirtualProtect(origAddr, 5, oldProtect, &oldProtect);
	FlushInstructionCache(GetCurrentProcess(), origAddr, 5);
}

