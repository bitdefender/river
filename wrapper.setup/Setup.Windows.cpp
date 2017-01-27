#ifndef __linux__

#include "../CommonCrossPlatform/Common.h"
#include "../revtracer-wrapper/RevtracerWrapper.h"

extern "C" bool InitFunctionOffsets(revwrapper::LibraryLayout *libs, revwrapper::ImportedApi *api) {

	LIB_T hNtDll = GET_LIB_HANDLER(L"ntdll.dll");
	DWORD baseNtDll = GET_LIB_BASE(hNtDll);

	if (!hNtDll) {
		return false;
	}

	libs->windows.ntdllBase = baseNtDll;

	api->functions.windows.ntdll._virtualAlloc = (DWORD)LOAD_PROC(hNtDll, "NtAllocateVirtualMemory") - baseNtDll;
	api->functions.windows.ntdll._virtualFree = (DWORD)LOAD_PROC(hNtDll, "NtFreeVirtualMemory") - baseNtDll;
	api->functions.windows.ntdll._mapMemory = (DWORD)LOAD_PROC(hNtDll, "NtMapViewOfSection") - baseNtDll;
	api->functions.windows.ntdll._flushMemoryCache = (DWORD)LOAD_PROC(hNtDll, "RtlFlushSecureMemoryCache") - baseNtDll;
	api->functions.windows.ntdll._terminateProcess = (DWORD)LOAD_PROC(hNtDll, "NtTerminateProcess") - baseNtDll;
	api->functions.windows.ntdll._writeFile = (DWORD)LOAD_PROC(hNtDll, "NtWriteFile") - baseNtDll;
	api->functions.windows.ntdll._waitForSingleObject = (DWORD)LOAD_PROC(hNtDll, "NtWaitForSingleObject") - baseNtDll;
	api->functions.windows.ntdll._systemError = (DWORD)LOAD_PROC(hNtDll, "RtlNtStatusToDosError") - baseNtDll;
	api->functions.windows.ntdll._formatPrint = (DWORD)LOAD_PROC(hNtDll, "_vsnprintf_s") - baseNtDll;
	api->functions.windows.ntdll._ntYieldExecution = (DWORD)LOAD_PROC(hNtDll, "NtYieldExecution") - baseNtDll;
	api->functions.windows.ntdll._flushInstructionCache = (DWORD)LOAD_PROC(hNtDll, "NtFlushInstructionCache") - baseNtDll;
	api->functions.windows.ntdll._createEvent = (DWORD)LOAD_PROC(hNtDll, "NtCreateEvent") - baseNtDll;
	api->functions.windows.ntdll._setEvent = (DWORD)LOAD_PROC(hNtDll, "NtSetEvent") - baseNtDll;

	return true;
}


#endif

