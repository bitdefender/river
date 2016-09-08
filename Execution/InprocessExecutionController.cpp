#include "InprocessExecutionController.h"
#include "RiverStructs.h"

#ifdef _LINUX
#define USE_MANUAL_LOADING
#endif

//#define USE_MANUAL_LOADING

#ifdef USE_MANUAL_LOADING
#include "Loader/PE.ldr.h"
#include "Loader/Inproc.Mapper.h"
typedef FloatingPE *MODULE_PTR;
typedef DWORD BASE_PTR;

// keep in mind that this funcion does NOT return the module base
void ManualLoadLibrary(const wchar_t *libName, MODULE_PTR &module, BASE_PTR &base) {
	FloatingPE *fPE = new FloatingPE(libName);
	InprocMapper mpr;
	base = 0;

	if (!fPE->IsValid()) {
		delete fPE;
		return;
	}

	if (!fPE->MapPE(mpr, base)) {
		delete fPE;
		return;
	}

	module = fPE;
}

void *ManualGetProcAddress(MODULE_PTR module, BASE_PTR base, const char *funcName) {
	DWORD rva;
	if (!module->GetExport(funcName, rva)) {
		return nullptr;
	}

	return (void *)(base + rva);
}

#define LOAD_LIBRARYW(libName, module, base) ManualLoadLibrary((libName), (module), (base))
#define GET_PROC_ADDRESS(module, base, name) ManualGetProcAddress((module), (base), (name))
#define UNLOAD_MODULE(module) delete (module)
#else
typedef HMODULE BASE_PTR;
typedef void *MODULE_PTR;

#define LOAD_LIBRARYW(libName, module, base) do { base = LoadLibraryW((libName)); module = nullptr; } while (false);
#define GET_PROC_ADDRESS(module, base, name) GetProcAddress((base), (name))
#define UNLOAD_MODULE(module)
#endif

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

typedef void(*InitializeFunc)();
typedef void(*ExecuteFunc)(int argc, char *argv[]);

InitializeFunc revtracerInitialize = nullptr;
ExecuteFunc revtraceExecute = nullptr;

bool InprocessExecutionController::Execute() {
	HMODULE hNtDll = GetModuleHandleW(L"ntdll.dll");
	MODULE_PTR hRevTracerModule;
	BASE_PTR hRevTracerBase;
		
	LOAD_LIBRARYW(L"revtracer.dll", hRevTracerModule, hRevTracerBase);

	rev::RevtracerAPI *api = (rev::RevtracerAPI *)GET_PROC_ADDRESS(hRevTracerModule, hRevTracerBase, "revtracerAPI");
	revCfg = (rev::RevtracerConfig *)GET_PROC_ADDRESS(hRevTracerModule, hRevTracerBase, "revtracerConfig");

	api->dbgPrintFunc = ::DebugPrintf;

	api->branchHandler = BranchHandlerFunc;
	api->syscallControl = SyscallControlFunc;

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

	gcr = (GetCurrentRegistersFunc)GET_PROC_ADDRESS(hRevTracerModule, hRevTracerBase, "GetCurrentRegisters");
	gmi = (GetMemoryInfoFunc)GET_PROC_ADDRESS(hRevTracerModule, hRevTracerBase, "GetMemoryInfo");
	mmv = (MarkMemoryValueFunc)GET_PROC_ADDRESS(hRevTracerModule, hRevTracerBase, "MarkMemoryValue");

	if ((nullptr == gcr) || (nullptr == gmi) || (nullptr == mmv)) {
		__asm int 3;
		return false;
	}

	//config->contextSize = sizeof(CustomExecutionContext);
	revCfg->entryPoint = entryPoint;
	revCfg->featureFlags = featureFlags;
	revCfg->context = this;
	//revCfg->sCons = symbolicConstructor;


	//config->sCons = SymExeConstructor;

	revtracerInitialize = (InitializeFunc)GET_PROC_ADDRESS(hRevTracerModule, hRevTracerBase, "Initialize");
	revtraceExecute = (ExecuteFunc)GET_PROC_ADDRESS(hRevTracerModule, hRevTracerBase, "Execute");
	UNLOAD_MODULE(hRevTracerModule);

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


DWORD InprocessExecutionController::ControlThread() {
	HMODULE hRevTracer = GetModuleHandleW(L"revtracer.dll");

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

