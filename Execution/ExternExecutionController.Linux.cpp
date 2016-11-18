#ifdef __linux__
#include "ExternExecutionController.h"

#include <sys/ptrace.h>
#include <sys/wait.h>
#include <sys/user.h>
#include <string.h>
#include <iostream>
#include "../libproc/os-linux.h"

// TODO seach for this lib in LD_LIBRARY_PATH
#define LOADER_PATH "/home/alex/river/tracer.simple/inst/lib/libloader.so"


unsigned long ExternExecutionController::ControlThread() {
	//TODO
	printf("I am the control thread function\n");
	return 0;
}

void *ControlThreadFunc(void *ptr) {
	ExternExecutionController *ctr = (ExternExecutionController *)ptr;
	ctr->ControlThread();
	return nullptr;
}

ExternExecutionController::ExternExecutionController() {
	shmAlloc = -1;
}

bool ExternExecutionController::SetEntryPoint() {
	return false;
}

THREAD_T ExternExecutionController::GetProcessHandle() {
	// N/A for the linux version
	return -1;
}


bool ExternExecutionController::PatchProcess() {
	return true;
}


bool ExternExecutionController::WaitForTermination() {
	int ret;
	JOIN_THREAD(hControlThread, ret);
	return ret;
}

unsigned int ExternExecutionController::ExecutionBegin(void *address, void *cbCtx) {
	if (!PatchProcess()) {
		execState = ERR;
		return EXECUTION_TERMINATE;
	}
	else {
		execState = SUSPENDED_AT_START;
		return observer->ExecutionBegin(cbCtx, address);
	}
}


// reads the child process memory
bool ExternExecutionController::ReadProcessMemory(unsigned int base, unsigned int size, unsigned char *buff) {
	return true;
}


bool ExternExecutionController::InitializeAllocator() {
	shmAlloc = shm_open("/thug_life", O_CREAT | O_RDWR | O_TRUNC | O_EXCL, 0644);
	if (shmAlloc == -1) {
		printf("Could not allocate shared memory chunk. Exiting. %d\n", errno);
		strerror(errno);
		return false;
	}

	return true;
}

void ExternExecutionController::ConvertWideStringPath(char *result, size_t len) {
	memset(result, 0, len);
	const wchar_t *pathStream = path.c_str();
	std::wcstombs(result, pathStream, len);
	std::cout << "[EXTERN EXECUTION] Tracee path is [" << result << "]\n";
}

const int long_size = sizeof(long);

void getdata(pid_t child, long addr,
		unsigned char *str, int len)
{   unsigned char *laddr;
	int i, j;
	union u {
		long val;
		char chars[long_size];
	}data;
	i = 0;
	j = len / long_size;
	laddr = str;
	while(i < j) {
		data.val = ptrace(PTRACE_PEEKDATA, child,
				addr + i * 4, nullptr);
		memcpy(laddr, data.chars, long_size);
		++i;
		laddr += long_size;
	}
	j = len % long_size;
	if(j != 0) {
		data.val = ptrace(PTRACE_PEEKDATA, child,
				addr + i * 4, nullptr);
		memcpy(laddr, data.chars, j);
	}
	str[len] = '\0';
}
void putdata(pid_t child, long addr,
		unsigned char *str, int len)
{   unsigned char *laddr;
	int i, j;
	union u {
		long val;
		char chars[long_size];
	}data;
	i = 0;
	j = len / long_size;
	laddr = str;
	while(i < j) {
		memcpy(data.chars, laddr, long_size);
		ptrace(PTRACE_POKEDATA, child,
				addr + i * 4, data.val);
		++i;
		laddr += long_size;
	}
	j = len % long_size;
	if(j != 0) {
		memcpy(data.chars, laddr, j);
		ptrace(PTRACE_POKEDATA, child,
				addr + i * 4, data.val);
	}
}

void InsertBreakpoint(DWORD address, pid_t child, unsigned char *bkp) {
	unsigned char code[] = {0xcd,0x80,0xcc,0};

	printf("[Parent] Inserting breakpoint at address %lx\n", address);
	getdata(child, address, bkp, 3);
	putdata(child, address, code, 3);

}

void DeleteBreakpoint(DWORD address, pid_t child, unsigned char *bkp) {
	printf("[Parent] Deleting breakpoint at address %lx\n", address);
	putdata(child, address, bkp, 3);
}

void ExternExecutionController::MapSharedLibraries(unsigned long baseAddress) {
	CreateModule(L"libipc.so", hIpcModule);
	LOAD_LIBRARYW(L"librevtracerwrapper.so", hRevWrapperModule, hRevWrapperBase);
	CreateModule(L"revtracer.dll", hRevtracerModule);

	DWORD dwIpcLibSize = (hIpcModule->GetRequiredSize() + 0xFFFF) & ~0xFFFF;
	DWORD dwRevTracerSize = (hRevtracerModule->GetRequiredSize() + 0xFFFF) & ~0xFFFF;
	DWORD dwTotalSize = dwIpcLibSize + dwRevTracerSize;

	hIpcBase = baseAddress;
	hRevtracerBase = hIpcBase + dwIpcLibSize;

	MapModule(hIpcModule, hIpcBase);
	MapModule(hRevtracerModule, hRevtracerBase);

	printf("[Parent] Mapped ipclib@0x%08lx and revtracer@0x%08lx\n", (DWORD)hIpcBase, (DWORD)hRevtracerBase);
}

unsigned long GetLoaderAddress(pid_t pid) {
	struct map_iterator mi;
	struct map_prot mp;
	unsigned long hi;
	unsigned long segbase, mapoff;


	if (maps_init (&mi, pid) < 0) {
		printf("Cannot find maps for pid %d\n", pid);
		return 0;
	}

	while (maps_next (&mi, &segbase, &hi, &mapoff, &mp)) {
		if (nullptr != strstr(mi.path, LOADER_PATH)) {
			printf("[Parent] Found child mapping for ld_preload %p\n", (void*)segbase);
			maps_close(&mi);
			return segbase;
		}

	}
	maps_close(&mi);
	return 0;
}

bool ExternExecutionController::InitializeIpcLib() {
	/* Imports */
	ipc::IpcAPI *ipcAPI;
	if (!LoadExportedName(hIpcModule, hIpcBase, "ipcAPI", ipcAPI)) {
		return false;
	}

	ipcAPI->ntYieldExecution = GET_PROC_ADDRESS(hRevWrapperModule, hRevWrapperBase, "CallYieldExecution");
	//ntdllYieldExecution = (NtYieldExecutionFunc)ipcAPI->ntYieldExecution;
	ipcAPI->vsnprintf_s = GET_PROC_ADDRESS(hRevWrapperModule, hRevWrapperBase, "CallFormattedPrintHandler");

	ipcAPI->ldrMapMemory = pLdrMapMemory;


	/* Exports */
	if (!LoadExportedName(hIpcModule, hIpcBase, "debugLog", debugLog) ||
		!LoadExportedName(hIpcModule, hIpcBase, "ipcToken", ipcToken) ||
		!LoadExportedName(hIpcModule, hIpcBase, "ipcData", ipcData) ||
		!LoadExportedName(hIpcModule, hIpcBase, "Initialize", tmpRevApi.ipcLibInitialize) ||
		!LoadExportedName(hIpcModule, hIpcBase, "DebugPrint", tmpRevApi.dbgPrintFunc) ||
		!LoadExportedName(hIpcModule, hIpcBase, "MemoryAllocFunc", tmpRevApi.memoryAllocFunc) ||
		!LoadExportedName(hIpcModule, hIpcBase, "MemoryFreeFunc", tmpRevApi.memoryFreeFunc) ||
		!LoadExportedName(hIpcModule, hIpcBase, "TakeSnapshot", tmpRevApi.takeSnapshot) ||
		!LoadExportedName(hIpcModule, hIpcBase, "RestoreSnapshot", tmpRevApi.restoreSnapshot) ||
		!LoadExportedName(hIpcModule, hIpcBase, "InitializeContextFunc", tmpRevApi.initializeContext) ||
		!LoadExportedName(hIpcModule, hIpcBase, "CleanupContextFunc", tmpRevApi.cleanupContext) ||
		!LoadExportedName(hIpcModule, hIpcBase, "BranchHandlerFunc", tmpRevApi.branchHandler) ||
		!LoadExportedName(hIpcModule, hIpcBase, "SyscallControlFunc", tmpRevApi.syscallControl) ||
		!LoadExportedName(hIpcModule, hIpcBase, "IsProcessorFeaturePresent", pIPFPFunc)
	) {
		return false;
	}

	return true;
}

bool ExternExecutionController::InitializeRevtracer() {
	/* Imports */
	tmpRevApi.lowLevel.ntAllocateVirtualMemory = GET_PROC_ADDRESS(hRevWrapperModule, hRevWrapperBase, "CallAllocateMemoryHandler");
	tmpRevApi.lowLevel.ntFreeVirtualMemory = GET_PROC_ADDRESS(hRevWrapperModule, hRevWrapperBase, "CallFreeMemoryHandler");

	tmpRevApi.lowLevel.ntQueryInformationThread = nullptr;
	tmpRevApi.lowLevel.ntTerminateProcess = GET_PROC_ADDRESS(hRevWrapperModule, hRevWrapperBase, "CallTerminateProcessHandler");

#ifdef DUMP_BLOCKS
	tmpRevApi.lowLevel.ntWriteFile = GET_PROC_ADDRESS(hRevWrapperModule, hRevWrapperBase, "CallWriteFile");
	tmpRevApi.lowLevel.ntWaitForSingleObject = nullptr;
#endif

	tmpRevApi.lowLevel.rtlNtStatusToDosError = nullptr;
	tmpRevApi.lowLevel.vsnprintf_s = GET_PROC_ADDRESS(hRevWrapperModule, hRevWrapperBase, "CallFormattedPrintHandler");

	if (nullptr != symbCb) {
		tmpRevApi.symbolicHandler = symbCb;
	}

	rev::RevtracerAPI *revAPI;
	if (!LoadExportedName(hRevtracerModule, hRevtracerBase, "revtracerAPI", revAPI) ||
		!LoadExportedName(hRevtracerModule, hRevtracerBase, "revtracerConfig", revCfg)
		) {
		return false;
	}

	memcpy(revAPI, &tmpRevApi, sizeof(tmpRevApi));

	if (!LoadExportedName(hRevtracerModule, hRevtracerBase, "GetCurrentRegisters", gcr) ||
		!LoadExportedName(hRevtracerModule, hRevtracerBase, "GetMemoryInfo", gmi) ||
		!LoadExportedName(hRevtracerModule, hRevtracerBase, "MarkMemoryValue", mmv) ||
		!LoadExportedName(hRevtracerModule, hRevtracerBase, "GetLastBasicBlockCost", glbbc)
	) {
		DEBUG_BREAK;
		return false;
	}

	revCfg->context = nullptr;
	// TODO config segment offsets are not initialized
	revCfg->hookCount = 0;
	revCfg->featureFlags = featureFlags;

#ifdef DUMP_BLOCKS
	revCfg->dumpBlocks = TRUE;

	/* The file descriptor will be open in the child as well */
	hBlocksFd = OPEN_FILE_RW("blocks.bin");
#else
	revCfg->dumpBlocks = FALSE;
	revCfg->hBlocks = nullptr;
#endif

	/* Exports */
	if (!LoadExportedName(hRevtracerModule, hRevtracerBase, "RevtracerPerform", revtracerPerform)) {
		return false;
	}

	return true;
}

bool ExternExecutionController::Execute() {

	if (!InitializeAllocator()) {
		execState = ERR;
		DEBUG_BREAK;
		return false;
	}

	pid_t child;
	int status;
	struct user_regs_struct regs;

	/* int 0x80, int3 */
	unsigned char backup[4];

	char arg[MAX_PATH];
	DWORD entryPoint;

	ConvertWideStringPath(arg, MAX_PATH);
	entryPoint = GetEntryPoint(arg);

	unsigned long shmAddress = 0x0;
	child = fork();
	if(child == 0) {
		ptrace(PTRACE_TRACEME, 0, nullptr, nullptr);

		char *const args[] = {arg, nullptr};
		char env[MAX_PATH] = "LD_PRELOAD=";
		strcat(env, LOADER_PATH);
		char *const envs[] = {env, nullptr};
		int ret = execve(arg, args, envs);

		if (ret == -1) {
			printf("Cannot start child process %s. Execve failed!\n", arg);
		}
	}

	else {
		wait(&status);

		ptrace(PTRACE_GETREGS, child, 0, &regs);
		printf("[Parent] Child started. EIP = 0x%08lx\n", regs.eip);
		fflush(stdout);

		InsertBreakpoint(entryPoint, child, backup);

		ptrace(PTRACE_CONT, child, nullptr, nullptr);
		wait(nullptr);

		ptrace(PTRACE_GETREGS, child, 0, &regs);
		printf("[Parent] Child should break at entry point. EIP = 0x%08lx\n", regs.eip);
		fflush(stdout);

		DeleteBreakpoint(entryPoint, child, backup);


		unsigned long symbolOffset = 0x2014;
		unsigned long symbolAddress = GetLoaderAddress(child) + symbolOffset;
		unsigned char sym_addr[4] = {0, 0, 0, 0};
		printf("[Parent] Trying to read data from child %08lx\n", symbolAddress);
		getdata(child, symbolAddress, sym_addr, 4);

		for (int i = 0; i < 4; ++i) {
			((char*)&shmAddress)[i] = sym_addr[i];
		}

		printf("[Parent] Received shared mem address %08lx\n", shmAddress);
		printf("[Parent] Mapped shared memory at address %08lx\n", shmAddress);

		MapSharedLibraries(shmAddress);
		InitializeIpcLib();
		InitializeRevtracer();

		printf("[Parent] Passing execution control to revtracerPerform\n");
		DEBUG_BREAK;
		regs.eip = (unsigned long) revtracerPerform;
		ptrace (PTRACE_SETREGS, child, 0, &regs);

		ptrace (PTRACE_DETACH, child, nullptr, nullptr);

		int ret;
		CREATE_THREAD(hControlThread, ControlThreadFunc, this, ret);
		execState = RUNNING;

		return ret == TRUE;
	}

}
#endif
