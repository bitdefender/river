#ifdef __linux__
#include "ExternExecutionController.h"

#include <sys/ptrace.h>
#include <sys/wait.h>
#include <sys/user.h>
#include <string.h>
#include <iostream>
#include "../libproc/os-linux.h"
#include "Debugger.h"
#include "DualAllocator.h"

// TODO seach for this lib in LD_LIBRARY_PATH
#define LOADER_PATH "/home/alex/river/tracer.simple/inst/lib/libloader.so"

static int ChildRunning = 1;
static void *hMapMemoryAddress = nullptr;
ldr::LoaderAPI *loaderAPI;

dbg::Debugger debugger;

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

			case REQUEST_DUMMY:
				ipcData->type = REPLY_DUMMY;
				break;
			case REQUEST_MEMORY_ALLOC: {
				DWORD offset;
				ipcData->type = REPLY_MEMORY_ALLOC;
				ipcData->data.asMemoryAllocReply.pointer = shmAlloc->Allocate(ipcData->data.asMemoryAllocRequest, offset);
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
				printf("[Parent] Received message with number %lu\n", ipcData->type);
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
	shmAlloc = nullptr;
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
		printf("path : %s base addr : %08lx high %08lx\n", mi.path, segbase, hi);
	}

	maps_close(&mi);
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
		!LoadExportedName(hRevWrapperModule, hRevWrapperBase, "CallFormattedPrintHandler", ipcAPI->vsnprintf_s) ||
		!LoadExportedName(hRevWrapperModule, hRevWrapperBase, "CallInitSemaphore", ipcAPI->initSemaphore) ||
		!LoadExportedName(hRevWrapperModule, hRevWrapperBase, "CallWaitSemaphore", ipcAPI->waitSemaphore) ||
		!LoadExportedName(hRevWrapperModule, hRevWrapperBase, "CallPostSemaphore", ipcAPI->postSemaphore) ||
		!LoadExportedName(hRevWrapperModule, hRevWrapperBase, "CallGetValueSemaphore", ipcAPI->getvalueSemaphore) ||
		!LoadExportedName(hRevWrapperModule, hRevWrapperBase, "CallDestroySemaphore", ipcAPI->destroySemaphore)
		) {
		return false;
	}

	ipcAPI->ldrMapMemory = hMapMemoryAddress;


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

	ipcToken->SetIpcApiHandler(ipcAPI);
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
	for (unsigned i = 0; i < 0x100; i++) {
		revCfg->segmentOffsets[i] = debugger.GetAndResolveModuleAddress(
				(DWORD)loaderAPI->segments + i * 4);
	}

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

	pid_t child;
	int ret, status;
	struct user_regs_struct regs;

	char arg[MAX_PATH];
	DWORD entryPoint;

	ConvertWideStringPath(arg, MAX_PATH);
	entryPoint = GetEntryPoint(arg);

	void *shmAddress = nullptr;
	const char* ld_library_path = getenv("LD_LIBRARY_PATH");
	printf("[Parent] Retrieved path %s\n", ld_library_path);
	child = fork();
	if(child == 0) {
		ptrace(PTRACE_TRACEME, 0, nullptr, nullptr);

		char *const args[] = {arg, nullptr};
		char env[MAX_PATH] = "LD_PRELOAD=";
		strcat(env, LOADER_PATH);
		char env_path[MAX_PATH] = "LD_LIBRARY_PATH=";
		strcat(env_path, ld_library_path);
		char *const envs[] = {env, env_path, nullptr};
		int ret = execve(arg, args, envs);

		if (ret == -1) {
			printf("Cannot start child process %s. Execve failed!\n", arg);
		}
	}

	else {
		debugger.Attach(child);

		debugger.InsertBreakpoint(entryPoint);
		ret = debugger.Run(PTRACE_CONT);
		debugger.PrintEip();
		if (ret == -1)
			return false;
		debugger.DeleteBreakpoint(entryPoint);

		//DebugPrintVMMap(child);

		unsigned long libloaderBase = GetLoaderAddress(child);
		MODULE_PTR libloaderModule;
		CreateModule(LOADER_PATH, libloaderModule);

		MODULE_PTR hPthreadModule;
		CreateModule(L"libipc.so", hIpcModule);
		CreateModule(L"libpthread.so", hPthreadModule);
		CreateModule(L"librevtracerwrapper.so", hRevWrapperModule);
		CreateModule(L"revtracer.dll", hRevtracerModule);

		if (!hIpcModule || !hPthreadModule || !hRevtracerModule || !hRevtracerModule) {
			printf("[Extern Execution] Please set LD_LIBRARY_PATH correctly. Modules not found\n");
			DEBUG_BREAK;
		}

		DWORD dwIpcLibSize = (hIpcModule->GetRequiredSize() + 0xFFFF) & ~0xFFFF;
		DWORD dwLibPthreadSize = (hPthreadModule->GetRequiredSize() + 0xFFFF) & ~0xFFFF;
		DWORD dwRevWrapperSize = (hRevWrapperModule->GetRequiredSize() + 0xFFFF) & ~0xFFFF;
		DWORD dwRevTracerSize = (hRevtracerModule->GetRequiredSize() + 0xFFFF) & ~0xFFFF;
		DWORD dwTotalSize = dwIpcLibSize + dwLibPthreadSize + dwRevWrapperSize + dwRevTracerSize;

		LoadExportedName(libloaderModule, libloaderBase, "loaderAPI", loaderAPI);
		// TODO dirty hack - sharedMemoryAddress offset in struct
		shmAddress = (void*)debugger.GetAndResolveModuleAddress((DWORD)loaderAPI + 12);
		printf("[Parent] Received shared mem address %08lx\n", (DWORD)shmAddress);

		// initialize dual allocator with child pid
		shmAlloc = new DualAllocator(1 << 30, child, "thug_life", 0);
		shmAlloc->SetBaseAddress((DWORD)shmAddress);
		shmAddress = (void*)shmAlloc->AllocateFixed((DWORD)shmAddress, dwTotalSize);

		if (shmAddress == (void *)-1) {
			printf("[Parent] Could not find enough space to map libraries. Exiting.");
			return -1;
		}

		printf("[Parent] Mapped shared memory at address %08lx\n", (DWORD)shmAddress);

		LoadExportedName(libloaderModule, libloaderBase, "MapMemory", hMapMemoryAddress);

		//TODO dirty hack - base members offsets in struct
		hIpcBase = debugger.GetAndResolveModuleAddress((DWORD)loaderAPI->mos + 4);
		hRevWrapperBase = debugger.GetAndResolveModuleAddress((DWORD)(loaderAPI->mos + 2) + 4);
		hRevtracerBase = debugger.GetAndResolveModuleAddress((DWORD)(loaderAPI->mos + 3) + 4);

		printf("[Parent] Found libraries mapping in shared memory ipclib@%lx revtracer@%lx revwrapper@%lx mapMemory %08lx\n",
				hIpcBase, hRevtracerBase, hRevWrapperBase, (DWORD)hMapMemoryAddress);

		InitializeIpcLib();
		//DebugPrintVMMap(child);
		InitializeRevtracer();

		//DebugPrintVMMap(getpid());
		// Setup token ring pids
		ipcToken->Init(0);
		ipcToken->Use(REMOTE_TOKEN_USER);

		printf("[Parent] Passing execution control to revtracerPerform %08lx\n", (unsigned long)revtracerPerform);
		// ipcToken object exists and called init and use.
		debugger.SetEip((unsigned long)revtracerPerform);
		revCfg->entryPoint = (void*)entryPoint;
		printf("[Parent] Entrypoint setup for revtracer to %08lx\n", (DWORD)revCfg->entryPoint);

		CREATE_THREAD(hControlThread, ControlThreadFunc, this, ret);
		execState = RUNNING;

		ChildRunning = debugger.Run(PTRACE_CONT);

		int read;
		DWORD written;
		debugLog->Read(debugBuffer, sizeof(debugBuffer)-1, read);

		BOOL _ret;
		WRITE_FILE(hDbg, debugBuffer, read, written, _ret);

		if (ChildRunning != 0) {
			printf("[Execution] Child %d tracing failed\n.", child);
			DEBUG_BREAK;
		}

		printf("[Execution] Child %d exited normally.\n", child);

		return ret == TRUE;
	}

}
#endif
