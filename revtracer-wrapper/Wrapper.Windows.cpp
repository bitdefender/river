#if defined _WIN32 || defined __CYGWIN__

#include <Windows.h>

#include "Wrapper.Global.h"

#include "RevtracerWrapper.h"
#include "../CommonCrossPlatform/Common.h"

DLL_LOCAL lib_t libhandler;

#ifndef NT_SUCCESS
#define NT_SUCCESS(Status) (((NTSTATUS)(Status)) >= 0)
#endif

// ------------------- Memory allocation ----------------------
typedef NTSTATUS(__stdcall *AllocateMemoryHandler)(
	HANDLE               ProcessHandle,
	LPVOID               *BaseAddress,
	DWORD                ZeroBits,
	size_t               *RegionSize,
	DWORD                AllocationType,
	DWORD                Protect);

AllocateMemoryHandler _virtualAlloc;

LPVOID __stdcall Kernel32VirtualAllocEx(HANDLE hProcess, LPVOID lpAddress, size_t dwSize, DWORD flAllocationType, DWORD flProtect) {
	LPVOID addr = lpAddress;
	if (lpAddress && (unsigned int)lpAddress < 0x10000) {
		//RtlSetLastWin32Error(87);
	}
	else {
		NTSTATUS ret = _virtualAlloc(
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
		SetLastError(ret);
	}
	return 0;
}

LPVOID __stdcall Kernel32VirtualAlloc(LPVOID lpAddress, size_t dwSize, DWORD flAllocationType, DWORD flProtect) {
	return Kernel32VirtualAllocEx((HANDLE)0xFFFFFFFF, lpAddress, dwSize, flAllocationType, flProtect);
}

void *WinAllocateVirtual(unsigned long size) {
	return Kernel32VirtualAlloc(NULL, size, MEM_RESERVE | MEM_COMMIT, PAGE_EXECUTE_READWRITE);
}

// -------------------- Flush Memory Cache --------------------

typedef BOOL(*RtlFlushSecureMemoryCacheHandler)(
		PVOID 	MemoryCache,
		SIZE_T 	MemoryLength
	);

RtlFlushSecureMemoryCacheHandler _flushMemoryCache;

// ------------------- Memory deallocation --------------------
typedef NTSTATUS(*FreeMemoryHandler)(
	HANDLE ProcessHandle,
	PVOID *BaseAddress,
	PULONG RegionSize,
	ULONG FreeType
);

FreeMemoryHandler _virtualFree;

BOOL Kernel32VirtualFreeEx(
		HANDLE hProcess,
		LPVOID lpAddress,
		SIZE_T dwSize,
		DWORD  dwFreeType
	) {
		NTSTATUS ret;

		if ((unsigned __int16)(dwFreeType & 0x8000) && dwSize) {
			return FALSE;
		} else {
			ret = ((FreeMemoryHandler)_virtualFree)(hProcess,
					&lpAddress, (PULONG)dwSize, dwFreeType);
			if (ret >= 0) {
				return TRUE;
			}

			if ((0xC0000045 == ret) && ((HANDLE)0xFFFFFFFF == hProcess)) {
				if (FALSE == ((RtlFlushSecureMemoryCacheHandler)_flushMemoryCache)(lpAddress, dwSize)) {
					return FALSE;
				}
				ret = ((FreeMemoryHandler)_virtualFree)(hProcess,
							&lpAddress, (PULONG)dwSize, dwFreeType);
				return (ret >= 0) ? TRUE : FALSE;
			} else {
				return FALSE;
			}
		}
	}

void WinFreeVirtual(void *address) {
	_virtualFree((HANDLE)0xFFFFFFFF, &address, 0, MEM_RELEASE);
}

// ------------------- Memory mapping -------------------------

typedef enum _SECTION_INHERIT {
		ViewShare = 1,
		ViewUnmap = 2
	} SECTION_INHERIT, *PSECTION_INHERIT;

typedef NTSTATUS(*MapMemoryHandler) (
		HANDLE          SectionHandle,
		HANDLE          ProcessHandle,
		PVOID           *BaseAddress,
		ULONG_PTR       ZeroBits,
		SIZE_T          CommitSize,
		PLARGE_INTEGER  SectionOffset,
		PSIZE_T         ViewSize,
		SECTION_INHERIT InheritDisposition,
		ULONG           AllocationType,
		ULONG           Win32Protect
	);

MapMemoryHandler _mapMemory;

LPVOID Kernel32MapViewOfFileEx(
		HANDLE hFileMappingObject,
		DWORD dwDesiredAccess,
		DWORD dwFileOffsetHigh,
		DWORD dwFileOffsetLow,
		SIZE_T dwNumberOfBytesToMap,
		LPVOID lpBaseAddress
		) {
	NTSTATUS Status;
	LARGE_INTEGER SectionOffset;
	SIZE_T ViewSize;
	ULONG Protect;
	LPVOID ViewBase;

	/* Convert the offset */
	SectionOffset.LowPart = dwFileOffsetLow;
	SectionOffset.HighPart = dwFileOffsetHigh;

	/* Save the size and base */
	ViewBase = lpBaseAddress;
	ViewSize = dwNumberOfBytesToMap;

	/* Convert flags to NT Protection Attributes */
	if (dwDesiredAccess == FILE_MAP_COPY) {
		Protect = PAGE_WRITECOPY;
	} else if (dwDesiredAccess & FILE_MAP_WRITE) {
		Protect = (dwDesiredAccess & FILE_MAP_EXECUTE) ? PAGE_EXECUTE_READWRITE : PAGE_READWRITE;
	} else if (dwDesiredAccess & FILE_MAP_READ) {
		Protect = (dwDesiredAccess & FILE_MAP_EXECUTE) ? PAGE_EXECUTE_READ : PAGE_READONLY;
	} else {
		Protect = PAGE_NOACCESS;
	}

	/* Map the section */
	Status = _mapMemory(
			hFileMappingObject,
			(HANDLE)0xFFFFFFFF,
			&ViewBase,
			0,
			0,
			&SectionOffset,
			&ViewSize,
			ViewShare,
			0,
			Protect
			);

	if (!NT_SUCCESS(Status)) {
		/* We failed */
		__asm int 3;
		return NULL;
	}

	/* Return the base */
	return ViewBase;
}

void *WinMapMemory(unsigned long mapHandler, unsigned long access, unsigned long offset, unsigned long size, void *address) {
	return Kernel32MapViewOfFileEx(
			(void *)mapHandler,
			access,
			0,
			offset,
			size,
			address
			);
}


// ------------------- Process termination --------------------
typedef NTSTATUS(__stdcall *TerminateProcessHandler)(
	HANDLE ProcessHandle,
	NTSTATUS ExitStatus
);

TerminateProcessHandler _terminateProcess;

void WinTerminateProcess(int retCode) {
	_terminateProcess((HANDLE)0xFFFFFFFF, retCode);
}

void *WinGetTerminationCodeFunc() {
	return (void *)_terminateProcess;
}


// ------------------- Write file -----------------------------
void *GetTEB() {
	DWORD r;
	__asm mov eax, dword ptr fs : [0x18];
	__asm mov r, eax
	return (void *)r;
}

void *GetPEB(void *teb) {
	return (void *)*((DWORD *)teb + 0x0C);
}

typedef struct _RTL_USER_PROCESS_PARAMETERS
{
	ULONG MaximumLength;
	ULONG Length;
	ULONG Flags;
	ULONG DebugFlags;
	PVOID ConsoleHandle;
	ULONG ConsoleFlags;
	PVOID StandardInput;
	PVOID StandardOutput;
	PVOID StandardError;
} RTL_USER_PROCESS_PARAMETERS, *PRTL_USER_PROCESS_PARAMETERS;

typedef struct _IO_STATUS_BLOCK {
	union {
		NTSTATUS Status;
		PVOID    Pointer;
	};
	ULONG_PTR Information;
} IO_STATUS_BLOCK, *PIO_STATUS_BLOCK;

PRTL_USER_PROCESS_PARAMETERS GetUserProcessParameters(void *peb) {
	return (PRTL_USER_PROCESS_PARAMETERS)(*((DWORD *)peb + 0x10));
}

typedef NTSTATUS(__stdcall *WriteFileHandler)(
	HANDLE FileHandle,
	HANDLE Event,
	PVOID ApcRoutine,
	PVOID ApcContext,
	PIO_STATUS_BLOCK IoStatusBlock,
	PVOID Buffer,
	ULONG Length,
	PVOID ByteOffset,
	PVOID Key
);

WriteFileHandler _writeFile;

typedef NTSTATUS(__stdcall *NtWaitForSingleObjectFunc)(
	HANDLE Handle,
	BOOL Alertable,
	PVOID Timeout
);

NtWaitForSingleObjectFunc _waitForSingleObject;

//#define STATUS_PENDING 0x103

BOOL Kernel32WriteFile(
	HANDLE hFile,
	PVOID lpBuffer,
	DWORD nNumberOfBytesToWrite,
	LPDWORD lpNumberOfBytesWritten
) {
	IO_STATUS_BLOCK ioStatus;

	ioStatus.Status = 0;
	ioStatus.Information = 0;

	if (lpNumberOfBytesWritten) {
		*lpNumberOfBytesWritten = 0;
	}

	PRTL_USER_PROCESS_PARAMETERS upp = GetUserProcessParameters(GetPEB(GetTEB()));
	HANDLE hIntFile = hFile;
	switch ((DWORD)hFile) {
	case 0xFFFFFFF4:
		hIntFile = upp->StandardError;
		break;
	case 0xFFFFFFF5:
		hIntFile = upp->StandardOutput;
		break;
	case 0xFFFFFFF6:
		hIntFile = upp->StandardInput;
		break;
	};

	NTSTATUS ret = _writeFile( 
		hIntFile,
		NULL,
		NULL,
		NULL,
		&ioStatus,
		lpBuffer,
		nNumberOfBytesToWrite,
		NULL,
		NULL
		);

	if (ret == STATUS_PENDING) {
		ret = _waitForSingleObject(
			hIntFile,
			FALSE,
			NULL
			);
		if (ret < 0) {
			if ((ret & 0xC0000000) == 0x80000000) {
				*lpNumberOfBytesWritten = ioStatus.Information;
			}
			//DefaultSetLastError(ret);
			return FALSE;
		}
		ret = ioStatus.Status;
	}

	if (ret >= 0) {
		*lpNumberOfBytesWritten = ioStatus.Information;
		return TRUE;
	}
	if ((ret & 0xC0000000) == 0x80000000) {
		*lpNumberOfBytesWritten = ioStatus.Information;
	}
	//DefaultSetLastError(ret);
	return FALSE;
}

bool WinWriteFile(void *handle, void *buffer, size_t size, unsigned long *written) {
	return TRUE == Kernel32WriteFile(handle, buffer, size, written);
}

// ------------------- Error codes ----------------------------

typedef DWORD(__stdcall *ConvertToSystemErrorHandler)(
	NTSTATUS status
);

ConvertToSystemErrorHandler _systemError;


long WinToErrno(long ntStatus) {
	return _systemError(ntStatus);
}

// ------------------- Formatted print ------------------------

typedef int (*FormatPrintHandler)(
	char *buffer,
	size_t sizeOfBuffer,
	size_t count,
	const char *format,
	va_list argptr
);

FormatPrintHandler _formatPrint;

int WinFormatPrint(char *buffer, size_t sizeOfBuffer, const char *format, char *argptr) {
	return _formatPrint(buffer, sizeOfBuffer, _TRUNCATE, format, (va_list)argptr);
}

// ------------------- Yield Execution ------------------------

typedef long NTSTATUS;
typedef NTSTATUS(*NtYieldExecutionHandler)();

NtYieldExecutionHandler _ntYieldExecution;

long WinYieldExecution(void) {
	return _ntYieldExecution();
}

// ------------------- Flush instruction cache ----------------

typedef NTSTATUS(*FlushInstructionCacheHandler)(
		HANDLE hProcess,
		LPVOID address,
		SIZE_T size
	);

FlushInstructionCacheHandler _flushInstructionCache;

void WinFlushInstructionCache(void) {
	_flushInstructionCache((HANDLE)0xFFFFFFFF, NULL, 0);
}


// ------------------- Initialization -------------------------

namespace revwrapper {
	extern "C" int InitRevtracerWrapper() {
		libhandler = GET_LIB_HANDLER(L"ntdll.dll");

		if (!libhandler)
			return -1;

		// get functionality from ntdll
		_virtualAlloc = (AllocateMemoryHandler)LOAD_PROC(libhandler, "NtAllocateVirtualMemory");
		_virtualFree = (FreeMemoryHandler)LOAD_PROC(libhandler, "NtFreeVirtualMemory");
		_mapMemory = (MapMemoryHandler)LOAD_PROC(libhandler, "NtMapViewOfSection");

		_terminateProcess = (TerminateProcessHandler)LOAD_PROC(libhandler, "NtTerminateProcess");

		_writeFile = (WriteFileHandler)LOAD_PROC(libhandler, "NtWriteFile");
		_waitForSingleObject = (NtWaitForSingleObjectFunc)LOAD_PROC(libhandler, "NtWaitForSingleObject");

		_systemError = (ConvertToSystemErrorHandler)LOAD_PROC(libhandler, "RtlNtStatusToDosError");

		_formatPrint = (FormatPrintHandler)LOAD_PROC(libhandler, "_vsnprintf_s");

		_ntYieldExecution = (NtYieldExecutionHandler)LOAD_PROC(libhandler, "NtYieldExecution");

		_flushInstructionCache = (FlushInstructionCacheHandler)LOAD_PROC(libhandler, "NtFlushInstructionCache");

		// set global functionality
		allocateVirtual = WinAllocateVirtual;
		freeVirtual = WinFreeVirtual;
		mapMemory = WinMapMemory;

		terminateProcess = WinTerminateProcess;
		getTerminationCode = WinGetTerminationCodeFunc;
		writeFile = WinWriteFile;
		toErrno = WinToErrno;
		formatPrint = WinFormatPrint;

		flushInstructionCache = WinFlushInstructionCache;
		return 0;
	}
}; // namespace revwrapper

BOOL WINAPI DllMain(_In_ HINSTANCE hinstDLL, _In_ DWORD fdwReason, _In_ LPVOID lpvReserved) {
	return TRUE;
}

#endif
