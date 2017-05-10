#include "ExternExecutionController.h"

#include "../BinLoader/LoaderAPI.h"
#include "../BinLoader/Extern.Mapper.h"

#include "TokenRingInit.Windows.h"

#include "RiverStructs.h"

#include "../wrapper.setup/Wrapper.Setup.h"
#include "../loader.setup/Loader.Setup.h"

#include "../VirtualMemory/VirtualMem.h"

//#define DUMP_BLOCKS

#include <Psapi.h>
#include <io.h>
#include <fcntl.h>

typedef long NTSTATUS;
typedef bool(*GetHandlerCallback)(void *);

//NtYieldExecutionFunc ntdllYieldExecution;

/*namespace ipc {
	void NtDllNtYieldExecution() {
		::ntdllYieldExecution();
	}
};*/

ExternExecutionController::ExternExecutionController() {
	shmAlloc = NULL;
}

bool ExternExecutionController::SetEntryPoint() {
	return false;
}

bool ExternExecutionController::InitializeAllocator() {
	SYSTEM_INFO si;
	memset(&si, 0, sizeof(si));

	GetSystemInfo(&si);

	shmAlloc = new DualAllocator(1 << 30, hProcess, "Local\\MumuMem", si.dwAllocationGranularity, 0);
	return NULL != shmAlloc;
}

bool ExternExecutionController::InitLibraryLayout() {
	libraryLayout = (ext::LibraryLayout *)vmem::GetFreeRegion(hProcess, GetCurrentProcess(), sizeof(*libraryLayout), 0x10000);

	void *ret1 = VirtualAllocEx(hProcess, libraryLayout, sizeof(*libraryLayout), MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
	void *ret2 = VirtualAlloc(libraryLayout, sizeof(*libraryLayout), MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);

	return true;
}

bool ExternExecutionController::MapLoader() {
	CreateModule(L"loader.dll", loader.module);
	DWORD dwWritten;
	bool bRet = false;
	do {

		if (!loader.module->IsValid()) {
			DEBUG_BREAK;
			break;
		}

		loader.base = (BASE_PTR)vmem::GetFreeRegion(hProcess, loader.module->GetRequiredSize(), 0x10000);
		MapModuleExtern(loader.module, loader.base, hProcess);

		FlushInstructionCache(hProcess, (LPVOID)loader.base, loader.module->GetRequiredSize());

		if (!InitLibraryLayout()) {
			DEBUG_BREAK;
			break;
		}


		ldr::LoaderImports ldrAPI;
		InitLoaderOffsets(libraryLayout, &ldrAPI);
		ldrAPI.libraries = libraryLayout;

		if (!LoadExportedName(loader.module, loader.base, "loaderConfig", loader.pConfig) ||
			!LoadExportedName(loader.module, loader.base, "loaderImports", loader.pImports) ||
			!LoadExportedName(loader.module, loader.base, "loaderExports", loader.pExports)
		) {
			DEBUG_BREAK;
			break;
		}

		if (FALSE == ::WriteProcessMemory(hProcess, libraryLayout, (LPCVOID)libraryLayout, sizeof(*libraryLayout), &dwWritten)) {
			DEBUG_BREAK;
			break;
		}

		if (FALSE == ::WriteProcessMemory(hProcess, loader.pImports, &ldrAPI, sizeof(ldrAPI), &dwWritten)) {
			DEBUG_BREAK;
			break;
		}

		if (FALSE == ::ReadProcessMemory(hProcess, loader.pExports, &loader.vExports, sizeof(loader.vExports), &dwWritten)) {
			DEBUG_BREAK;
			break;
		}

		memset(&loader.vConfig, 0, sizeof(loader.vConfig));
		loader.vConfig.sharedMemory = shmAlloc->CloneTo(hProcess);

		bRet = true;
	} while (false);
	delete loader.module;
	loader.module = nullptr;
	return bRet;
}

DWORD CharacteristicsToDesiredAccess(DWORD c) {
	DWORD r = 0;

	r |= (c & IMAGE_SCN_MEM_READ) ? FILE_MAP_READ : 0;
	r |= (c & IMAGE_SCN_MEM_WRITE) ? FILE_MAP_WRITE : 0;
	r |= (c & IMAGE_SCN_MEM_EXECUTE) ? FILE_MAP_EXECUTE : 0;
	return r;
}

bool ExternExecutionController::InitializeWrapper() {
	if (!LoadExportedName(wrapper.module, wrapper.base, "wrapperImports", wrapper.pImports) ||
		!LoadExportedName(wrapper.module, wrapper.base, "wrapperExports", wrapper.pExports)
	) {
		return false;
	}

	InitWrapperOffsets(libraryLayout, wrapper.pImports);


	unsigned int pids[2] = {
		GetCurrentProcessId(),
		pid
	};

	revwrapper::InitTokenRing(wrapper.pExports->tokenRing, 2, pids);
	return true;
}

bool ExternExecutionController::InitializeIpcLib() {
	if (!LoadExportedName(ipc.module, ipc.base, "ipcImports", ipc.pImports) ||
		!LoadExportedName(ipc.module, ipc.base, "ipcExports", ipc.pExports) ||
		!LoadExportedName(ipc.module, ipc.base, "IsProcessorFeaturePresent", ipc.pIPFPFunc)
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
	glbbi = revtracer.pExports->getLastBasicBlockInfo;

	revtracer.pConfig->context = nullptr;
	InitSegments(hMainThread, revtracer.pConfig->segmentOffsets);
	revtracer.pConfig->hookCount = 0;
	revtracer.pConfig->featureFlags = featureFlags;
	//revtracer.pConfig->sCons = symbolicConstructor;

#ifdef DUMP_BLOCKS
	revCfg->dumpBlocks = TRUE;

	HANDLE hBlocks = CreateFile(
		"blocks.bin",
		GENERIC_WRITE,
		FILE_SHARE_READ,
		NULL,
		CREATE_ALWAYS,
		0,
		NULL
	);

	if (FALSE == DuplicateHandle(
		GetCurrentProcess(),
		hBlocks,
		hProcess,
		&revCfg->hBlocks,
		0,
		FALSE,
		DUPLICATE_CLOSE_SOURCE | DUPLICATE_SAME_ACCESS
	)) {
		CloseHandle(hBlocks);
		return false;
	}
#else
	revtracer.pConfig->dumpBlocks = FALSE;
	revtracer.pConfig->hBlocks = INVALID_HANDLE_VALUE;
#endif

	return true;
}

bool ExternExecutionController::MapTracer() {
	CreateModule(L"revtracer-wrapper.dll", wrapper.module);
	CreateModule(L"ipclib.dll", ipc.module);
	CreateModule(L"revtracer.dll", revtracer.module);
	bool bRet = false;
	do {
		DWORD dwWrapperSize = (wrapper.module->GetRequiredSize() + 0xFFFF) & ~0xFFFF;
		DWORD dwIpcLibSize = (ipc.module->GetRequiredSize() + 0xFFFF) & ~0xFFFF;
		DWORD dwRevTracerSize = (revtracer.module->GetRequiredSize() + 0xFFFF) & ~0xFFFF;
		DWORD dwTotalSize = dwWrapperSize + dwIpcLibSize + dwRevTracerSize;

		DWORD dwOffset;

		// Bases are module bases int the child/parent process address space
		DWORD libs = (DWORD)shmAlloc->Allocate(dwTotalSize, dwOffset);
		wrapper.base = (BASE_PTR)libs;
		ipc.base = (BASE_PTR)(libs + dwWrapperSize);
		revtracer.base = (BASE_PTR)(libs + dwWrapperSize + dwIpcLibSize);

		// Offsets are displacements relative to the shared memory
		DWORD wrapperOffset = dwOffset;
		DWORD ipcLibOffset = wrapperOffset + dwWrapperSize;
		DWORD revTracerOffset = ipcLibOffset + dwIpcLibSize;

		//TODO fix this map
		MapModule2(wrapper.module, wrapper.base);
		MapModule2(ipc.module, ipc.base);
		MapModule2(revtracer.module, revtracer.base);

		printf(
			"wrapper@0x%08x\nipclib@0x%08x\nrevtracer@0x%08x\n", 
			(DWORD)wrapper.base, 
			(DWORD)ipc.base,
			(DWORD)revtracer.base
		);
		FlushInstructionCache(hProcess, (BYTE*)libs, dwTotalSize);


		DWORD sCount = 0;
		//TODO implement GetSectionCount for Elf loader
		DWORD dwWrapperSections = dynamic_cast<ldr::FloatingPE*>(wrapper.module)->GetSectionCount();
		for (DWORD i = 0; i < dwWrapperSections; ++i) {
			const ldr::PESection *sec = dynamic_cast<ldr::FloatingPE*>(wrapper.module)->GetSection(i);

			if (sec->header.Characteristics & IMAGE_SCN_MEM_DISCARDABLE) {
				continue;
			}

			loader.vConfig.osData.windows.sections[sCount].mappingAddress = (ldr::ADDR_TYPE)((BYTE *)wrapper.base + sec->header.VirtualAddress);
			loader.vConfig.osData.windows.sections[sCount].mappingSize = (sec->header.VirtualSize + 0xFFFF) & ~0xFFFF;
			loader.vConfig.osData.windows.sections[sCount].sectionOffset = wrapperOffset + sec->header.VirtualAddress;
			loader.vConfig.osData.windows.sections[sCount].desiredAccess = CharacteristicsToDesiredAccess(sec->header.Characteristics);
			sCount++;
		}

		DWORD dwIpcLibSections = dynamic_cast<ldr::FloatingPE*>(ipc.module)->GetSectionCount();
		for (DWORD i = 0; i < dwIpcLibSections; ++i) {
			const ldr::PESection *sec = dynamic_cast<ldr::FloatingPE*>(ipc.module)->GetSection(i);

			if (sec->header.Characteristics & IMAGE_SCN_MEM_DISCARDABLE) {
				continue;
			}

			loader.vConfig.osData.windows.sections[sCount].mappingAddress = (ldr::ADDR_TYPE)((BYTE *)ipc.base + sec->header.VirtualAddress);
			loader.vConfig.osData.windows.sections[sCount].mappingSize = (sec->header.VirtualSize + 0xFFFF) & ~0xFFFF;
			loader.vConfig.osData.windows.sections[sCount].sectionOffset = ipcLibOffset + sec->header.VirtualAddress;
			loader.vConfig.osData.windows.sections[sCount].desiredAccess = CharacteristicsToDesiredAccess(sec->header.Characteristics);
			sCount++;
		}

		DWORD dwRevTracerSections = dynamic_cast<ldr::FloatingPE*>(revtracer.module)->GetSectionCount();
		for (DWORD i = 0; i < dwRevTracerSections; ++i) {
			const ldr::PESection *sec = dynamic_cast<ldr::FloatingPE*>(revtracer.module)->GetSection(i);

			if (sec->header.Characteristics & IMAGE_SCN_MEM_DISCARDABLE) {
				continue;
			}

			loader.vConfig.osData.windows.sections[sCount].mappingAddress = (ldr::ADDR_TYPE)((BYTE *)revtracer.base + sec->header.VirtualAddress);
			loader.vConfig.osData.windows.sections[sCount].mappingSize = (sec->header.VirtualSize + 0xFFFF) & ~0xFFFF;
			loader.vConfig.osData.windows.sections[sCount].sectionOffset = revTracerOffset + sec->header.VirtualAddress;
			loader.vConfig.osData.windows.sections[sCount].desiredAccess = CharacteristicsToDesiredAccess(sec->header.Characteristics);
			sCount++;
		}
		loader.vConfig.osData.windows.sectionCount = sCount;
		loader.vConfig.shmBase = (ldr::ADDR_TYPE)libs;

		// Reserve this memory chunk to prevent the windows loader on using it before our loader can do its job
		if (NULL == VirtualAllocEx(
			hProcess,
			(BYTE*)libs,
			dwTotalSize,
			MEM_RESERVE,
			PAGE_READONLY
		)) {
			break;
		}

		if (!InitializeWrapper()) {
			break;
		}

		if (!InitializeIpcLib()) {
			break;
		}
		
		if (!InitializeRevtracer()) {
			break;
		}

		loader.vConfig.entryPoint = revtracer.pExports->revtracerPerform;

		bRet = true;
	} while (false);

	delete wrapper.module;
	wrapper.module = nullptr;
	delete ipc.module;
	ipc.module = nullptr;
	delete revtracer.module;
	revtracer.module = nullptr;
	return bRet;
}

bool ExternExecutionController::WriteLoaderConfig() {
	DWORD dwWritten;
	if (FALSE == ::WriteProcessMemory(hProcess, loader.pConfig, &loader.vConfig, sizeof(loader.vConfig), &dwWritten)) {
		return false;
	}

	return true;
}

bool ExternExecutionController::SwitchEntryPoint() {
	CONTEXT ctx;
	memset(&ctx, 0, sizeof(ctx));
	ctx.ContextFlags = CONTEXT_ALL;
	if (FALSE == GetThreadContext(hMainThread, &ctx)) {
		return false;
	}

	revtracer.pConfig->entryPoint = (rev::ADDR_TYPE)ctx.Eip;

	ctx.Eip = (DWORD)loader.vExports.perform;
	if (FALSE == SetThreadContext(hMainThread, &ctx)) {
		return false;
	}

	return true;
}

BYTE *RemoteGetModuleHandle(HANDLE hProcess, const wchar_t *module) {
	BYTE *pOffset = (BYTE *)0x10000;
	while ((BYTE *)0x7FFE0000 >= pOffset) {
		MEMORY_BASIC_INFORMATION mbi;
		wchar_t moduleName[MAX_PATH];

		VirtualQueryEx(hProcess, pOffset, (PMEMORY_BASIC_INFORMATION)&mbi, sizeof(mbi));

		if ((MEM_COMMIT == mbi.State) && (SEC_IMAGE == mbi.Type) && (PAGE_READONLY == mbi.Protect)) {
			if (0 != GetMappedFileNameW(
				hProcess,
				pOffset,
				moduleName,
				sizeof(moduleName)-1
			)) {
				wchar_t *mName = moduleName;
				for (int t = wcslen(moduleName) - 1; t >= 0; --t) {
					if (L'\\' == moduleName[t]) {
						mName = &moduleName[t + 1];
						break;
					}
				}

				if (0 == _wcsicmp(mName, module)) {
					return pOffset;
				}
			}
		}

		pOffset += mbi.RegionSize;
	}

	return NULL;
}

bool ExternExecutionController::PatchProcess() {
	// hook IsProcessorFeaturePresent
	HMODULE hKernel32 = GetModuleHandle("kernel32.dll");
	BYTE *ipfpFunc = (BYTE *)GetProcAddress(hKernel32, "IsProcessorFeaturePresent");
	BYTE *remoteKernel = RemoteGetModuleHandle(hProcess, L"kernel32.dll");

	if (NULL == remoteKernel) {
		return false;
	}

	revtracer.pConfig->hooks[revtracer.pConfig->hookCount].originalAddr = &remoteKernel[ipfpFunc - (BYTE *)hKernel32];
	revtracer.pConfig->hooks[revtracer.pConfig->hookCount].detourAddr = ipc.pIPFPFunc;
	revtracer.pConfig->hookCount++;


	const wchar_t *pth = path.c_str();

	for (int t = path.length() - 1; t > 0; --t) {
		if (L'\\' == pth[t]) {
			pth = &pth[t + 1];
			break;
		}
	}

	BYTE *mAddr = RemoteGetModuleHandle(hProcess, pth);

	unsigned short tmp = '  ';
	DWORD dwWr, oldPr;

	if (FALSE == VirtualProtectEx(hProcess, mAddr, sizeof(tmp), PAGE_READWRITE, &oldPr)) {
		return false;
	}

	if (FALSE == ::WriteProcessMemory(hProcess, mAddr, &tmp, sizeof(tmp), &dwWr)) {
		return false;
	}

	revtracer.pConfig->mainModule = mAddr;

	/*if (FALSE == VirtualProtectEx(hProcess, mAddr, sizeof(tmp), oldPr, &oldPr)) {
		return false;
	}*/
	return true;
}

DWORD WINAPI ControlThreadFunc(void *ptr) {
	ExternExecutionController *ctr = (ExternExecutionController *)ptr;
	return ctr->ControlThread();
}

THREAD_T ExternExecutionController::GetProcessHandle() {
	return hProcess;
}

rev::ADDR_TYPE ExternExecutionController::GetTerminationCode() {
	return wrapper.pExports->getTerminationCode();
}

bool ExternExecutionController::Execute() {
	STARTUPINFOW startupInfo;
	PROCESS_INFORMATION processInfo;

	memset(&startupInfo, 0, sizeof(startupInfo));
	startupInfo.cb = sizeof(startupInfo);
	
	memset(&processInfo, 0, sizeof(processInfo));

	if ((execState != INITIALIZED) && (execState != TERMINATED) && (execState != ERR)) {
		DEBUG_BREAK;
		return false;
	}

	BOOL bRet = CreateProcessW(
		path.c_str(),
		L"", //cmdLine.c_str(),
		NULL,
		NULL,
		FALSE,
		CREATE_SUSPENDED | CREATE_NEW_CONSOLE,
		NULL,
		NULL,
		&startupInfo,
		&processInfo
	);

	if (FALSE == bRet) {
		execState = ERR;
		DEBUG_BREAK;
		return false;
	}

	pid = processInfo.dwProcessId;
	hProcess = processInfo.hProcess;
	hMainThread = processInfo.hThread;

	if (!InitializeAllocator()) {
		execState = ERR;
		DEBUG_BREAK;
		return false;
	}

	if (!MapLoader()) {
		execState = ERR;
		DEBUG_BREAK;
		return false;
	}

	if (!MapTracer()) {
		execState = ERR;
		DEBUG_BREAK;
		return false;
	}

	if (!WriteLoaderConfig()) {
		execState = ERR;
		DEBUG_BREAK;
		return false;
	}

	if (!SwitchEntryPoint()) {
		execState = ERR;
		DEBUG_BREAK;
		return false;
	}

	hControlThread = CreateThread(
		NULL,
		0,
		ControlThreadFunc,
		this,
		0,
		NULL
	);

	execState = RUNNING;
	ResumeThread(hMainThread);
	return true;
}

bool ExternExecutionController::WaitForTermination() {
	WaitForSingleObject(hControlThread, INFINITE);

	return true;
}

//DWORD BranchHandler(void *context, rev::ADDR_TYPE a, void *controller);

DWORD ExternExecutionController::ControlThread() {
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

		int cFile = _open_osfhandle((intptr_t)hOffs, _O_TEXT);
		FILE *fOffs = _fdopen(cFile, "wt");


		while (bRunning) {
			do {
				GetExitCodeProcess(hProcess, &exitCode);

				if (STILL_ACTIVE != exitCode) {
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

			if (STILL_ACTIVE != exitCode) {
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
				DEBUG_BREAK;
				break;
			}

			wrapper.pExports->tokenRing->Release(CONTROL_PROCESS_TOKENID);
		}


		fclose(fOffs);
	} while (false);

	CloseHandle(hDbg);
	hDbg = INVALID_HANDLE_VALUE;
	//CloseHandle(hOffs);
	//hOffs = INVALID_HANDLE_VALUE;

	WaitForSingleObject(hProcess, INFINITE);

	CloseHandle(hMainThread);
	CloseHandle(hProcess);

	execState = TERMINATED;
	delete shmAlloc;

	observer->TerminationNotification(context); // this needs to be the last thing called!
	return 0;
}

/*_declspec(dllimport) extern "C" NTSTATUS WINAPI NtQueryInformationProcess(
	_In_      HANDLE             ProcessHandle,
	_In_      MYPROCESSINFOCLASS ProcessInformationClass,
	_Out_     PVOID              ProcessInformation,
	_In_      ULONG              ProcessInformationLength,
	_Out_opt_ PULONG             ReturnLength
);*/


bool ExternExecutionController::ReadProcessMemory(unsigned int base, unsigned int size, unsigned char *buff) {
	DWORD dwRd;
	return TRUE == ::ReadProcessMemory(hProcess, (LPCVOID)base, buff, size, &dwRd);
}

bool ExternExecutionController::WriteProcessMemory(unsigned int base, unsigned int size, unsigned char *buff) {
	DWORD dwWr;
	return TRUE == ::WriteProcessMemory(hProcess, (LPVOID)base, buff, size, &dwWr);
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

/*void ExternExecutionController::GetCurrentRegisters(Registers &registers) {
	RemoteRuntime *ree = (RemoteRuntime *)revCfg->pRuntime;

	memcpy(&registers, (Registers *)ree->registers, sizeof(registers));
	registers.esp = ree->virtualStack;
}*/
