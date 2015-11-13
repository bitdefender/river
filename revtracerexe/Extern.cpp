#include <stdlib.h>
//#include <stdio.h>
#include <stdarg.h>
#include <share.h>

#include <Windows.h>
#include <winternl.h>
#include <intrin.h>

#include "extern.h"
#include "../revtracer/revtracer.h"

__declspec(dllimport) int vsnprintf_s(
	char *buffer,
	size_t sizeOfBuffer,
	size_t count,
	const char *format,
	va_list argptr
);

void ___DbgPrint(const char *fmt, ...) {
}

DbgPrintFunc DbgPrint = ___DbgPrint;

typedef BOOL (*MyIPFPFunc)(DWORD ProcessorFeature);

BOOL __stdcall MyIsProcessorFeaturePresent(DWORD ProcessorFeature) {
	static MyIPFPFunc func = NULL;

	/*if (NULL == func) {
		HMODULE hKernel = GetModuleHandle("kernel32.dll");
		func = (MyIPFPFunc)GetProcAddress(hKernel, "IsProcessorFeaturePresent");
	}*/

	switch (ProcessorFeature) {
	case PF_MMX_INSTRUCTIONS_AVAILABLE :
	case PF_SSE3_INSTRUCTIONS_AVAILABLE :
	case PF_XMMI64_INSTRUCTIONS_AVAILABLE :
	case PF_XMMI_INSTRUCTIONS_AVAILABLE :
	case PF_COMPARE_EXCHANGE_DOUBLE :
	case PF_COMPARE_EXCHANGE128 :
	case PF_COMPARE64_EXCHANGE128 :
		return FALSE;
	default:
		//return func(ProcessorFeature);
		return TRUE;
	}
}

VOID __stdcall MyExitProcess(DWORD retCode) {
	//rev::RevtracerAPI *api = &rev::revtracerAPI;
	//api->dbgPrintFunc("Guest process exited with code %d\n", retCode);
	ExitProcess(retCode);
}