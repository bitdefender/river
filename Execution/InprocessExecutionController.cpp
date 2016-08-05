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


bool InprocessExecutionController::Execute() {
	HMODULE hNtDll = GetModuleHandleW(L"ntdll.dll");
	HMODULE hRevTracer = LoadLibraryW(L"revtracer.dll");

	rev::RevtracerAPI *api = (rev::RevtracerAPI *)GetProcAddress(hRevTracer, "revtracerAPI");
	revCfg = (rev::RevtracerConfig *)GetProcAddress(hRevTracer, "revtracerConfig");

	api->dbgPrintFunc = ::DebugPrintf;

	api->branchHandler = BranchHandlerFunc;
	api->syscallControl = SyscallControlFunc;

	if (nullptr != trackCb) {
		api->trackCallback = trackCb;
	}

	if (nullptr != markCb) {
		api->markCallback = markCb;
	}

	api->lowLevel.ntAllocateVirtualMemory = GetProcAddress(hNtDll, "NtAllocateVirtualMemory");
	api->lowLevel.ntFreeVirtualMemory = GetProcAddress(hNtDll, "NtFreeVirtualMemory");

	api->lowLevel.ntQueryInformationThread = GetProcAddress(hNtDll, "NtQueryInformationThread");
	api->lowLevel.ntTerminateProcess = GetProcAddress(hNtDll, "NtTerminateProcess");

	api->lowLevel.rtlNtStatusToDosError = GetProcAddress(hNtDll, "RtlNtStatusToDosError");
	api->lowLevel.vsnprintf_s = GetProcAddress(hNtDll, "_vsnprintf_s");

	gcr = (GetCurrentRegistersFunc)GetProcAddress(hRevTracer, "GetCurrentRegisters");
	gmi = (GetMemoryInfoFunc)GetProcAddress(hRevTracer, "GetMemoryInfo");
	mmn = (MarkMemoryNameFunc)GetProcAddress(hRevTracer, "MarkMemoryName");
	mmv = (MarkMemoryValueFunc)GetProcAddress(hRevTracer, "MarkMemoryValue");
	
	if ((nullptr == gcr) || (nullptr == gmi) || (nullptr == mmn) || (nullptr == mmv)) {
		__asm int 3;
		return false;
	}

	//config->contextSize = sizeof(CustomExecutionContext);
	revCfg->entryPoint = entryPoint;
	revCfg->featureFlags = featureFlags;
	revCfg->context = this;
	revCfg->sCons = symbolicConstructor;

	//config->sCons = SymExeConstructor;

	hThread = CreateThread(
		NULL,
		0,
		ThreadProc,
		this,
		0,
		NULL
	);

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


/*void InprocessExecutionController::GetCurrentRegisters(Registers &registers) {
	RemoteRuntime *ree = (RemoteRuntime *)revCfg->pRuntime;

	memcpy(&registers, (Registers *)ree->registers, sizeof(registers));
	registers.esp = ree->virtualStack;
}*/

