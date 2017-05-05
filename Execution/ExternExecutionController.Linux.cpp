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

#include "TokenRingInit.Linux.h"
#include "../wrapper.setup/Wrapper.Setup.h"

#include "../VirtualMemory/VirtualMem.h"

// TODO seach for this lib in LD_LIBRARY_PATH
//#define LOADER_PATH "/home/alex/river/tracer.simple/inst/lib/libloader.so"
#define LOADER_PATH "/home/teodor/wrk/tools/river.sdk/lin/lib/libloader.so"

static int ChildRunning = 1;
//static void *hMapMemoryAddress = nullptr;

dbg::Debugger debugger;

unsigned long ExternExecutionController::ControlThread() {
	bool bRunning = true;
	DWORD exitCode;

	//HANDLE hDbg = 0;
	FILE_T hOffs = 0;

	printf("[ControlThread] Started controlthread!\n");

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

			} while (!wrapper.pExports->tokenRing->Wait(CONTROL_PROCESS_TOKENID, false));
			updated = false;

			while (!ipc.pExports->debugLog->IsEmpty()) {
				int read;
				DWORD written;
				ipc.pExports->debugLog->Read(debugBuffer, sizeof(debugBuffer)-1, read);
				
				BOOL ret;
				WRITE_FILE(hDbg, debugBuffer, read, written, ret);
			}

			if (!ChildRunning) {
				break;
			}

			switch (ipc.pExports->ipcData->type) {
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
				ipc.pExports->ipcData->type = REPLY_DUMMY;
				break;

			case REQUEST_MEMORY_ALLOC: {
				DWORD offset;
				ipc.pExports->ipcData->type = REPLY_MEMORY_ALLOC;
				ipc.pExports->ipcData->data.asMemoryAllocReply.pointer = shmAlloc->Allocate(ipc.pExports->ipcData->data.asMemoryAllocRequest, offset);
				ipc.pExports->ipcData->data.asMemoryAllocReply.offset = offset;
				break;
			}

			case REQUEST_BRANCH_HANDLER: {
				void *context = ipc.pExports->ipcData->data.asBranchHandlerRequest.executionEnv;
				ipc::ADDR_TYPE next = ipc.pExports->ipcData->data.asBranchHandlerRequest.nextInstruction;
				ipc.pExports->ipcData->type = REPLY_BRANCH_HANDLER;
				execState = SUSPENDED;
				if (EXECUTION_TERMINATE == (ipc.pExports->ipcData->data.asBranchHandlerReply = BranchHandlerFunc(context, this, next))) {
					bRunning = false;
				}
				break;
			}

			case REQUEST_SYSCALL_CONTROL:
				ipc.pExports->ipcData->type = REPLY_SYSCALL_CONTROL;
				break;

			default:
				printf("[Parent] Received message with number %lu\n", ipc.pExports->ipcData->type);
				DEBUG_BREAK;
				break;
			}

			wrapper.pExports->tokenRing->Release(CONTROL_PROCESS_TOKENID);
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

rev::ADDR_TYPE ExternExecutionController::GetTerminationCode() {
	return wrapper.pExports->getTerminationCode();
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

bool ExternExecutionController::InitLibraryLayout() {
	libraryLayout = (ext::LibraryLayout *)vmem::GetFreeRegion(pid, getpid(), sizeof(*libraryLayout), 0x10000);
	printf("[Parent] libraryLayout will be @ %p <%ld,%d>\n", libraryLayout, pid, getpid());
	printf("[Parent] mmap(%p, %08x)\n", libraryLayout, (sizeof(*libraryLayout) + 0xFFF) & ~0xFFF);
	if (MAP_FAILED == mmap(libraryLayout, (sizeof(*libraryLayout) + 0xFFF) & ~0xFFF, PROT_READ | PROT_WRITE, MAP_ANONYMOUS | MAP_PRIVATE, -1, 0)) {
		printf("[Parent] Couldn't map loaderImports.libraries; errno %d\n", errno);
		return false;
	}
	//void *ret1 = VirtualAllocEx(hProcess, libraryLayout, sizeof(*libraryLayout), MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
	//void *ret2 = VirtualAlloc(libraryLayout, sizeof(*libraryLayout), MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);

	return true;
}

bool ExternExecutionController::InitializeWrapper() {
	if (!LoadExportedName(wrapper.module, wrapper.base, "wrapperImports", wrapper.pImports) ||
		!LoadExportedName(wrapper.module, wrapper.base, "wrapperExports", wrapper.pExports)
		) {
		return false;
	}

	wrapper.pImports->libraries = libraryLayout;
	InitWrapperOffsets(libraryLayout, wrapper.pImports);

	if (!revwrapper::InitTokenRing(wrapper.pExports->tokenRing, 2)) {
		printf("[Parent] Token ring initialization failure. Errno %d\n", errno);
		return false;
	}
	return true;
}

bool ExternExecutionController::InitializeIpcLib() {
	if (!LoadExportedName(ipc.module, ipc.base, "ipcImports", ipc.pImports) ||
		!LoadExportedName(ipc.module, ipc.base, "ipcExports", ipc.pExports)
	) {
		return false;
	}

	ipc.pImports->mapMemory = loader.vExports.mapMemory;
	ipc.pImports->vsnprintf_sFunc = wrapper.pExports->formattedPrint;
	ipc.pImports->ipcToken = wrapper.pExports->tokenRing;

	return true;
}

bool ExternExecutionController::InitializeRevtracer() {
	if (!LoadExportedName(revtracer.module, revtracer.base, "revtracerConfig", revtracer.pConfig) ||
		!LoadExportedName(revtracer.module, revtracer.base, "revtracerImports", revtracer.pImports) ||
		!LoadExportedName(revtracer.module, revtracer.base, "revtracerExports", revtracer.pExports)
		) {
		return false;
	}

	revtracer.pImports->lowLevel.ntTerminateProcess = (rev::ADDR_TYPE)wrapper.pExports->terminateProcess;
	revtracer.pImports->lowLevel.vsnprintf_s = (rev::ADDR_TYPE)wrapper.pExports->formattedPrint;
	revtracer.pImports->lowLevel.ntWriteFile = (rev::ADDR_TYPE)nullptr;
#ifdef DUMP_BLOCKS
	revtracer.pImports->lowLevel.ntWriteFile = GET_PROC_ADDRESS(wrapper.module, wrapper.base, "CallWriteFile");
#endif

	revtracer.pImports->dbgPrintFunc = (rev::DbgPrintFunc)ipc.pExports->debugPrint;

	/* Memory management function */
	revtracer.pImports->memoryAllocFunc = ipc.pExports->memoryAlloc;
	revtracer.pImports->memoryFreeFunc = ipc.pExports->memoryFree;

	/* VM Snapshot control */
	revtracer.pImports->takeSnapshot = ipc.pExports->takeSnapshot;
	revtracer.pImports->restoreSnapshot = ipc.pExports->restoreSnapshot;

	/* Execution callbacks */
	revtracer.pImports->initializeContext = ipc.pExports->initializeContext;
	revtracer.pImports->cleanupContext = ipc.pExports->cleanupContext;
	revtracer.pImports->branchHandler = ipc.pExports->branchHandler;
	revtracer.pImports->syscallControl = ipc.pExports->syscallControl;

	/* IpcLib initialization */
	revtracer.pImports->ipcLibInitialize;

	//tmpRevApi.lowLevel.ntAllocateVirtualMemory = GET_PROC_ADDRESS(wrapper.module, wrapper.base, "CallAllocateMemoryHandler");
	//tmpRevApi.lowLevel.ntFreeVirtualMemory = GET_PROC_ADDRESS(wrapper.module, wrapper.base, "CallFreeMemoryHandler");

	if (nullptr != trackCb) {
		revtracer.pImports->trackCallback = trackCb;
	}

	if (nullptr != markCb) {
		revtracer.pImports->markCallback = markCb;
	}

	if (nullptr != symbCb) {
		revtracer.pImports->symbolicHandler = symbCb;
	}

	gcr = revtracer.pExports->getCurrentRegisters;
	gmi = revtracer.pExports->getMemoryInfo;
	mmv = revtracer.pExports->markMemoryValue;
	glbbc = revtracer.pExports->getLastBasicBlockInfo;

	revtracer.pConfig->context = nullptr;
	/*for (unsigned i = 0; i < 0x100; i++) {
		loader.pImports->segmentOffsets[i] = 
			debugger.GetAndResolveModuleAddress((DWORD)loader.pImports->segments + i * 4);
	}*/

	revtracer.pConfig->hookCount = 0;
	revtracer.pConfig->featureFlags = featureFlags;
	//revtracer.pConfig->sCons = symbolicConstructor;

#ifdef DUMP_BLOCKS
	revtracer.pConfig->dumpBlocks = TRUE;

	hBlocksFd = OPEN_FILE_RW("blocks.bin");
#else
	revtracer.pConfig->dumpBlocks = FALSE;
	revtracer.pConfig->hBlocks = nullptr;
#endif

	return true;
	
}

bool ExternExecutionController::Execute() {

	pid_t child;
	int ret, status;
	struct user_regs_struct regs;

	char arg[MAX_PATH];
	DWORD entryPoint;

	ConvertWideStringPath(arg, MAX_PATH);
	//entryPoint = GetEntryPoint(arg);

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
		pid = child;
		debugger.Attach(child);
		ret = debugger.Run(PTRACE_CONT);
		debugger.PrintEip();
		debugger.PrintRegs();

		InitLibraryLayout();
		printf(
			"[Parent] Library layout @ %p\n",
			libraryLayout
		);

		loader.base = GetLoaderAddress(child);
		CreateModule(LOADER_PATH, loader.module);

		if (!loader.module) {
			printf(
				"[Extern Execution] Please set LD_LIBRARY_PATH correctly. Modules not found\n"
				"\tlibloader.so = %p @ %08lx\n",
				loader.module, loader.base
			);
			DEBUG_BREAK;
		}
		printf("[Parent] loader found at address %08lx\n", loader.base);
		

		LoadExportedName(loader.module, loader.base, "loaderConfig", loader.pConfig);
		LoadExportedName(loader.module, loader.base, "loaderImports", loader.pImports);
		LoadExportedName(loader.module, loader.base, "loaderExports", loader.pExports);
		debugger.GetData((DWORD)loader.pExports, (unsigned char *)&loader.vExports, sizeof(loader.vExports));
		printf(
			"[Loader] Layout:\n"
			"\tpConfig = %p\n"
			"\tpImports = %p\n"
			"\tpExports = %p\n"
			"\tpExports->mapMemory = %p\n",
			loader.pConfig,
			loader.pImports,
			loader.pExports,
			loader.vExports
		);


		debugger.PutData((DWORD)&loader.pImports->libraries, (unsigned char *)&libraryLayout, sizeof(libraryLayout));


		// is this safe to be removed?
		/*debugger.InsertBreakpoint(entryPoint);
		ret = debugger.Run(PTRACE_CONT);
		debugger.PrintEip();
		if (ret == -1)
			return false;
		debugger.DeleteBreakpoint(entryPoint);*/

		//DebugPrintVMMap(child);

		CreateModule(L"libipc.so", ipc.module);
		CreateModule(L"librevtracerwrapper.so", wrapper.module);
		CreateModule(L"revtracer.dll", revtracer.module);

		if (!ipc.module || !wrapper.module || !revtracer.module) {
			printf(
				"[Extern Execution] Please set LD_LIBRARY_PATH correctly. Modules not found\n"
				"\tlibipc.so = %p\n"
				"\tlibrevtracerwrapper.so = %p\n"
				"\trevtracer.dll = %p\n",
				ipc.module,
				wrapper.module,
				revtracer.module
			);
			DEBUG_BREAK;
		}

		DWORD dwIpcLibSize = (ipc.module->GetRequiredSize() + 0xFFFF) & ~0xFFFF;
		//DWORD dwLibPthreadSize = (hPthreadModule->GetRequiredSize() + 0xFFFF) & ~0xFFFF;
		DWORD dwRevWrapperSize = (wrapper.module->GetRequiredSize() + 0xFFFF) & ~0xFFFF;
		DWORD dwRevTracerSize = (revtracer.module->GetRequiredSize() + 0xFFFF) & ~0xFFFF;
		DWORD dwTotalSize = dwIpcLibSize + /*dwLibPthreadSize +*/ dwRevWrapperSize + dwRevTracerSize;

		// TODO dirty hack - sharedMemoryAddress offset in struct
		shmAddress = (void*)debugger.GetAndResolveModuleAddress((DWORD)&(loader.pConfig->shmBase));
		printf("[Parent] Received shared mem address %08lx, while reading %p\n", (DWORD)shmAddress, &(loader.pConfig->shmBase));

		// initialize dual allocator with child pid
		shmAlloc = new DualAllocator(1 << 30, child, "thug_life", 0, 0);
		shmAlloc->SetBaseAddress((DWORD)shmAddress);
		shmAddress = (void*)shmAlloc->AllocateFixed((DWORD)shmAddress, dwTotalSize);

		if (shmAddress == (void *)-1) {
			printf("[Parent] Could not find enough space to map libraries. Exiting.");
			return -1;
		}

		printf("[Parent] Mapped shared memory at address %08lx\n", (DWORD)shmAddress);

		//TODO dirty hack - base members offsets in struct
		wrapper.base = debugger.GetAndResolveModuleAddress((DWORD)&loader.pConfig->osData.linux.mos[0].base);
			//(DWORD)(loader.pConfig->osData.linux.mos + 2) + 4);
		ipc.base = debugger.GetAndResolveModuleAddress((DWORD)&loader.pConfig->osData.linux.mos[1].base);
			//(DWORD)loader.pConfig->osData.linux.mos + 4);
		revtracer.base = debugger.GetAndResolveModuleAddress((DWORD)&loader.pConfig->osData.linux.mos[2].base);
			//(DWORD)(loader.pConfig->osData.linux.mos + 3) + 4);

		printf("[Parent] Found libraries mapping in shared memory revwrapper@%lx ipclib@%lx revtracer@%lx mapMemory %08lx\n",
			wrapper.base, ipc.base, revtracer.base, (DWORD)loader.pExports->mapMemory);


		if (!InitializeWrapper()) {
			DEBUG_BREAK;
		}
		InitializeIpcLib();
		//DebugPrintVMMap(child);
		InitializeRevtracer();

		//DebugPrintVMMap(getpid());
		// Setup token ring pids
		//wrapper.pExports->tokenRing->Init(0);
		revwrapper::InitTokenRing(wrapper.pExports->tokenRing, 2);
		
		printf("[Parent] Passing execution control to revtracerPerform %08lx\n", (unsigned long)revtracer.pExports->revtracerPerform);
		// ipcToken object exists and called init and use.
		//debugger.SetEip((unsigned long)revtracer.pExports->revtracerPerform);
		
		debugger.PutData((DWORD)&loader.pConfig->entryPoint, (unsigned char *)&revtracer.pExports->revtracerPerform, sizeof(unsigned long));
		
		debugger.GetData((DWORD)&loader.pConfig->osData.linux.retAddr, (unsigned char *)&entryPoint, sizeof(entryPoint));
		printf("[Parent] First address to be translated is %08lx\n", (DWORD)entryPoint);

		revtracer.pConfig->entryPoint = (void*)entryPoint;
		printf("[Parent] Entrypoint setup for revtracer to %08lx\n", (DWORD)revtracer.pConfig->entryPoint);

		CREATE_THREAD(hControlThread, ControlThreadFunc, this, ret);
		execState = RUNNING;

		ChildRunning = debugger.Run(PTRACE_CONT);
		debugger.PrintEip();
		debugger.PrintRegs();

		int read;
		DWORD written;
		ipc.pExports->debugLog->Read(debugBuffer, sizeof(debugBuffer) - 1, read);

		BOOL _ret;
		WRITE_FILE(hDbg, debugBuffer, read, written, _ret);

		DEBUG_BREAK;

		while (-1 != debugger.Run(PTRACE_SINGLESTEP)) {
			//debugger.PrintEip();
			debugger.PrintRegs();
		}
		debugger.PrintEip();
		debugger.PrintRegs();

		printf("[Parent] Remember: revwrapper@%lx ipclib@%lx revtracer@%lx mapMemory %08lx\n",
			wrapper.base, ipc.base, revtracer.base, (DWORD)loader.pExports->mapMemory);

		ipc.pExports->debugLog->Read(debugBuffer, sizeof(debugBuffer)-1, read);

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
