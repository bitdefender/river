#include "InternalExecutionController.h"



#include "Loader/Extern.Mapper.h"
#include "Loader/Mem.Mapper.h"

InternalExecutionController::InternalExecutionController() {
	execState = NEW;

	path = L"";
	cmdLine = L"";

	shmAlloc = NULL;
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

bool InternalExecutionController::InitializeAllocator() {
	SYSTEM_INFO si;
	memset(&si, 0, sizeof(si));

	GetSystemInfo(&si);

	shmAlloc = new DualAllocator(1 << 30, hProcess, L"Global\\MumuMem", si.dwAllocationGranularity);
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

		ldrAPI.ntOpenSection = GetProcAddress(hNtDll, "NtOpenSection");
		ldrAPI.ntMapViewOfSection = GetProcAddress(hNtDll, "NtMapViewOfSection");

		ldrAPI.ntOpenDirectoryObject = GetProcAddress(hNtDll, "NtOpenDirectoryObject");
		ldrAPI.ntClose = GetProcAddress(hNtDll, "NtClose");

		ldrAPI.ntFlushInstructionCache = GetProcAddress(hNtDll, "NtFlushInstructionCache");

		ldrAPI.rtlInitUnicodeStringEx = GetProcAddress(hNtDll, "RtlInitUnicodeStringEx");
		ldrAPI.rtlFreeUnicodeString = GetProcAddress(hNtDll, "RtlFreeUnicodeString");

		DWORD ldrAPIOffset;
		if (!fLoader->GetExport("loaderAPI", ldrAPIOffset)) {
			break;
		}

		if (FALSE == WriteProcessMemory(hProcess, pLoaderBase + ldrAPIOffset, &ldrAPI, sizeof(ldrAPI), &dwWritten)) {
			break;
		}

		DWORD ldrCfgOffset;
		if (!fLoader->GetExport("loaderConfig", ldrCfgOffset)) {
			break;
		}
		pLoaderConfig = pLoaderBase + ldrCfgOffset;

		DWORD ldrMapMemoryOffset;
		if (!fLoader->GetExport("MapMemory", ldrMapMemoryOffset)) {
			break;
		}
		pLdrMapMemory = pLoaderBase + ldrMapMemoryOffset;


		memset(&loaderConfig, 0, sizeof(loaderConfig));
		wcscpy_s(loaderConfig.sharedMemoryName, L"Global\\MumuMem");

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

template <typename T> bool LoadExportedName(FloatingPE *fpe, BYTE *base, char *name, T *&ptr) {
	DWORD rva;
	if (!fpe->GetExport(name, rva)) {
		return false;
	}

	ptr = (T *)(base + rva);
	return true;
}

bool InternalExecutionController::InitializeIpcLib(FloatingPE *fIpcLib) {
	/* Imports */
	ipc::IpcAPI *ipcAPI;
	if (!LoadExportedName(fIpcLib, pIpcBase, "ipcAPI", ipcAPI)) {
		return false;
	}

	HMODULE hNtDll = GetModuleHandle("ntdll.dll");
	ipcAPI->ntYieldExecution = GetProcAddress(hNtDll, "NtYieldExecution");
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
		!LoadExportedName(fIpcLib, pIpcBase, "SyscallControlFunc", tmpRevApi.syscallControl)
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

bool InternalExecutionController::Execute() {
	STARTUPINFOW startupInfo;
	PROCESS_INFORMATION processInfo;

	memset(&startupInfo, 0, sizeof(startupInfo));
	startupInfo.cb = sizeof(startupInfo);

	memset(&processInfo, 0, sizeof(processInfo));

	if (execState != INITIALIZED) {
		return false;
	}

	BOOL bRet = CreateProcessW(
		path.c_str(),
		L"", //cmdLine.c_str(),
		NULL,
		NULL,
		FALSE,
		CREATE_SUSPENDED,
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

	return false;
}

bool InternalExecutionController::Terminate() {
	TerminateProcess(hProcess, 0x00000000);

	CloseHandle(hProcess);
	CloseHandle(hMainThread);

	execState = TERMINATED;

	return true;
}

bool InternalExecutionController::GetProcessVirtualMemory(VirtualMemorySection *&sections, int &sectionCount) {
	sec.clear();

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

		dwAddr = mbi.BaseAddress + mbi.RegionSize;
		sec.push_back(vms);
	}

	sectionCount = sec.size();
	sections = &sec[0];

	return true;
}