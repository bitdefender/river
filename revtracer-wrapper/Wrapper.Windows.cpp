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

// ------------------- Memory deallocation --------------------
typedef NTSTATUS(*FreeMemoryHandler)(
	HANDLE ProcessHandle,
	PVOID *BaseAddress,
	PULONG RegionSize,
	ULONG FreeType
);

FreeMemoryHandler _virtualFree;

void WinFreeVirtual(void *address) {
	_virtualFree((HANDLE)0xFFFFFFFF, &address, 0, MEM_RELEASE);
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

// ------------------- Initialization -------------------------

namespace revwrapper {
	extern "C" void InitRevtracerWrapper() {
		libhandler = GET_LIB_HANDLER(L"ntdll.dll");

		// get functionality from ntdll
		_virtualAlloc = (AllocateMemoryHandler)LOAD_PROC(libhandler, "NtAllocateVirtualMemory");
		_virtualFree = (FreeMemoryHandler)LOAD_PROC(libhandler, "NtFreeVirtualMemory");

		_terminateProcess = (TerminateProcessHandler)LOAD_PROC(libhandler, "NtTerminateProcess");

		_writeFile = (WriteFileHandler)LOAD_PROC(libhandler, "NtWriteFile");
		_waitForSingleObject = (NtWaitForSingleObjectFunc)LOAD_PROC(libhandler, "NtWaitForSingleObject");

		_systemError = (ConvertToSystemErrorHandler)LOAD_PROC(libhandler, "RtlNtStatusToDosError");

		_formatPrint = (FormatPrintHandler)LOAD_PROC(libhandler, "_vsnprintf_s");


		// set global functionality
		allocateVirtual = WinAllocateVirtual;
		freeVirtual = WinFreeVirtual;

		terminateProcess = WinTerminateProcess;
		getTerminationCode = WinGetTerminationCodeFunc;
		writeFile = WinWriteFile;
		toErrno = WinToErrno;
		formatPrint = WinFormatPrint;
	}
}; // namespace revwrapper

BOOL WINAPI DllMain(_In_ HINSTANCE hinstDLL, _In_ DWORD fdwReason, _In_ LPVOID lpvReserved) {
	return TRUE;
}

#endif