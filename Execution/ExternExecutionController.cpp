#include "ExternExecutionController.h"

#include "Loader/Extern.Mapper.h"
#include "Loader/Mem.Mapper.h"

#include "RiverStructs.h"

//#define DUMP_BLOCKS

#include <Psapi.h>
#include <io.h>
#include <fcntl.h>

typedef long NTSTATUS;
typedef NTSTATUS(*NtYieldExecutionFunc)();

NtYieldExecutionFunc ntdllYieldExecution;

namespace ipc {
	void NtDllNtYieldExecution() {
		::ntdllYieldExecution();
	}
};

template <typename T> bool LoadExportedName(FloatingPE *fpe, BYTE *base, char *name, T *&ptr) {
	DWORD rva;
	if (!fpe->GetExport(name, rva)) {
		return false;
	}

	ptr = (T *)(base + rva);
	return true;
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

	shmAlloc = new DualAllocator(1 << 30, hProcess, L"Local\\MumuMem", si.dwAllocationGranularity);
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
	FloatingPE *fLoader = new FloatingPE("loader.dll");
	ExternMapper mLoader(hProcess);
	DWORD dwWritten;
	bool bRet = false;
	do {

		pLoaderBase = GetFreeRegion(hProcess, fLoader->GetRequiredSize());
		if (!fLoader->IsValid()) {
			DEBUG_BREAK;
			break;
		}

		if (!fLoader->MapPE(mLoader, (DWORD &)pLoaderBase)) {
			DEBUG_BREAK;
			break;
		}

		FlushInstructionCache(hProcess, (LPVOID)pLoaderBase, fLoader->GetRequiredSize());

		HMODULE hNtDll = GetModuleHandleW(L"ntdll.dll");
		ldr::LoaderAPI ldrAPI;

		//ldrAPI.ntOpenSection = GetProcAddress(hNtDll, "NtOpenSection");
		ldrAPI.ntMapViewOfSection = GetProcAddress(hNtDll, "NtMapViewOfSection");

		//ldrAPI.ntOpenDirectoryObject = GetProcAddress(hNtDll, "NtOpenDirectoryObject");
		//ldrAPI.ntClose = GetProcAddress(hNtDll, "NtClose");

		ldrAPI.ntFlushInstructionCache = GetProcAddress(hNtDll, "NtFlushInstructionCache");
		ldrAPI.ntFreeVirtualMemory = GetProcAddress(hNtDll, "NtFreeVirtualMemory");

		ldrAPI.rtlFlushSecureMemoryCache = GetProcAddress(hNtDll, "RtlFlushSecureMemoryCache");

		//ldrAPI.rtlInitUnicodeStringEx = GetProcAddress(hNtDll, "RtlInitUnicodeStringEx");
		//ldrAPI.rtlFreeUnicodeString = GetProcAddress(hNtDll, "RtlFreeUnicodeString");


		BYTE *ldrAPIPtr;
		if (!LoadExportedName(fLoader, pLoaderBase, "loaderAPI", ldrAPIPtr) || 
			!LoadExportedName(fLoader, pLoaderBase, "loaderConfig", pLoaderConfig) ||
			!LoadExportedName(fLoader, pLoaderBase, "MapMemory", pLdrMapMemory) ||
			!LoadExportedName(fLoader, pLoaderBase, "LoaderPerform", pLoaderPerform)
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
	delete fLoader;
	return bRet;
}

DWORD CharacteristicsToDesiredAccess(DWORD c) {
	DWORD r = 0;

	r |= (c & IMAGE_SCN_MEM_READ) ? FILE_MAP_READ : 0;
	r |= (c & IMAGE_SCN_MEM_WRITE) ? FILE_MAP_WRITE : 0;
	r |= (c & IMAGE_SCN_MEM_EXECUTE) ? FILE_MAP_EXECUTE : 0;
	return r;
}

bool ExternExecutionController::InitializeIpcLib(FloatingPE *fIpcLib) {
	/* Imports */
	ipc::IpcAPI *ipcAPI;
	if (!LoadExportedName(fIpcLib, pIpcBase, "ipcAPI", ipcAPI)) {
		return false;
	}

	HMODULE hNtDll = GetModuleHandle("ntdll.dll");
	ipcAPI->ntYieldExecution = GetProcAddress(hNtDll, "NtYieldExecution");
	ntdllYieldExecution = (NtYieldExecutionFunc)ipcAPI->ntYieldExecution;
	ipcAPI->vsnprintf_s = GetProcAddress(hNtDll, "_vsnprintf_s");
	ipcAPI->ldrMapMemory = pLdrMapMemory;


	/* Exports */
	if (!LoadExportedName(fIpcLib, pIpcBase, "debugLog", debugLog) ||
		!LoadExportedName(fIpcLib, pIpcBase, "ipcToken", ipcToken) ||
		!LoadExportedName(fIpcLib, pIpcBase, "ipcData", ipcData) ||
		!LoadExportedName(fIpcLib, pIpcBase, "Initialize", tmpRevApi.ipcLibInitialize) ||
		!LoadExportedName(fIpcLib, pIpcBase, "DebugPrint", tmpRevApi.dbgPrintFunc) ||
		!LoadExportedName(fIpcLib, pIpcBase, "MemoryAllocFunc", tmpRevApi.memoryAllocFunc) ||
		!LoadExportedName(fIpcLib, pIpcBase, "MemoryFreeFunc", tmpRevApi.memoryFreeFunc) ||
		!LoadExportedName(fIpcLib, pIpcBase, "TakeSnapshot", tmpRevApi.takeSnapshot) ||
		!LoadExportedName(fIpcLib, pIpcBase, "RestoreSnapshot", tmpRevApi.restoreSnapshot) ||
		!LoadExportedName(fIpcLib, pIpcBase, "InitializeContextFunc", tmpRevApi.initializeContext) ||
		!LoadExportedName(fIpcLib, pIpcBase, "CleanupContextFunc", tmpRevApi.cleanupContext) ||
		!LoadExportedName(fIpcLib, pIpcBase, "BranchHandlerFunc", tmpRevApi.branchHandler) ||
		!LoadExportedName(fIpcLib, pIpcBase, "SyscallControlFunc", tmpRevApi.syscallControl) ||
		!LoadExportedName(fIpcLib, pIpcBase, "IsProcessorFeaturePresent", pIPFPFunc)
	) {
		return false;
	}
		
	return true;
}

void InitSegment(HANDLE hThread, DWORD dwSeg, DWORD &offset) {
	LDT_ENTRY entry;
	DWORD base, limit;

	if (FALSE == GetThreadSelectorEntry(hThread, dwSeg, &entry)) {
		base = 0xFFFFFFFF;
		limit = 0;
	}
	else {
		base = entry.BaseLow | (entry.HighWord.Bytes.BaseMid << 16) | (entry.HighWord.Bytes.BaseHi << 24);
		limit = entry.LimitLow | (entry.HighWord.Bits.LimitHi << 16);
	}

	if (entry.HighWord.Bits.Granularity) {
		limit = (limit << 12) | 0x0FFF;
	}

	offset = base;
}


void InitSegments(HANDLE hThread, DWORD *segments) {
	for (DWORD i = 0; i < 0x100; ++i) {
		InitSegment(hThread, i, segments[i]);
	}
}

bool ExternExecutionController::InitializeRevtracer(FloatingPE *fRevTracer) {
	/* Imports */
	HMODULE hNtDll = GetModuleHandle("ntdll.dll");
	tmpRevApi.lowLevel.ntAllocateVirtualMemory = GetProcAddress(hNtDll, "NtAllocateVirtualMemory");
	tmpRevApi.lowLevel.ntFreeVirtualMemory = GetProcAddress(hNtDll, "NtFreeVirtualMemory");

	tmpRevApi.lowLevel.ntQueryInformationThread = GetProcAddress(hNtDll, "NtQueryInformationThread");
	tmpRevApi.lowLevel.ntTerminateProcess = GetProcAddress(hNtDll, "NtTerminateProcess");

#ifdef DUMP_BLOCKS
	tmpRevApi.lowLevel.ntWriteFile = GetProcAddress(hNtDll, "NtWriteFile");
	tmpRevApi.lowLevel.ntWaitForSingleObject = GetProcAddress(hNtDll, "NtWaitForSingleObject");
#endif

	tmpRevApi.lowLevel.rtlNtStatusToDosError = GetProcAddress(hNtDll, "RtlNtStatusToDosError");
	tmpRevApi.lowLevel.vsnprintf_s = GetProcAddress(hNtDll, "_vsnprintf_s");

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
	if (!LoadExportedName(fRevTracer, pRevtracerBase, "revtracerAPI", revAPI) ||
		!LoadExportedName(fRevTracer, pRevtracerBase, "revtracerConfig", revCfg)
		) {
		return false;
	}

	memcpy(revAPI, &tmpRevApi, sizeof(tmpRevApi));

	if (!LoadExportedName(fRevTracer, pRevtracerBase, "GetCurrentRegisters", gcr) ||
		!LoadExportedName(fRevTracer, pRevtracerBase, "GetMemoryInfo", gmi) ||
		!LoadExportedName(fRevTracer, pRevtracerBase, "MarkMemoryValue", mmv)
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
	if (!LoadExportedName(fRevTracer, pRevtracerBase, "RevtracerPerform", revtracerPerform)) {
		return false;
	}


	return true;
}

bool ExternExecutionController::MapTracer() {
	FloatingPE *fIpcLib = new FloatingPE("ipclib.dll");
	FloatingPE *fRevTracer = new FloatingPE("revtracer.dll");
	bool bRet = false;
	do {
		MemMapper mMapper;

		DWORD dwIpcLibSize = (fIpcLib->GetRequiredSize() + 0xFFFF) & ~0xFFFF;
		DWORD dwRevTracerSize = (fRevTracer->GetRequiredSize() + 0xFFFF) & ~0xFFFF;
		DWORD dwTotalSize = dwIpcLibSize + dwRevTracerSize;

		DWORD dwOffset;

		BYTE *libs = (BYTE *)shmAlloc->Allocate(dwTotalSize, dwOffset);
		pIpcBase = libs;
		pRevtracerBase = libs + dwIpcLibSize;
		DWORD ipcLibOffset = dwOffset;
		DWORD revTracerOffset = dwOffset + dwIpcLibSize;

		if (!fIpcLib->MapPE(mMapper, (DWORD &)pIpcBase) || !fRevTracer->MapPE(mMapper, (DWORD &)pRevtracerBase)) {
			break;
		}

		printf("ipclib@0x%08x\nrevtracer@0x%08x\n", (DWORD)pIpcBase, (DWORD)pRevtracerBase);
		FlushInstructionCache(hProcess, libs, dwTotalSize);


		DWORD sCount = 0;
		DWORD dwIpcLibSections = fIpcLib->GetSectionCount();
		for (DWORD i = 0; i < dwIpcLibSections; ++i) {
			const PESection *sec = fIpcLib->GetSection(i);

			if (sec->header.Characteristics & IMAGE_SCN_MEM_DISCARDABLE) {
				continue;
			}

			loaderConfig.sections[sCount].mappingAddress = pIpcBase + sec->header.VirtualAddress;
			loaderConfig.sections[sCount].mappingSize = (sec->header.VirtualSize + 0xFFF) & ~0xFFF;
			loaderConfig.sections[sCount].sectionOffset = ipcLibOffset + sec->header.VirtualAddress;
			loaderConfig.sections[sCount].desiredAccess = CharacteristicsToDesiredAccess(sec->header.Characteristics);
			sCount++;
		}

		DWORD dwRevTracerSections = fRevTracer->GetSectionCount();
		for (DWORD i = 0; i < dwRevTracerSections; ++i) {
			const PESection *sec = fRevTracer->GetSection(i);

			if (sec->header.Characteristics & IMAGE_SCN_MEM_DISCARDABLE) {
				continue;
			}

			loaderConfig.sections[sCount].mappingAddress = pRevtracerBase + sec->header.VirtualAddress;
			loaderConfig.sections[sCount].mappingSize = (sec->header.VirtualSize + 0xFFF) & ~0xFFF;
			loaderConfig.sections[sCount].sectionOffset = revTracerOffset + sec->header.VirtualAddress;
			loaderConfig.sections[sCount].desiredAccess = CharacteristicsToDesiredAccess(sec->header.Characteristics);
			sCount++;
		}
		loaderConfig.sectionCount = sCount;
		loaderConfig.shmBase = libs;

		if (NULL == VirtualAllocEx(
			hProcess,
			libs,
			dwTotalSize,
			MEM_RESERVE,
			PAGE_READONLY
		)) {
			break;
		}

		if (!InitializeIpcLib(fIpcLib)) {
			break;
		}
		
		if (!InitializeRevtracer(fRevTracer)) {
			break;
		}

		loaderConfig.entryPoint = revtracerPerform;

		bRet = true;
	} while (false);

	delete fIpcLib;
	delete fRevTracer;
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
