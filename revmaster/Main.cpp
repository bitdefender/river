#include <Windows.h>
#include <Psapi.h>
#include <string.h>
#include <io.h>
#include <fcntl.h>
#include <tlhelp32.h>


#include "DualAllocator.h"

#include "Loader/Extern.Mapper.h"
#include "Loader/Mem.Mapper.h"
#include "Loader/PE.ldr.h"

#include "../loader/loader.h"
#include "../ipclib/ipclib.h"
#include "../revtracer/revtracer.h"

typedef long NTSTATUS;
typedef NTSTATUS(*NtYieldExecutionFunc)();

NtYieldExecutionFunc ntdllYieldExecution;

namespace ipc {
	void NtDllNtYieldExecution() {
		::ntdllYieldExecution();
	}
};

void InitSegment(HANDLE hThread, DWORD dwSeg, DWORD &offset) {
	LDT_ENTRY entry;
	DWORD base, limit;

	if (FALSE == GetThreadSelectorEntry(hThread, dwSeg, &entry)) {
		base = 0xFFFFFFFF;
		limit = 0;
	} else {
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


DualAllocator *shmAlloc = NULL;

FloatingPE *fLoader = NULL, *fIpcLib = NULL, *fRevTracer = NULL;

DWORD CharacteristicsToDesiredAccess(DWORD c) {
	DWORD r = 0;

	r |= (c & IMAGE_SCN_MEM_READ) ? FILE_MAP_READ : 0;
	r |= (c & IMAGE_SCN_MEM_WRITE) ? FILE_MAP_WRITE : 0;
	r |= (c & IMAGE_SCN_MEM_EXECUTE) ? FILE_MAP_EXECUTE : 0;
	return r;
}

char debugBuffer[4096];

bool FixProcessorFeature(HANDLE hProcess, DWORD newAddr) {
	HMODULE hKernel32 = GetModuleHandleA("kernel32.dll");
	unsigned char *ipfpAddr = (unsigned char *)GetProcAddress(hKernel32, "IsProcessorFeaturePresent");

	DWORD ipfpOffset = ipfpAddr - (unsigned char *)hKernel32;

	DWORD dwOffset = 0x10000;
	while (0x7FFE0000 >= dwOffset) {
		MEMORY_BASIC_INFORMATION32 mbi;
		char moduleName[MAX_PATH];

		VirtualQueryEx(hProcess, (PVOID)dwOffset, (PMEMORY_BASIC_INFORMATION)&mbi, sizeof(mbi));

		if ((MEM_COMMIT == mbi.State) && (SEC_IMAGE == mbi.Type) && (PAGE_READONLY == mbi.Protect)) {
			if (0 != GetMappedFileNameA(
				hProcess,
				(PVOID)dwOffset,
				moduleName,
				sizeof(moduleName)-1
			)) {
				int pos = strlen(moduleName) - 12;

				if ((pos >= 0) && (0 == strcmp(&moduleName[pos], "kernel32.dll"))) {
					break;
				}
			}
		}

		dwOffset += mbi.RegionSize;
	}

	if (0x7FFE0000 < dwOffset) {
		return false;
	}

	// IPFP is at dwOffset + ipfpOffset
	// next instruction is at dwOffset + ipfpOffset + 5
	// relative jmp is newAddr - dwOffset + ipfpOffset + 5
	unsigned char buf[5];
	DWORD dwJmp = newAddr - (dwOffset + ipfpOffset + 5);
	buf[0] = 0xE9; // relative jump
	*(DWORD *)&buf[1] = dwJmp;
	DWORD written;

	if (FALSE == WriteProcessMemory(hProcess, (PVOID)(dwOffset + ipfpOffset), buf, sizeof(buf), &written)) {
		return false;
	}

	return true;
}

bool UnmapMainModule(char *modName, DWORD pid, HANDLE hProcess) {
	HANDLE hModuleSnap = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE, pid);
	BYTE *mAddr = NULL;

	do {
		MODULEENTRY32 me32;

		// Take a snapshot of all modules in the specified process.
		if (hModuleSnap == INVALID_HANDLE_VALUE)
		{
			break;
		}

		// Set the size of the structure before using it.
		me32.dwSize = sizeof(MODULEENTRY32);

		// Retrieve information about the first module,
		// and exit if unsuccessful
		if (!Module32First(hModuleSnap, &me32))
		{
			CloseHandle(hModuleSnap);           // clean the snapshot object
			break;
		}

		// Now walk the module list of the process,
		// and display information about each module
		do
		{
			if (0 == _stricmp(modName, me32.szModule)) {
				mAddr = me32.modBaseAddr;
				break;
			}

		} while (Module32Next(hModuleSnap, &me32));
	} while (0);

	if (NULL != mAddr) {
		unsigned short tmp = 0;
		DWORD dwWr, oldPr;

		if (FALSE == VirtualProtectEx(hProcess, mAddr, sizeof(tmp), PAGE_READWRITE, &oldPr)) {
			return false;
		}

		if (FALSE == WriteProcessMemory(hProcess, mAddr, &tmp, sizeof(tmp), &dwWr)) {
			return false;
		}

		if (FALSE == VirtualProtectEx(hProcess, mAddr, sizeof(tmp), oldPr, &oldPr)) {
			return false;
		}

		return true;
	}

	return false;
}

int main() {
	STARTUPINFOW startup;
	PROCESS_INFORMATION pInfo;

	memset(&startup, 0, sizeof(startup));
	startup.cb = sizeof(startup);

	BOOL bRet = CreateProcessW(
		L"..\\lzo\\a.exe",
		L"",
		NULL,
		NULL,
		FALSE,
		CREATE_SUSPENDED,
		NULL,
		NULL,
		&startup,
		&pInfo
	);

	if (FALSE == bRet) {
		printf("CreateProcess() error %d\n", GetLastError());
		return 0;
	}

	//ldr::LoaderConfig ldrConfig;
	//ldrConfig.sharedMemoryName


	SYSTEM_INFO si;
	memset(&si, 0, sizeof(si));

	GetSystemInfo(&si);

	shmAlloc = new DualAllocator(1 << 30, pInfo.hProcess, L"Local\\MumuMem", si.dwAllocationGranularity);


	fLoader = new FloatingPE("loader.dll");
	ExternMapper mLoader(pInfo.hProcess);

	DWORD dwLoaderBase = 0x500000;
	if (!fLoader->MapPE(mLoader, dwLoaderBase)) {
		TerminateProcess(pInfo.hProcess, 0);
		return 0;
	}

	FlushInstructionCache(pInfo.hProcess, (LPVOID)dwLoaderBase, fLoader->GetRequiredSize());

	fIpcLib = new FloatingPE("ipclib.dll");
	fRevTracer = new FloatingPE("revtracer.dll");
	DWORD dwIpcLibSize = (fIpcLib->GetRequiredSize() + 0xFFFF) & ~0xFFFF;
	DWORD dwRevTracerSize = (fRevTracer->GetRequiredSize() + 0xFFFF) & ~0xFFFF;
	DWORD dwTotalSize = dwIpcLibSize + dwRevTracerSize;
	DWORD dwOffset, dwWritten;
	
	BYTE *libs = (BYTE *)shmAlloc->Allocate(dwTotalSize, dwOffset);
	DWORD ipcLibAddr = (DWORD)libs, revTracerAddr = (DWORD)libs + dwIpcLibSize;
	DWORD ipcLibOffset = dwOffset, revTracerOffset = dwOffset + dwIpcLibSize;
	MemMapper mIpcLib, mRevTracer;

	if (!fIpcLib->MapPE(mIpcLib, ipcLibAddr) || !fRevTracer->MapPE(mRevTracer, revTracerAddr)) {
		TerminateProcess(pInfo.hProcess, 0);
		return 0;
	}

	FlushInstructionCache(pInfo.hProcess, (LPVOID)libs, dwTotalSize);

	// TODO: write loaderAPI
	HMODULE hNtDll = GetModuleHandleW(L"ntdll.dll");
	ldr::LoaderAPI ldrAPI;

//	ldrAPI.ntOpenSection = GetProcAddress(hNtDll, "NtOpenSection");
	ldrAPI.ntMapViewOfSection = GetProcAddress(hNtDll, "NtMapViewOfSection");

//	ldrAPI.ntOpenDirectoryObject = GetProcAddress(hNtDll, "NtOpenDirectoryObject");
//	ldrAPI.ntClose = GetProcAddress(hNtDll, "NtClose");

	ldrAPI.ntFlushInstructionCache = GetProcAddress(hNtDll, "NtFlushInstructionCache");

//	ldrAPI.rtlInitUnicodeStringEx = GetProcAddress(hNtDll, "RtlInitUnicodeStringEx");
//	ldrAPI.rtlFreeUnicodeString = GetProcAddress(hNtDll, "RtlFreeUnicodeString");

	DWORD ldrAPIOffset;
	if (!fLoader->GetExport("loaderAPI", ldrAPIOffset)) {
		TerminateProcess(pInfo.hProcess, 0);
		return 0;
	}

	if (FALSE == WriteProcessMemory(pInfo.hProcess, (ldr::ADDR_TYPE)(dwLoaderBase + ldrAPIOffset), &ldrAPI, sizeof(ldrAPI), &dwWritten)) {
		TerminateProcess(pInfo.hProcess, 0);
		return 0;
	}

	ldr::LoaderConfig ldrCfg;
	DWORD dwPerform;

	if (!fRevTracer->GetExport("RevtracerPerform", dwPerform)) {
		TerminateProcess(pInfo.hProcess, 0);
		return 0;
	}

	memset(&ldrCfg, 0, sizeof(ldrCfg));
	ldrCfg.entryPoint = (ldr::ADDR_TYPE)(revTracerAddr + dwPerform); // TODO, populate later
	//ldrCfg.mappingAddress = 0; // TODO, populate later

	DWORD dwIpcLibSections = fIpcLib->GetSectionCount();
	DWORD sCount = 0;
	for (DWORD i = 0; i < dwIpcLibSections; ++i) {
		const PESection *sec = fIpcLib->GetSection(i);

		if (sec->header.Characteristics & IMAGE_SCN_MEM_DISCARDABLE) {
			continue;
		}

		ldrCfg.sections[sCount].mappingAddress = (ldr::ADDR_TYPE)(ipcLibAddr + sec->header.VirtualAddress);
		ldrCfg.sections[sCount].mappingSize = (sec->header.VirtualSize + 0xFFF) & ~0xFFF;
		ldrCfg.sections[sCount].sectionOffset = ipcLibOffset + sec->header.VirtualAddress;
		ldrCfg.sections[sCount].desiredAccess = CharacteristicsToDesiredAccess(sec->header.Characteristics);
		sCount++;
	}

	DWORD dwRevTracerSections = fRevTracer->GetSectionCount();
	for (DWORD i = 0; i < dwRevTracerSections; ++i) {
		const PESection *sec = fRevTracer->GetSection(i);

		if (sec->header.Characteristics & IMAGE_SCN_MEM_DISCARDABLE) {
			continue;
		}

		ldrCfg.sections[sCount].mappingAddress = (ldr::ADDR_TYPE)(revTracerAddr + sec->header.VirtualAddress);
		ldrCfg.sections[sCount].mappingSize = (sec->header.VirtualSize + 0xFFF) & ~0xFFF;
		ldrCfg.sections[sCount].sectionOffset = revTracerOffset + sec->header.VirtualAddress;
		ldrCfg.sections[sCount].desiredAccess = CharacteristicsToDesiredAccess(sec->header.Characteristics);
		sCount++;
	}
	
	ldrCfg.sectionCount = sCount;
	//wcscpy_s(ldrCfg.sharedMemoryName, L"Local\\MumuMem");
	ldrCfg.sharedMemory = NULL;

	DWORD ldrCfgOffset;
	if (!fLoader->GetExport("loaderConfig", ldrCfgOffset)) {
		TerminateProcess(pInfo.hProcess, 0);
		return 0;
	}

	DWORD ldrMyIPFPOffset;
	if (!fLoader->GetExport("MyIsProcessorFeaturePresent", ldrMyIPFPOffset)) {
		TerminateProcess(pInfo.hProcess, 0);
		return 0;
	}

	if (FALSE == WriteProcessMemory(pInfo.hProcess, (ldr::ADDR_TYPE)(dwLoaderBase + ldrCfgOffset), &ldrCfg, sizeof(ldrCfg), &dwWritten)) {
		TerminateProcess(pInfo.hProcess, 0);
		return 0;
	}

	DWORD ipcDebugLogOffset;
	ipc::RingBuffer<(1 << 20)> *debugLog;
	if (!fIpcLib->GetExport("debugLog", ipcDebugLogOffset)) {
		TerminateProcess(pInfo.hProcess, 0);
		return 0;
	}
	debugLog = (ipc::RingBuffer<(1 << 20)> *)(ipcLibAddr + ipcDebugLogOffset);

	DWORD ipcTokenOffset;
	ipc::ShmTokenRing *ipcToken;
	if (!fIpcLib->GetExport("ipcToken", ipcTokenOffset)) {
		TerminateProcess(pInfo.hProcess, 0);
		return 0;
	}
	ipcToken = (ipc::ShmTokenRing *)(ipcLibAddr + ipcTokenOffset);

	DWORD ipcAPIOffset;
	if (!fIpcLib->GetExport("ipcAPI", ipcAPIOffset)) {
		TerminateProcess(pInfo.hProcess, 0);
		return 0;
	}
	ipc::IpcAPI *ipcAPI = (ipc::IpcAPI *)(ipcLibAddr + ipcAPIOffset);

	DWORD ipcDataOffset;
	if (!fIpcLib->GetExport("ipcData", ipcDataOffset)) {
		TerminateProcess(pInfo.hProcess, 0);
		return 0;
	}
	ipc::IpcData *ipcData = (ipc::IpcData *)(ipcLibAddr + ipcDataOffset);
	
	//ipcAPI.ntSetEvent = GetProcAddress(hNtDll, "NtSetEvent");
	//ipcAPI.ntWaitForSingleObject = GetProcAddress(hNtDll, "NtWaitForSingleObject");
	ipcAPI->ntYieldExecution = GetProcAddress(hNtDll, "NtYieldExecution");
	ntdllYieldExecution = (NtYieldExecutionFunc)ipcAPI->ntYieldExecution;
	ipcAPI->vsnprintf_s = GetProcAddress(hNtDll, "_vsnprintf_s");

	DWORD dwMapMemoryOffset;
	if (!fLoader->GetExport("MapMemory", dwMapMemoryOffset)) {
		TerminateProcess(pInfo.hProcess, 0);
		return 0;
	}
	ipcAPI->ldrMapMemory = (ipc::ADDR_TYPE)(dwLoaderBase + dwMapMemoryOffset);


	DWORD revAPIOffset;
	if (!fRevTracer->GetExport("revtracerAPI", revAPIOffset)) {
		TerminateProcess(pInfo.hProcess, 0);
		return 0;
	}

	rev::RevtracerAPI *revAPI = (rev::RevtracerAPI *)(revTracerAddr + revAPIOffset);
	
	DWORD dwIpcFuncOffset;
	if (!fIpcLib->GetExport("Initialize", dwIpcFuncOffset)) {
		TerminateProcess(pInfo.hProcess, 0);
		return 0;
	}
	revAPI->ipcLibInitialize = (rev::IpcLibInitFunc)(ipcLibAddr + dwIpcFuncOffset);

	if (!fIpcLib->GetExport("DebugPrint", dwIpcFuncOffset)) {
		TerminateProcess(pInfo.hProcess, 0);
		return 0;
	}
	revAPI->dbgPrintFunc = (rev::DbgPrintFunc)(ipcLibAddr + dwIpcFuncOffset);

	if (!fIpcLib->GetExport("MemoryAllocFunc", dwIpcFuncOffset)) {
		TerminateProcess(pInfo.hProcess, 0);
		return 0;
	}
	revAPI->memoryAllocFunc = (rev::MemoryAllocFunc)(ipcLibAddr + dwIpcFuncOffset);

	if (!fIpcLib->GetExport("MemoryFreeFunc", dwIpcFuncOffset)) {
		TerminateProcess(pInfo.hProcess, 0);
		return 0;
	}
	revAPI->memoryFreeFunc = (rev::MemoryFreeFunc)(ipcLibAddr + dwIpcFuncOffset);

	if (!fIpcLib->GetExport("TakeSnapshot", dwIpcFuncOffset)) {
		TerminateProcess(pInfo.hProcess, 0);
		return 0;
	}
	revAPI->takeSnapshot = (rev::TakeSnapshotFunc)(ipcLibAddr + dwIpcFuncOffset);
	
	if (!fIpcLib->GetExport("RestoreSnapshot", dwIpcFuncOffset)) {
		TerminateProcess(pInfo.hProcess, 0);
		return 0;
	}
	revAPI->restoreSnapshot = (rev::RestoreSnapshotFunc)(ipcLibAddr + dwIpcFuncOffset);
	
	if (!fIpcLib->GetExport("InitializeContextFunc", dwIpcFuncOffset)) {
		TerminateProcess(pInfo.hProcess, 0);
		return 0;
	}
	revAPI->initializeContext = (rev::InitializeContextFunc)(ipcLibAddr + dwIpcFuncOffset);
	
	if (!fIpcLib->GetExport("CleanupContextFunc", dwIpcFuncOffset)) {
		TerminateProcess(pInfo.hProcess, 0);
		return 0;
	}
	revAPI->cleanupContext = (rev::CleanupContextFunc)(ipcLibAddr + dwIpcFuncOffset);

	/*if (!fIpcLib->GetExport("ExecutionBeginFunc", dwIpcFuncOffset)) {
		TerminateProcess(pInfo.hProcess, 0);
		return 0;
	}
	revAPI->executionBegin = (rev::ExecutionBeginFunc)(ipcLibAddr + dwIpcFuncOffset);

	if (!fIpcLib->GetExport("ExecutionControlFunc", dwIpcFuncOffset)) {
		TerminateProcess(pInfo.hProcess, 0);
		return 0;
	}
	revAPI->executionControl = (rev::ExecutionControlFunc)(ipcLibAddr + dwIpcFuncOffset);

	if (!fIpcLib->GetExport("ExecutionEndFunc", dwIpcFuncOffset)) {
		TerminateProcess(pInfo.hProcess, 0);
		return 0;
	}
	revAPI->executionEnd = (rev::ExecutionEndFunc)(ipcLibAddr + dwIpcFuncOffset);*/

	if (!fIpcLib->GetExport("SyscallControlFunc", dwIpcFuncOffset)) {
		TerminateProcess(pInfo.hProcess, 0);
		return 0;
	}
	revAPI->syscallControl = (rev::SyscallControlFunc)(ipcLibAddr + dwIpcFuncOffset);

	revAPI->lowLevel.ntAllocateVirtualMemory = GetProcAddress(hNtDll, "NtAllocateVirtualMemory");
	revAPI->lowLevel.ntFreeVirtualMemory = GetProcAddress(hNtDll, "NtFreeVirtualMemory");

	revAPI->lowLevel.ntQueryInformationThread = GetProcAddress(hNtDll, "NtQueryInformationThread");
	revAPI->lowLevel.ntTerminateProcess = GetProcAddress(hNtDll, "NtTerminateProcess");

	revAPI->lowLevel.rtlNtStatusToDosError = GetProcAddress(hNtDll, "RtlNtStatusToDosError");
	revAPI->lowLevel.vsnprintf_s = GetProcAddress(hNtDll, "_vsnprintf_s");

	CONTEXT ctx;
	memset(&ctx, 0, sizeof(ctx));
	ctx.ContextFlags = CONTEXT_ALL;
	if (FALSE == GetThreadContext(pInfo.hThread, &ctx)) {
		TerminateProcess(pInfo.hProcess, 0);
		return 0;
	}

	DWORD revCfgOffset;
	if (!fRevTracer->GetExport("revtracerConfig", revCfgOffset)) {
		TerminateProcess(pInfo.hProcess, 0);
		return 0;
	}
	rev::RevtracerConfig *revCfg = (rev::RevtracerConfig *)(revTracerAddr + revCfgOffset);

	revCfg->context = nullptr;
	revCfg->entryPoint = (rev::ADDR_TYPE)ctx.Eip;
	InitSegments(pInfo.hThread, revCfg->segmentOffsets);

	revCfg->hookCount = 0;

	DWORD ldrPerform;
	if (!fLoader->GetExport("LoaderPerform", ldrPerform)) {
		TerminateProcess(pInfo.hProcess, 0);
		return 0;
	}

	ctx.Eip = dwLoaderBase + ldrPerform;
	SetThreadContext(pInfo.hThread, &ctx);

	DWORD exitCode;
	HANDLE hDbg = CreateFileA(
		"debug.log",
		GENERIC_WRITE,
		FILE_SHARE_READ,
		NULL,
		CREATE_ALWAYS,
		0,
		NULL
	);

	if (INVALID_HANDLE_VALUE == hDbg) {
		TerminateProcess(pInfo.hProcess, 0);
		return 0;
	}
	
	HANDLE hOffs = CreateFileA(
		"bbs1.txt",
		GENERIC_WRITE,
		FILE_SHARE_READ,
		NULL,
		CREATE_ALWAYS,
		0,
		NULL
	);

	if (INVALID_HANDLE_VALUE == hOffs) {
		TerminateProcess(pInfo.hProcess, 0);
		return 0;
	}

	int cFile = _open_osfhandle((intptr_t)hOffs, _O_TEXT);
	FILE *fOffs = _fdopen(cFile, "wt");

	ResumeThread(pInfo.hThread); 
	
	bool bRunning = true;
	while (bRunning) {
		GetExitCodeProcess(pInfo.hProcess, &exitCode);

		if (STILL_ACTIVE != exitCode) {
			break;
		}

		ipcToken->Wait(REMOTE_TOKEN_USER);

		while (!debugLog->IsEmpty()) {
			int read;
			DWORD written;
			debugLog->Read(debugBuffer, sizeof(debugBuffer)-1, read);

			WriteFile(hDbg, debugBuffer, read, &written, NULL);
		}

		

		switch (ipcData->type) {
			case REPLY_MEMORY_ALLOC :
			case REPLY_MEMORY_FREE :
			case REPLY_TAKE_SNAPSHOT :
			case REPLY_RESTORE_SNAPSHOT :
			case REPLY_INITIALIZE_CONTEXT :
			case REPLY_CLEANUP_CONTEXT :
			//case REPLY_EXECUTION_CONTORL :
			case REPLY_SYSCALL_CONTROL :
				__asm int 3;
				break;
			
			case REQUEST_MEMORY_ALLOC: {
				DWORD offset;
				ipcData->type = REPLY_MEMORY_ALLOC;
				ipcData->data.asMemoryAllocReply.pointer = shmAlloc->Allocate(ipcData->data.asMemoryAllocRequest, offset);
				ipcData->data.asMemoryAllocReply.offset = offset;
				break;
			}

			/*case REQUEST_EXECUTION_BEGIN: {
				ipc::ADDR_TYPE next = ipcData->data.asExecutionBeginRequest.nextInstruction;
				
				
				ipcData->type = REPLY_EXECUTION_BEGIN;
				ipcData->data.asExecutionBeginReply = EXECUTION_ADVANCE;
				if (!FixProcessorFeature(pInfo.hProcess, dwLoaderBase + ldrMyIPFPOffset) || !UnmapMainModule("a.exe", pInfo.dwProcessId, pInfo.hProcess)) {
					ipcData->data.asExecutionBeginReply = EXECUTION_TERMINATE;
				}
				break;
			}

			case REQUEST_EXECUTION_CONTORL: {
				ipc::ADDR_TYPE next = ipcData->data.asExecutionControlRequest.nextInstruction;

				char mf[MAX_PATH];
				char defMf[8] = "??";

				DWORD dwSz = GetMappedFileNameA(GetCurrentProcess(), next, mf, sizeof(mf)-1);

				char *module = defMf;
				for (unsigned int i = 1; i < dwSz; ++i) {
					if (mf[i - 1] == '\\') {
						module = &mf[i];
					}
				}

				for (char *t = module; *t != '\0'; ++t) {
					*t = toupper(*t);
				}

				fprintf(fOffs, "%s + 0x%04x\n", module, (DWORD)next);
				fflush(fOffs);

				ipcData->type = REPLY_EXECUTION_CONTORL;
				ipcData->data.asExecutionControlReply = EXECUTION_ADVANCE;
				break;
			}

			case REQUEST_EXECUTION_END:
				ipcData->type = REPLY_EXECUTION_END;
				ipcData->data.asExecutionEndReply = EXECUTION_TERMINATE;
				bRunning = false;
				break;*/

			case REQUEST_SYSCALL_CONTROL:
				ipcData->type = REPLY_SYSCALL_CONTROL;
				break;

			default :
				__asm int 3;
				break;
		}

		ipcToken->Release(REMOTE_TOKEN_USER);
	}

	while (!debugLog->IsEmpty()) {
		int read;
		DWORD written;
		debugLog->Read(debugBuffer, sizeof(debugBuffer)-1, read);

		WriteFile(hDbg, debugBuffer, read, &written, NULL);
	}


	CloseHandle(hDbg);
	CloseHandle(hOffs);

	WaitForSingleObject(pInfo.hThread, INFINITE);


	system("pause");

	return 0;
}