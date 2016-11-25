#ifdef __linux__
#include "ExternExecutionController.h"

#include <sys/ptrace.h>
#include <sys/wait.h>
#include <sys/user.h>
#include <string.h>
#include <iostream>
#include "../libproc/os-linux.h"
#include "Debugger.h"

// TODO seach for this lib in LD_LIBRARY_PATH
#define LOADER_PATH "/home/alex/river/tracer.simple/inst/lib/libloader.so"

static bool ChildRunning = false;

unsigned long ExternExecutionController::ControlThread() {
	bool bRunning = true;
	DWORD exitCode;

	//HANDLE hDbg = 0;
	FILE_T hOffs = 0;

	do {

		hDbg = OPEN_FILE_W("debug.log");

		if (FAIL_OPEN_FILE(hDbg)) {
			//TerminateProcess(hProcess, 0);
			break;
		}

		hOffs = OPEN_FILE_W("bbs1.txt");

		if (FAIL_OPEN_FILE(hOffs)) {
			break;
		}


		while (bRunning) {
			do {
				if (!ChildRunning) {
					break;
				}

			} while (!ipcToken->Wait(REMOTE_TOKEN_USER, false));
			updated = false;

			while (!debugLog->IsEmpty()) {
				int read;
				DWORD written;
				debugLog->Read(debugBuffer, sizeof(debugBuffer)-1, read);

				BOOL ret;
				WRITE_FILE(hDbg, debugBuffer, read, written, ret);
			}

			if (!ChildRunning) {
				break;
			}

			switch (ipcData->type) {
			case REPLY_MEMORY_ALLOC:
			case REPLY_MEMORY_FREE:
			case REPLY_TAKE_SNAPSHOT:
			case REPLY_RESTORE_SNAPSHOT:
			case REPLY_INITIALIZE_CONTEXT:
			case REPLY_CLEANUP_CONTEXT:
			case REPLY_SYSCALL_CONTROL:
			case REPLY_BRANCH_HANDLER:
				DEBUG_BREAK;
				break;

			case REQUEST_MEMORY_ALLOC: {
				DWORD offset;
				ipcData->type = REPLY_MEMORY_ALLOC;
				//ipcData->data.asMemoryAllocReply.pointer = shmAlloc->Allocate(ipcData->data.asMemoryAllocRequest, offset);
				//TODO allocate memory in shared mem
				ipcData->data.asMemoryAllocReply.offset = offset;
				break;
			}

			case REQUEST_BRANCH_HANDLER: {
				void *context = ipcData->data.asBranchHandlerRequest.executionEnv;
				ipc::ADDR_TYPE next = ipcData->data.asBranchHandlerRequest.nextInstruction;
				ipcData->type = REPLY_BRANCH_HANDLER;
				execState = SUSPENDED;
				if (EXECUTION_TERMINATE == (ipcData->data.asBranchHandlerReply = BranchHandlerFunc(context, this, next))) {
					bRunning = false;
				}
				break;
			}

			case REQUEST_SYSCALL_CONTROL:
				ipcData->type = REPLY_SYSCALL_CONTROL;
				break;

			default:
				DEBUG_BREAK;
				break;
			}

			ipcToken->Release(REMOTE_TOKEN_USER);
		}


	} while (false);

	CLOSE_FILE(hDbg);

	execState = TERMINATED;

	observer->TerminationNotification(context); // this needs to be the last thing called!
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

void DebugPrintVMMap(pid_t pid) {
	struct map_iterator mi;
	struct map_prot mp;
	unsigned long hi;
	unsigned long segbase, mapoff;


	printf("[%d] debug vmmap ........................................\n", pid);
	if (maps_init (&mi, pid) < 0) {
		printf("Cannot find maps for pid %d\n", pid);
		return;
	}

	while (maps_next (&mi, &segbase, &hi, &mapoff, &mp)) {
		printf("path : %s base addr : %lx\n", mi.path, segbase);
	}

	maps_close(&mi);
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

	//DebugPrintVMMap();
	MapModule(hIpcModule, hIpcBase, shmAlloc, 0);
	MapModule(hRevtracerModule, hRevtracerBase, shmAlloc, dwIpcLibSize);

	printf("[Parent] Mapped ipclib@0x%08lx revtracer@0x%08lx and revwrapper@0x%08lx\n",
			(DWORD)hIpcBase, (DWORD)hRevtracerBase,
			(DWORD)hRevWrapperBase);
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

	if (!LoadExportedName(hRevWrapperModule, hRevWrapperBase, "CallYieldExecution", ipcAPI->ntYieldExecution) ||
		!LoadExportedName(hRevWrapperModule, hRevWrapperBase, "CallFormattedPrintHandler", ipcAPI->vsnprintf_s)
		) {
		return false;
	}

	ipcAPI->ldrMapMemory = pLdrMapMemory;


	/* Exports */
	if (!LoadExportedName(hIpcModule, hIpcBase, "debugLog", debugLog) ||
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
		!LoadExportedName(hIpcModule, hIpcBase, "IsProcessorFeaturePresent", pIPFPFunc) ||
		!LoadExportedName(hIpcModule, hIpcBase, "ipcToken", ipcToken)
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
		dbg::Debugger debugger;
		debugger.Attach(child);

		debugger.InsertBreakpoint(entryPoint);

		ChildRunning = debugger.Run(PTRACE_CONT);
		if (!ChildRunning) {
			printf("[Parent] Child %d exited\n", child);
			return false;
		}

		debugger.PrintEip();

		debugger.DeleteBreakpoint(entryPoint);

		DebugPrintVMMap(child);
		unsigned long symbolOffset = 0x2014;
		unsigned long symbolAddress = GetLoaderAddress(child) + symbolOffset;
		unsigned char sym_addr[5] = {0, 0, 0, 0, 0};
		printf("[Parent] Trying to read data from child %08lx\n", symbolAddress);
		debugger.GetData(symbolAddress, sym_addr, 4);

		for (int i = 0; i < 4; ++i) {
			((char*)&shmAddress)[i] = sym_addr[i];
		}

		printf("[Parent] Received shared mem address %08lx\n", shmAddress);
		printf("[Parent] Mapped shared memory at address %08lx\n", shmAddress);

		MapSharedLibraries(shmAddress);
		InitializeIpcLib();
		ipcToken->Use(getpid());
		ipcToken->Use(child);
		InitializeRevtracer();
		DebugPrintVMMap(getpid());

		printf("[Parent] Passing execution control to revtracerPerform %08lx\n", (unsigned long)revtracerPerform);
		debugger.SetEip((unsigned long)revtracerPerform);
		revCfg->entryPoint = (void*)entryPoint;

		for (int i = 0; i < 10000; i++) {
			debugger.Run(PTRACE_SINGLESTEP);
			debugger.PrintEip();
		}

		int ret;
		CREATE_THREAD(hControlThread, ControlThreadFunc, this, ret);
		execState = RUNNING;

		ChildRunning = debugger.Run(PTRACE_CONT);
		if (!ChildRunning) {
			printf("[Parent] Child %d exited\n", child);
			return false;
		}

		return ret == TRUE;
	}

}
#endif
