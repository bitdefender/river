#ifndef __linux__

#include "../CommonCrossPlatform/Common.h"
#include "../revtracer-wrapper/RevtracerWrapper.h"

extern "C" bool InitFunctionOffsets(revwrapper::LibraryLayout *libs, revwrapper::ImportedApi *api) {

	LIB_T hNtDll = GET_LIB_HANDLER(L"ntdll.dll");
	DWORD baseNtDll = GET_LIB_BASE(hNtDll);

	if (!hNtDll) {
		return false;
	}

	libs->winLib.ntdllBase = baseNtDll;

	api->functions.winFunc.ntdll._virtualAlloc = (DWORD)LOAD_PROC(hNtDll, "NtAllocateVirtualMemory") - baseNtDll;
	api->functions.winFunc.ntdll._virtualFree = (DWORD)LOAD_PROC(hNtDll, "NtFreeVirtualMemory") - baseNtDll;
	api->functions.winFunc.ntdll._mapMemory = (DWORD)LOAD_PROC(hNtDll, "NtMapViewOfSection") - baseNtDll;
	api->functions.winFunc.ntdll._flushMemoryCache = (DWORD)LOAD_PROC(hNtDll, "RtlFlushSecureMemoryCache") - baseNtDll;
	api->functions.winFunc.ntdll._terminateProcess = (DWORD)LOAD_PROC(hNtDll, "NtTerminateProcess") - baseNtDll;
	api->functions.winFunc.ntdll._writeFile = (DWORD)LOAD_PROC(hNtDll, "NtWriteFile") - baseNtDll;
	api->functions.winFunc.ntdll._waitForSingleObject = (DWORD)LOAD_PROC(hNtDll, "NtWaitForSingleObject") - baseNtDll;
	api->functions.winFunc.ntdll._systemError = (DWORD)LOAD_PROC(hNtDll, "RtlNtStatusToDosError") - baseNtDll;
	api->functions.winFunc.ntdll._formatPrint = (DWORD)LOAD_PROC(hNtDll, "_vsnprintf_s") - baseNtDll;
	api->functions.winFunc.ntdll._ntYieldExecution = (DWORD)LOAD_PROC(hNtDll, "NtYieldExecution") - baseNtDll;
	api->functions.winFunc.ntdll._flushInstructionCache = (DWORD)LOAD_PROC(hNtDll, "NtFlushInstructionCache") - baseNtDll;
	api->functions.winFunc.ntdll._createEvent = (DWORD)LOAD_PROC(hNtDll, "NtCreateEvent") - baseNtDll;
	api->functions.winFunc.ntdll._setEvent = (DWORD)LOAD_PROC(hNtDll, "NtSetEvent") - baseNtDll;

	return true;
}


#endif

