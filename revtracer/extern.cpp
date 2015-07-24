#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <share.h>

#include <Windows.h>
#include <winternl.h>

#include "extern.h"

DWORD dwExitProcess = (DWORD)ExitProcess;

__declspec(dllimport) int vsnprintf_s(
	char *buffer,
	size_t sizeOfBuffer,
	size_t count,
	const char *format,
	va_list argptr
);

struct _ {
	HANDLE fDbg;
	_() {
		fDbg = CreateFile("translation.log", GENERIC_WRITE, FILE_SHARE_READ, NULL, TRUNCATE_EXISTING, 0, NULL);
		if (INVALID_HANDLE_VALUE == fDbg) {
			__asm int 3;
		}
	}

	~_() {
		CloseHandle(fDbg);
	}
} __;

void DbgPrint(const char *fmt, ...) {
	va_list va;
	char tmpBuff[512];

	va_start(va, fmt);
	int sz = _vsnprintf_s(tmpBuff, sizeof(tmpBuff) - 1, fmt, va);
	va_end(va);

	unsigned long wr;
	WriteFile(__.fDbg, tmpBuff, sz * sizeof(tmpBuff[0]), &wr, NULL);
	//FlushFileBuffers(__.fDbg);
}

void *EnvMemoryAlloc(unsigned long dwSize) {
	//return ExAllocatePoolWithTag(NonPagedPool, dwSize, 0x3070754C);
	void *ret = VirtualAlloc(NULL, dwSize, MEM_RESERVE | MEM_COMMIT, PAGE_EXECUTE_READWRITE);

	return ret;
}

void EnvMemoryFree(void *b) {
	//ExFreePoolWithTag(b, 0x3070754C);
	//VirtualFree(b);
}

typedef BOOL (*MyIPFPFunc)(DWORD ProcessorFeature);

BOOL __stdcall MyIsProcessorFeaturePresent(DWORD ProcessorFeature) {
	static MyIPFPFunc func = NULL;

	if (NULL == func) {
		HMODULE hKernel = GetModuleHandle("kernel32.dll");
		func = (MyIPFPFunc)GetProcAddress(hKernel, "IsProcessorFeaturePresent");
	}

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
		return func(ProcessorFeature);
	}
}

VOID __stdcall MyExitProcess(DWORD retCode) {
	//printf("Guest process exited with code %d\n", retCode);
}