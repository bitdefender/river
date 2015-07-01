#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <share.h>

#include <Windows.h>

#include "extern.h"

#ifdef USE_VBOX_SNAPSHOTS
#include "VBoxSnapshotDll.h"

HMODULE hVboxDll = NULL;
BDTakeSnapshotType takeSnapshot = NULL;
BDRestoreSnapshotType restoreSnapshot = NULL;

struct SnapshotInitializer {
	SnapshotInitializer() {
		hVboxDll = LoadLibrary(VBOXSNAPSHOTDLL_DLL_NAME);

		if (NULL == hVboxDll) {
			return;
		}

		takeSnapshot = (BDTakeSnapshotType)GetProcAddress(hVboxDll, "BDTakeSnapshot");
		restoreSnapshot = (BDTakeSnapshotType)GetProcAddress(hVboxDll, "BDRestoreSnapshot");
	}

	~SnapshotInitializer() {
		if (NULL != hVboxDll) {
			FreeLibrary(hVboxDll);
		}
	}
} _snapshotInitializer;
#endif

unsigned long long TakeSnapshot() {
#ifdef USE_VBOX_SNAPSHOTS
	if (NULL != takeSnapshot) {
		return takeSnapshot();
	}
	return BD_SNAPSHOT_FAIL;
#endif
}

unsigned long long RestoreSnapshot() {
#ifdef USE_VBOX_SNAPSHOTS
	if (NULL != restoreSnapshot) {
		return restoreSnapshot();
	}
	return BD_SNAPSHOT_FAIL;
#endif
}

DWORD dwExitProcess = (DWORD)ExitProcess;

struct _ {
	FILE *fDbg;
	_() {
		fDbg = _fsopen("translation.log", "wt", _SH_DENYWR);
	}
} __;

void DbgPrint(const char *fmt, ...) {
	va_list va;

	va_start(va, fmt);
	vfprintf(__.fDbg, fmt, va);
	va_end(va);

	fflush(__.fDbg);
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