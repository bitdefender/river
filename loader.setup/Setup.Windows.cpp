#ifndef __linux__

#include "../CommonCrossPlatform/Common.h"
#include "Loader.Setup.h"

extern "C" bool InitLoaderOffsets(ext::LibraryLayout *libs, ldr::LoaderImports *api) {

	LIB_T hNtDll = GET_LIB_HANDLER(L"ntdll.dll");
	DWORD baseNtDll = GET_LIB_BASE(hNtDll);

	if (!hNtDll) {
		return false;
	}

	libs->winLib.ntdllBase = baseNtDll;

	api->functions.winFunc.ntdll._ntMapViewOfSection = (DWORD)LOAD_PROC(hNtDll, "NtMapViewOfSection") - baseNtDll;
	api->functions.winFunc.ntdll._ntFlushInstructionCache = (DWORD)LOAD_PROC(hNtDll, "NtFlushInstructionCache") - baseNtDll;
	api->functions.winFunc.ntdll._ntFreeVirtualMemory = (DWORD)LOAD_PROC(hNtDll, "NtFreeVirtualMemory") - baseNtDll;

	return true;
}


#endif