#include "InternalExecutionController.h"

#include "Loader/Extern.Mapper.h"
#include "Loader/Mem.Mapper.h"

#include "RiverStructs.h"

#include <Psapi.h>
#include <io.h>
#include <fcntl.h>
#include <tlhelp32.h>
#include <Winternl.h>

//#define DUMP_BLOCKS


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

void __stdcall DefaultTerminationNotify(void *ctx) { }

unsigned int __stdcall DefaultExecutionBegin(void *ctx, unsigned int address) {
	return EXECUTION_ADVANCE;
}

unsigned int __stdcall DefaultExecutionControl(void *ctx, unsigned int address) {
	return EXECUTION_ADVANCE;
}

unsigned int __stdcall DefaultExecutionEnd(void *ctx) {
	return EXECUTION_TERMINATE;
}

InternalExecutionController::InternalExecutionController() {
	execState = NEW;

	virtualSize = commitedSize = 0;

	path = L"";
	cmdLine = L"";
	featureFlags = EXECUTION_FEATURE_REVERSIBLE | EXECUTION_FEATURE_TRACKING;

	context = NULL;

	shmAlloc = NULL;

	term = DefaultTerminationNotify;
	eBegin = DefaultExecutionBegin;
	eControl = DefaultExecutionControl;
	eEnd = DefaultExecutionEnd;

	updated = false;
}

int InternalExecutionController::GetState() const {
	return (int)execState;
}

bool InternalExecutionController::SetPath(const wstring &p) {
	if (execState == RUNNING) {
		return false;
	}

	path = p;
	execState = INITIALIZED;
	return true;
}

bool InternalExecutionController::SetCmdLine(const wstring &c) {
	if (execState == RUNNING) {
		return false;
	}

	cmdLine = c;
	return true;
}

bool InternalExecutionController::SetExecutionFeatures(unsigned int feat) {
	featureFlags = feat;
	return true;
}

bool InternalExecutionController::InitializeAllocator() {
	SYSTEM_INFO si;
	memset(&si, 0, sizeof(si));

	GetSystemInfo(&si);

	shmAlloc = new DualAllocator(1 << 30, hProcess, L"Local\\MumuMem", si.dwAllocationGranularity);
	return NULL != shmAlloc;
}

bool InternalExecutionController::MapLoader() {
	FloatingPE *fLoader = new FloatingPE("loader.dll");
	ExternMapper mLoader(hProcess);
	DWORD dwWritten;
	bool bRet = false;
	do {

		pLoaderBase = (BYTE *)0x00500000;
		if (!fLoader->IsValid()) {
			break;
		}

		if (!fLoader->MapPE(mLoader, (DWORD &)pLoaderBase)) {
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
			break;
		}

		if (FALSE == WriteProcessMemory(hProcess, ldrAPIPtr, &ldrAPI, sizeof(ldrAPI), &dwWritten)) {
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

bool InternalExecutionController::InitializeIpcLib(FloatingPE *fIpcLib) {
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
		!LoadExportedName(fIpcLib, pIpcBase, "ExecutionBeginFunc", tmpRevApi.executionBegin) ||
		!LoadExportedName(fIpcLib, pIpcBase, "ExecutionControlFunc", tmpRevApi.executionControl) ||
		!LoadExportedName(fIpcLib, pIpcBase, "ExecutionEndFunc", tmpRevApi.executionEnd) ||
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

bool InternalExecutionController::InitializeRevtracer(FloatingPE *fRevTracer) {
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

	rev::RevtracerAPI *revAPI;
	if (!LoadExportedName(fRevTracer, pRevtracerBase, "revtracerAPI", revAPI) ||
		!LoadExportedName(fRevTracer, pRevtracerBase, "revtracerConfig", revCfg)
	) {
		return false;
	}
	memcpy(revAPI, &tmpRevApi, sizeof(tmpRevApi));

	revCfg->contextSize = 0;
	InitSegments(hMainThread, revCfg->segmentOffsets);
	revCfg->hookCount = 0;
	revCfg->featureFlags = featureFlags;

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

bool InternalExecutionController::MapTracer() {
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

		printf("ipclib@0x%08x\nrevtracer@0x%08x\n", pIpcBase, pRevtracerBase);
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

bool InternalExecutionController::WriteLoaderConfig() {
	DWORD dwWritten;
	if (FALSE == WriteProcessMemory(hProcess, pLoaderConfig, &loaderConfig, sizeof(loaderConfig), &dwWritten)) {
		return false;
	}

	return true;
}

bool InternalExecutionController::SwitchEntryPoint() {
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

bool InternalExecutionController::PatchProcess() {
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

	unsigned short tmp = '\1\1';
	DWORD dwWr, oldPr;

	if (FALSE == VirtualProtectEx(hProcess, mAddr, sizeof(tmp), PAGE_READWRITE, &oldPr)) {
		return false;
	}

	if (FALSE == WriteProcessMemory(hProcess, mAddr, &tmp, sizeof(tmp), &dwWr)) {
		return false;
	}

	revCfg->mainModule = mAddr;

	if (FALSE == VirtualProtectEx(hProcess, mAddr, sizeof(tmp), oldPr, &oldPr)) {
		return false;
	}
	return true;
}

DWORD WINAPI ControlThreadFunc(void *ptr) {
	InternalExecutionController *ctr = (InternalExecutionController *)ptr;
	return ctr->ControlThread();
}

void *InternalExecutionController::GetProcessHandle() {
	return hProcess;
}

bool InternalExecutionController::Execute() {
	STARTUPINFOW startupInfo;
	PROCESS_INFORMATION processInfo;

	memset(&startupInfo, 0, sizeof(startupInfo));
	startupInfo.cb = sizeof(startupInfo);
	
	memset(&processInfo, 0, sizeof(processInfo));

	if ((execState != INITIALIZED) && (execState != TERMINATED) && (execState != ERR)) {
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
		return false;
	}

	hProcess = processInfo.hProcess;
	hMainThread = processInfo.hThread;

	if (!InitializeAllocator()) {
		execState = ERR;
		return false;
	}

	if (!MapLoader()) {
		execState = ERR;
		return false;
	}

	if (!MapTracer()) {
		execState = ERR;
		return false;
	}

	if (!WriteLoaderConfig()) {
		execState = ERR;
		return false;
	}

	if (!SwitchEntryPoint()) {
		execState = ERR;
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

bool InternalExecutionController::Terminate() {
	TerminateProcess(hProcess, 0x00000000);

	CloseHandle(hProcess);
	CloseHandle(hMainThread);

	execState = TERMINATED;

	return true;
}

DWORD InternalExecutionController::ControlThread() {
	bool bRunning = true;
	DWORD exitCode;

	HANDLE hDbg = 0, hOffs = 0;

	do {

		hDbg = CreateFileA(
			"debug.log",
			GENERIC_WRITE,
			FILE_SHARE_READ,
			NULL,
			CREATE_ALWAYS,
			0,
			NULL
		);

		if (INVALID_HANDLE_VALUE == hDbg) {
			//TerminateProcess(hProcess, 0);
			break;
		}

		hOffs = CreateFileA(
			"bbs1.txt",
			GENERIC_WRITE,
			FILE_SHARE_READ,
			NULL,
			CREATE_ALWAYS,
			0,
			NULL
		);

		if (INVALID_HANDLE_VALUE == hOffs) {
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

				WriteFile(hDbg, debugBuffer, read, &written, NULL);
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
			case REPLY_EXECUTION_CONTORL:
			case REPLY_SYSCALL_CONTROL:
				__asm int 3;
				break;

			case REQUEST_MEMORY_ALLOC: {
				DWORD offset;
				ipcData->type = REPLY_MEMORY_ALLOC;
				ipcData->data.asMemoryAllocReply.pointer = shmAlloc->Allocate(ipcData->data.asMemoryAllocRequest, offset);
				ipcData->data.asMemoryAllocReply.offset = offset;
				break;
			}

			case REQUEST_EXECUTION_BEGIN: {
				ipc::ADDR_TYPE next = ipcData->data.asExecutionBeginRequest.nextInstruction;

				ipcData->type = REPLY_EXECUTION_BEGIN;
				if (!PatchProcess()) {
					ipcData->data.asExecutionBeginReply = EXECUTION_TERMINATE;
					bRunning = false;
					execState = ERR;
				}
				else {
					execState = SUSPENDED_AT_START;
					if (EXECUTION_TERMINATE == (ipcData->data.asExecutionBeginReply = eBegin(context, (unsigned int)next))) {
						bRunning = false;
					}
				}
				break;
			}

			case REQUEST_EXECUTION_CONTORL: {
				ipc::ADDR_TYPE next = ipcData->data.asExecutionControlRequest.nextInstruction;

				char mf[MAX_PATH];
				char defMf[8] = "??";

				/*if (((DWORD)next & 0xFFFD0000) == baseAddr) {
				strcpy(defMf, "a.exe");
				}*/

				DWORD dwSz = GetMappedFileNameA(hProcess, next, mf, sizeof(mf)-1);

				char *module = defMf;
				for (DWORD i = 1; i < dwSz; ++i) {
					if (mf[i - 1] == '\\') {
						module = &mf[i];
					}
				}

				for (char *t = module; *t != '\0'; ++t) {
					*t = toupper(*t);
				}

				fprintf(fOffs, "%s + 0x%04X\n", module, (DWORD)next & 0xFFFF);
				/*fprintf(
					fOffs,
					"%-15s+%04X EAX:%08x ECX:%08x EDX:%08x EBX:%08x ESP:%08x EBP:%08x ESI:%08x EDI:%08x\r\n",
					module,
					(DWORD)next & 0xFFFF,

					);*/

				fflush(fOffs);

				ipcData->type = REPLY_EXECUTION_CONTORL;
				execState = SUSPENDED;
				if (EXECUTION_TERMINATE == (ipcData->data.asExecutionControlReply = eControl(context, (unsigned int)next))) {
					bRunning = false;

				}
				break;
			}

			case REQUEST_EXECUTION_END:
				ipcData->type = REPLY_EXECUTION_END;
				execState = SUSPENDED_AT_TERMINATION;
				if (EXECUTION_TERMINATE == (ipcData->data.asExecutionEndReply = eEnd(context))) {
					bRunning = false;
				}
				break;

			case REQUEST_SYSCALL_CONTROL:
				ipcData->type = REPLY_SYSCALL_CONTROL;
				break;

			default:
				__asm int 3;
				break;
			}

			ipcToken->Release(REMOTE_TOKEN_USER);
		}

	} while (false);

	CloseHandle(hDbg);
	CloseHandle(hOffs);

	WaitForSingleObject(hProcess, INFINITE);

	CloseHandle(hMainThread);
	CloseHandle(hProcess);

	execState = TERMINATED;
	term(context);

	delete shmAlloc;

	return 0;
}

struct _VM_COUNTERS_ {
	ULONG PeakVirtualSize;
	ULONG VirtualSize; // << VM use 
	ULONG PageFaultCount;
	ULONG PeakWorkingSetSize;
	ULONG WorkingSetSize;
	ULONG QuotaPeakPagedPoolUsage;
	ULONG QuotaPagedPoolUsage;
	ULONG QuotaPeakNonPagedPoolUsage;
	ULONG QuotaNonPagedPoolUsage;
	ULONG PagefileUsage; // << Commited use
	ULONG PeakPagefileUsage;
};

/*_declspec(dllimport) extern "C" NTSTATUS WINAPI NtQueryInformationProcess(
	_In_      HANDLE             ProcessHandle,
	_In_      MYPROCESSINFOCLASS ProcessInformationClass,
	_Out_     PVOID              ProcessInformation,
	_In_      ULONG              ProcessInformationLength,
	_Out_opt_ PULONG             ReturnLength
);*/

bool InternalExecutionController::UpdateLayout() {
	if (updated) {
		return true;
	}

	ULONG retLen;
	_VM_COUNTERS_ ctrs;

	NtQueryInformationProcess(
		hProcess,
		(PROCESSINFOCLASS) 3,
		&ctrs,
		sizeof(ctrs),
		&retLen
	);

	if ((ctrs.VirtualSize == virtualSize) && (ctrs.PagefileUsage == commitedSize)) {
		return true;
	}

	virtualSize = ctrs.VirtualSize;
	commitedSize = ctrs.PagefileUsage;

	sec.clear();
	mod.clear();

	wchar_t mf[MAX_PATH];

	for (DWORD dwAddr = 0; dwAddr < 0x7FFF0000;) {
		MEMORY_BASIC_INFORMATION32 mbi;

		if (0 == VirtualQueryEx(hProcess, (LPCVOID)dwAddr, (PMEMORY_BASIC_INFORMATION)&mbi, sizeof(mbi))) {
			return false;
		}

		VirtualMemorySection vms;
		vms.BaseAddress = dwAddr;
		vms.Protection = mbi.Protect;
		vms.RegionSize = mbi.RegionSize;
		vms.State = mbi.State;
		vms.Type = mbi.Type;
		sec.push_back(vms);

		DWORD dwSz = GetMappedFileNameW(hProcess, (LPVOID)dwAddr, mf, (sizeof(mf) / sizeof(mf[0]))-1);
		if (0 != dwSz) {
			wchar_t *module = mf;
			for (DWORD i = 1; i < dwSz; ++i) {
				if (mf[i - 1] == '\\') {
					module = &mf[i];
				}
			}

			for (wchar_t *t = module; *t != L'\0'; ++t) {
				*t = tolower(*t);
			}

			if ((0 == mod.size()) || (0 != wcscmp(module, mod[mod.size() - 1].Name))) {
				ModuleInfo mi;
				wcscpy_s(mi.Name, module);
				mi.ModuleBase = dwAddr;
				mi.Size = mbi.RegionSize;

				mod.push_back(mi);
			}
			else {
				ModuleInfo &mi = mod[mod.size() - 1];

				mi.Size = dwAddr - mi.ModuleBase + mbi.RegionSize;
			}
		}


		dwAddr = mbi.BaseAddress + mbi.RegionSize;
	}

	updated = true;
	return true;
}

bool InternalExecutionController::GetProcessVirtualMemory(VirtualMemorySection *&sections, int &sectionCount) {
	if (!UpdateLayout()) {
		return false;
	}

	sectionCount = sec.size();
	sections = &sec[0];

	return true;
}

bool InternalExecutionController::GetModules(ModuleInfo *&modules, int &moduleCount) {
	if (!UpdateLayout()) {
		return false;
	}

	moduleCount = mod.size();
	modules = &mod[0];
	return true;
}

bool InternalExecutionController::ReadProcessMemory(unsigned int base, unsigned int size, unsigned char *buff) {
	DWORD dwRd;
	return TRUE == ::ReadProcessMemory(hProcess, (LPCVOID)base, buff, size, &dwRd);
}

void InternalExecutionController::SetNotificationContext(void *ctx) {
	context = ctx;
}

void InternalExecutionController::SetTerminationNotification(TerminationNotifyFunc func) {
	term = func;
}

void InternalExecutionController::SetExecutionBeginNotification(ExecutionBeginFunc func) {
	eBegin = func;
}

void InternalExecutionController::SetExecutionControlNotification(ExecutionControlFunc func) {
	eControl = func;
}

void InternalExecutionController::SetExecutionEndNotification(ExecutionEndFunc func) {
	eEnd = func;
}

void InternalExecutionController::GetCurrentRegisters(Registers &registers) {
	RemoteRuntime *ree = (RemoteRuntime *)revCfg->pRuntime;

	memcpy(&registers, (Registers *)ree->registers, sizeof(registers));
	registers.esp = ree->virtualStack;
}
