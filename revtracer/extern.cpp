#include <stdlib.h>
//#include <stdio.h>
#include <stdarg.h>
#include <share.h>

#include <Windows.h>
#include <winternl.h>
#include <intrin.h>

#include "extern.h"

/*#ifdef USE_VBOX_SNAPSHOTS
#include "VBoxSnapshotDll.h"

HMODULE hVboxDll = NULL;
BDTakeSnapshotType takeSnapshot = NULL;
BDRestoreSnapshotType restoreSnapshot = NULL;
BDSnapshotInitType initSnapshot = NULL;

struct SnapshotInitializer {
	SnapshotInitializer() {
		hVboxDll = LoadLibrary(VBOXSNAPSHOTDLL_DLL_NAME);

		if (NULL == hVboxDll) {
			return;
		}

		initSnapshot = (BDSnapshotInitType)GetProcAddress(hVboxDll, "BDSnapshotInit");

		if ((NULL == initSnapshot) || (FALSE == initSnapshot())) {
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
#endif*/

void __declspec(dllimport) RtlSetLastWin32Error(ULONG);
int __stdcall MyBaseSetLastNTError(NTSTATUS nErr) {
	
	ULONG dErr = RtlNtStatusToDosError(nErr);
	//RtlSetLastWin32Error(dErr);
	return dErr;
}

QWORD __TakeSnapshot() {
#ifdef USE_VBOX_SNAPSHOTS
	/*if (NULL != takeSnapshot) {
		return takeSnapshot();
	}
	return BD_SNAPSHOT_FAIL;*/
#endif
	return 0;
}

TakeSnapshotFunc TakeSnapshot = __TakeSnapshot;

QWORD __RestoreSnapshot() {
#ifdef USE_VBOX_SNAPSHOTS
	/*if (NULL != restoreSnapshot) {
		return restoreSnapshot();
	}
	return BD_SNAPSHOT_FAIL;*/
#endif
	return 0;
}

RestoreSnapshotFunc RestoreSnapshot = __RestoreSnapshot;

__declspec(dllexport) DWORD myRtlExitUserProcess;

//DWORD dwExitProcess = (DWORD)RtlExitUserProcess;

__declspec(dllimport) int vsnprintf_s(
	char *buffer,
	size_t sizeOfBuffer,
	size_t count,
	const char *format,
	va_list argptr
);

/*struct _ {
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

void __DbgPrint(const char *fmt, ...) {
	va_list va;
	char tmpBuff[512];

	va_start(va, fmt);
	int sz = vsnprintf_s(tmpBuff, sizeof(tmpBuff)-1, sizeof(tmpBuff)-1, fmt, va);
	va_end(va);

	unsigned long wr;
	WriteFile(__.fDbg, tmpBuff, sz * sizeof(tmpBuff[0]), &wr, NULL);
	//FlushFileBuffers(__.fDbg);
}*/

void ___DbgPrint(const char *fmt, ...) {
}

DbgPrintFunc DbgPrint = ___DbgPrint;


typedef NTSTATUS(NTAPI *NtAllocateVirtualMemoryFunc)(
	IN HANDLE               ProcessHandle,
	IN OUT PVOID            *BaseAddress,
	IN ULONG                ZeroBits,
	IN OUT PULONG           RegionSize,
	IN ULONG                AllocationType,
	IN ULONG                Protect);

__declspec(dllexport) NtAllocateVirtualMemoryFunc MyNtAllocateVirtualMemory;

LPVOID __stdcall MyVirtualAllocEx(HANDLE hProcess, LPVOID lpAddress, SIZE_T dwSize, DWORD flAllocationType, DWORD flProtect) {
	LPVOID addr = lpAddress;
	if (lpAddress && (unsigned int)lpAddress < 0x10000) {
		//RtlSetLastWin32Error(87);
	} else {
		NTSTATUS ret = MyNtAllocateVirtualMemory(
			hProcess, 
			&addr, 
			0, 
			&dwSize, 
			flAllocationType & 0xFFFFFFF0, 
			flProtect
		);
		if (NT_SUCCESS(ret)) {
			return addr;
		}
		MyBaseSetLastNTError(ret);
	}
	return 0;
}


LPVOID __stdcall MyVirtualAlloc(LPVOID lpAddress, SIZE_T dwSize, DWORD flAllocationType, DWORD flProtect) {
	return MyVirtualAllocEx((HANDLE)0xFFFFFFFF, lpAddress, dwSize, flAllocationType, flProtect);
}


void *__EnvMemoryAlloc(unsigned long dwSize) {
	//return ExAllocatePoolWithTag(NonPagedPool, dwSize, 0x3070754C);
	void *ret = MyVirtualAlloc(NULL, dwSize, MEM_RESERVE | MEM_COMMIT, PAGE_EXECUTE_READWRITE);

	return ret;
}

MemoryAllocFunc EnvMemoryAlloc = __EnvMemoryAlloc;

void __EnvMemoryFree(void *b) {
	//ExFreePoolWithTag(b, 0x3070754C);
	//VirtualFree(b);
}

MemoryFreeFunc EnvMemoryFree = __EnvMemoryFree;

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
	//printf("Guest process exited with code %d\n", retCode);
}