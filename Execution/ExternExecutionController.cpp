#include "ExternExecutionController.h"

#include "../BinLoader/LoaderAPI.h"
#include "../BinLoader/Extern.Mapper.h"

#include "RiverStructs.h"

//#define DUMP_BLOCKS

#include <Psapi.h>
#include <io.h>
#include <fcntl.h>

typedef long NTSTATUS;
typedef NTSTATUS(*NtYieldExecutionFunc)();
typedef HANDLE(*GetHandlerCallback)(void);

NtYieldExecutionFunc ntdllYieldExecution;

namespace ipc {
	void NtDllNtYieldExecution() {
		::ntdllYieldExecution();
	}
};

template <typename T> bool LoadExportedName(MODULE_PTR &module, BASE_PTR &base, char *name, T *&ptr) {
	ptr = (T *)GET_PROC_ADDRESS(module, base, name);
	return ptr != nullptr;
}

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

	shmAlloc = new DualAllocator(1 << 30, hProcess, "Local\\MumuMem", si.dwAllocationGranularity);
	return NULL != shmAlloc;
}

BYTE *GetFreeRegion(HANDLE hProcess, DWORD size) {
	printf("Looking for a 0x%08x block\n", size);

	size = (size + 0xFFF) & ~0xFFF;

	DWORD dwGran = 0x10000;

	// now look for a suitable address;

	DWORD dwOffset = 0x01000000; // dwGran;
	DWORD dwCandidate = 0, dwCandidateSize = 0xFFFFFFFF;

	while (dwOffset < 0x2FFF0000) {
		MEMORY_BASIC_INFORMATION32 mbi;
		DWORD regionSize = 0xFFFFFFFF;
		bool regionFree = true;

		if (0 == VirtualQueryEx(hProcess, (LPCVOID)dwOffset, (PMEMORY_BASIC_INFORMATION)&mbi, sizeof(mbi))) {
			return NULL;
		}

		DWORD dwSize = mbi.RegionSize - (dwOffset - mbi.BaseAddress); // or allocationbase
		if (regionSize > dwSize) {
			regionSize = dwSize;
		}

		regionFree &= (MEM_FREE == mbi.State);
		
		if (regionFree & (regionSize >= size) & (regionSize < dwCandidateSize)) {
			printf("    Candidate found @0x%08x size 0x%08x\n", dwOffset, regionSize);
			dwCandidate = dwOffset;
			dwCandidateSize = regionSize;

			if (regionSize == size) {
				break;
			}
		}

		dwOffset += regionSize;
		dwOffset += dwGran - 1;
		dwOffset &= ~(dwGran - 1);
	}

	if (0 == dwCandidate) {
		return NULL;
	}

	return (BYTE *)dwCandidate;
}

bool ExternExecutionController::MapLoader() {
	CreateModule(L"loader.dll", hLoaderModule);
	DWORD dwWritten;
	bool bRet = false;
	do {

		if (!hLoaderModule->IsValid()) {
			DEBUG_BREAK;
			break;
		}

		hLoaderBase = (DWORD)GetFreeRegion(hProcess, hLoaderModule->GetRequiredSize());
		MapModuleExtern(hLoaderModule, hLoaderBase, hProcess);

		FlushInstructionCache(hProcess, (LPVOID)hLoaderBase, hLoaderModule->GetRequiredSize());

		wchar_t revWrapperPath[] = L"revtracer-wrapper.dll";

		LOAD_LIBRARYW(revWrapperPath, hRevWrapperModule, hRevWrapperBase);

		HANDLE initHandler = GET_PROC_ADDRESS(hRevWrapperModule, hRevWrapperBase, "InitRevtracerWrapper");
		if (!initHandler) {
			DEBUG_BREAK;
			return false;
		}

		if (nullptr == ((GetHandlerCallback)initHandler)()) {
			DEBUG_BREAK;
			return false;
		}

		ldr::LoaderAPI ldrAPI;

		//ldrAPI.ntOpenSection = GetProcAddress(hNtDll, "NtOpenSection");
		ldrAPI.ntMapViewOfSection = GET_PROC_ADDRESS(hRevWrapperModule, hRevWrapperBase, "CallMapMemoryHandler");

		//ldrAPI.ntOpenDirectoryObject = GetProcAddress(hNtDll, "NtOpenDirectoryObject");
		//ldrAPI.ntClose = GetProcAddress(hNtDll, "NtClose");

		ldrAPI.ntFlushInstructionCache = GET_PROC_ADDRESS(hRevWrapperModule, hRevWrapperBase, "CallFlushInstructionCache");
		ldrAPI.ntFreeVirtualMemory = GET_PROC_ADDRESS(hRevWrapperModule, hRevWrapperBase, "CallFreeMemoryHandler");

		//ldrAPI.rtlInitUnicodeStringEx = GetProcAddress(hNtDll, "RtlInitUnicodeStringEx");
		//ldrAPI.rtlFreeUnicodeString = GetProcAddress(hNtDll, "RtlFreeUnicodeString");


		BYTE *ldrAPIPtr;
		if (!LoadExportedName(hLoaderModule, hLoaderBase, "loaderAPI", ldrAPIPtr) ||
			!LoadExportedName(hLoaderModule, hLoaderBase, "loaderConfig", pLoaderConfig) ||
			!LoadExportedName(hLoaderModule, hLoaderBase, "MapMemory", pLdrMapMemory) ||
			!LoadExportedName(hLoaderModule, hLoaderBase, "LoaderPerform", pLoaderPerform)
		) {
			DEBUG_BREAK;
			break;
		}

		if (FALSE == WriteProcessMemory(hProcess, ldrAPIPtr, &ldrAPI, sizeof(ldrAPI), &dwWritten)) {
			DEBUG_BREAK;
			break;
		}


		memset(&loaderConfig, 0, sizeof(loaderConfig));
		loaderConfig.sharedMemory = shmAlloc->CloneTo(hProcess);

		bRet = true;
	} while (false);
	delete hLoaderModule;
	return bRet;
}

DWORD CharacteristicsToDesiredAccess(DWORD c) {
	DWORD r = 0;

	r |= (c & IMAGE_SCN_MEM_READ) ? FILE_MAP_READ : 0;
	r |= (c & IMAGE_SCN_MEM_WRITE) ? FILE_MAP_WRITE : 0;
	r |= (c & IMAGE_SCN_MEM_EXECUTE) ? FILE_MAP_EXECUTE : 0;
	return r;
}

bool ExternExecutionController::InitializeIpcLib() {
	/* Imports */
	ipc::IpcAPI *ipcAPI;
	if (!LoadExportedName(hIpcModule, hIpcBase, "ipcAPI", ipcAPI)) {
		return false;
	}

	ipcAPI->ntYieldExecution = GET_PROC_ADDRESS(hRevWrapperModule, hRevWrapperBase, "CallYieldExecution");
	ntdllYieldExecution = (NtYieldExecutionFunc)ipcAPI->ntYieldExecution;
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

	/*if (nullptr != trackCb) {
		api->trackCallback = trackCb;
	}

	if (nullptr != markCb) {
		api->markCallback = markCb;
	}*/

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
		!LoadExportedName(hRevtracerModule, hRevtracerBase, "GetLastBasicBlockInfo", glbbc)
	) {
		DEBUG_BREAK;
		return false;
	} 

	revCfg->context = nullptr;
	InitSegments(hMainThread, revCfg->segmentOffsets);
	revCfg->hookCount = 0;
	revCfg->featureFlags = featureFlags;
	//revCfg->sCons = symbolicConstructor;

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
	revCfg->dumpBlocks = FALSE;
	revCfg->hBlocks = INVALID_HANDLE_VALUE;
#endif

	/* Exports */
	if (!LoadExportedName(hRevtracerModule, hRevtracerBase, "RevtracerPerform", revtracerPerform)) {
		return false;
	}


	return true;
}

bool ExternExecutionController::MapTracer() {
	CreateModule(L"ipclib.dll", hIpcModule);
	CreateModule(L"revtracer.dll", hRevtracerModule);
	bool bRet = false;
	do {
		DWORD dwIpcLibSize = (hIpcModule->GetRequiredSize() + 0xFFFF) & ~0xFFFF;
		DWORD dwRevTracerSize = (hRevtracerModule->GetRequiredSize() + 0xFFFF) & ~0xFFFF;
		DWORD dwTotalSize = dwIpcLibSize + dwRevTracerSize;

		DWORD dwOffset;

		DWORD libs = (DWORD)shmAlloc->Allocate(dwTotalSize, dwOffset);
		hIpcBase = libs;
		hRevtracerBase = libs + dwIpcLibSize;
		DWORD ipcLibOffset = dwOffset;
		DWORD revTracerOffset = dwOffset + dwIpcLibSize;

		//TODO fix this map
		MapModule(hIpcModule, hIpcBase);
		MapModule(hRevtracerModule, hRevtracerBase);

		printf("ipclib@0x%08x\nrevtracer@0x%08x\n", (DWORD)hIpcBase, (DWORD)hRevtracerBase);
		FlushInstructionCache(hProcess, (BYTE*)libs, dwTotalSize);


		DWORD sCount = 0;
		//TODO implement GetSectionCount for Elf loader
		DWORD dwIpcLibSections = dynamic_cast<ldr::FloatingPE*>(hIpcModule)->GetSectionCount();
		for (DWORD i = 0; i < dwIpcLibSections; ++i) {
			const ldr::PESection *sec = dynamic_cast<ldr::FloatingPE*>(hIpcModule)->GetSection(i);

			if (sec->header.Characteristics & IMAGE_SCN_MEM_DISCARDABLE) {
				continue;
			}

			loaderConfig.sections[sCount].mappingAddress = (ldr::ADDR_TYPE)(hIpcBase + sec->header.VirtualAddress);
			loaderConfig.sections[sCount].mappingSize = (sec->header.VirtualSize + 0xFFF) & ~0xFFF;
			loaderConfig.sections[sCount].sectionOffset = ipcLibOffset + sec->header.VirtualAddress;
			loaderConfig.sections[sCount].desiredAccess = CharacteristicsToDesiredAccess(sec->header.Characteristics);
			sCount++;
		}

		DWORD dwRevTracerSections = dynamic_cast<ldr::FloatingPE*>(hRevtracerModule)->GetSectionCount();
		for (DWORD i = 0; i < dwRevTracerSections; ++i) {
			const ldr::PESection *sec = dynamic_cast<ldr::FloatingPE*>(hRevtracerModule)->GetSection(i);

			if (sec->header.Characteristics & IMAGE_SCN_MEM_DISCARDABLE) {
				continue;
			}

			loaderConfig.sections[sCount].mappingAddress = (ldr::ADDR_TYPE)(hRevtracerBase + sec->header.VirtualAddress);
			loaderConfig.sections[sCount].mappingSize = (sec->header.VirtualSize + 0xFFF) & ~0xFFF;
			loaderConfig.sections[sCount].sectionOffset = revTracerOffset + sec->header.VirtualAddress;
			loaderConfig.sections[sCount].desiredAccess = CharacteristicsToDesiredAccess(sec->header.Characteristics);
			sCount++;
		}
		loaderConfig.sectionCount = sCount;
		loaderConfig.shmBase = (ldr::ADDR_TYPE)libs;

		if (NULL == VirtualAllocEx(
			hProcess,
			(BYTE*)libs,
			dwTotalSize,
			MEM_RESERVE,
			PAGE_READONLY
		)) {
			break;
		}

		if (!InitializeIpcLib()) {
			break;
		}
		
		if (!InitializeRevtracer()) {
			break;
		}

		loaderConfig.entryPoint = revtracerPerform;

		bRet = true;
	} while (false);

	delete hIpcModule;
	delete hRevtracerModule;
	return bRet;
}

bool ExternExecutionController::WriteLoaderConfig() {
	DWORD dwWritten;
	if (FALSE == WriteProcessMemory(hProcess, pLoaderConfig, &loaderConfig, sizeof(loaderConfig), &dwWritten)) {
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

	revCfg->entryPoint = (rev::ADDR_TYPE)ctx.Eip;

	ctx.Eip = (DWORD)pLoaderPerform;
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

	revCfg->hooks[revCfg->hookCount].originalAddr = &remoteKernel[ipfpFunc - (BYTE *)hKernel32];
	revCfg->hooks[revCfg->hookCount].detourAddr = pIPFPFunc;
	revCfg->hookCount++;


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

	if (FALSE == WriteProcessMemory(hProcess, mAddr, &tmp, sizeof(tmp), &dwWr)) {
		return false;
	}

	revCfg->mainModule = mAddr;

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

			} while (!ipcToken->Wait(REMOTE_TOKEN_USER, false));
			updated = false;

			while (!debugLog->IsEmpty()) {
				int read;
				DWORD written;
				debugLog->Read(debugBuffer, sizeof(debugBuffer)-1, read);

				BOOL ret;
				WRITE_FILE(hDbg, debugBuffer, read, written, ret);
			}

			if (STILL_ACTIVE != exitCode) {
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
				DEBUG_BREAK;
				break;
			}

			ipcToken->Release(REMOTE_TOKEN_USER);
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
