#if defined _WIN32 || defined __CYGWIN__

#include <Windows.h>

#include "Wrapper.Global.h"

#include "RevtracerWrapper.h"
#include "../CommonCrossPlatform/Common.h"

revwrapper::LibraryLayout *libLayout;

struct WindowsFunctions {
	struct {
		unsigned int _virtualAlloc;
		unsigned int _virtualFree;
		unsigned int _mapMemory;

		unsigned int _flushMemoryCache;

		unsigned int _terminateProcess;

		unsigned int _writeFile;
		unsigned int _waitForSingleObject;

		unsigned int _systemError;

		unsigned int _formatPrint;

		unsigned int _ntYieldExecution;
		unsigned int _flushInstructionCache;

		unsigned int _createEvent;
		unsigned int _setEvent;
	} ntdll;
} windowsFunctions;

#define CALL_API(LIB, FUNC, TYPE) ((TYPE)((unsigned char *)libLayout->windows.##LIB##Base + windowsFunctions.##LIB##.##FUNC))


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

LPVOID __stdcall Kernel32VirtualAllocEx(HANDLE hProcess, LPVOID lpAddress, size_t dwSize, DWORD flAllocationType, DWORD flProtect) {
	LPVOID addr = lpAddress;
	if (lpAddress && (unsigned int)lpAddress < 0x10000) {
		//RtlSetLastWin32Error(87);
	}
	else {
		//NTSTATUS ret = ((AllocateMemoryHandler)((unsigned char *)libLayout->windows.ntdllBase + windowsFunctions.ntdll._virtualAlloc))(
		NTSTATUS ret = CALL_API(ntdll, _virtualAlloc, AllocateMemoryHandler) (
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

// ------------------- Memory deallocation --------------------
typedef NTSTATUS(*FreeMemoryHandler)(
	HANDLE ProcessHandle,
	PVOID *BaseAddress,
	PULONG RegionSize,
	ULONG FreeType
);

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
			ret = CALL_API(ntdll, _virtualFree, FreeMemoryHandler) (
				hProcess,
				&lpAddress, 
				(PULONG)dwSize, 
				dwFreeType
			);
			if (ret >= 0) {
				return TRUE;
			}

			if ((0xC0000045 == ret) && ((HANDLE)0xFFFFFFFF == hProcess)) {
				if (FALSE == CALL_API(ntdll, _flushMemoryCache, RtlFlushSecureMemoryCacheHandler) (lpAddress, dwSize)) {
					return FALSE;
				}
				ret = CALL_API(ntdll, _virtualFree, FreeMemoryHandler) (
					hProcess,
					&lpAddress, 
					(PULONG)dwSize, 
					dwFreeType
				);
				return (ret >= 0) ? TRUE : FALSE;
			} else {
				return FALSE;
			}
		}
	}

void WinFreeVirtual(void *address) {
	Kernel32VirtualFreeEx((HANDLE)0xFFFFFFFF, &address, 0, MEM_RELEASE);
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
	Status = CALL_API(ntdll, _mapMemory, MapMemoryHandler) (
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

void WinTerminateProcess(int retCode) {
	CALL_API(ntdll, _terminateProcess, TerminateProcessHandler) ((HANDLE)0xFFFFFFFF, retCode);
}

void *WinGetTerminationCodeFunc() {
	return (void *)CALL_API(ntdll, _terminateProcess, TerminateProcessHandler);
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

typedef NTSTATUS(__stdcall *NtWaitForSingleObjectFunc)(
	HANDLE Handle,
	BOOL Alertable,
	PVOID Timeout
);

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

	NTSTATUS ret = CALL_API(ntdll, _writeFile, WriteFileHandler) (
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
		ret = CALL_API(ntdll, _waitForSingleObject, NtWaitForSingleObjectFunc) (
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

// ------------------- Events ---------------------------------

#define CREATE_EVENT_MANUAL_RESET 1
#define CREATE_EVENT_INITIAL_SET  2

#define EVENT_ALL_ACCESS			0x1F0003

typedef struct _LSA_UNICODE_STRING {
	USHORT Length;
	USHORT MaximumLength;
	PWSTR  Buffer;
} LSA_UNICODE_STRING, *PLSA_UNICODE_STRING, UNICODE_STRING, *PUNICODE_STRING;

typedef struct _OBJECT_ATTRIBUTES {
	ULONG           Length;
	HANDLE          RootDirectory;
	PUNICODE_STRING ObjectName;
	ULONG           Attributes;
	PVOID           SecurityDescriptor;
	PVOID           SecurityQualityOfService;
}  OBJECT_ATTRIBUTES, *POBJECT_ATTRIBUTES;

int __stdcall BaseFormatObjectAttributes(OBJECT_ATTRIBUTES *objectAttributes)
{
	objectAttributes->SecurityQualityOfService = 0;
	objectAttributes->Attributes = 0;
	//v7 = (void(__stdcall *)(OBJECT_ATTRIBUTES *))dword_10152984;
	objectAttributes->RootDirectory = nullptr;
	objectAttributes->Length = 24;
	objectAttributes->ObjectName = nullptr;
	objectAttributes->SecurityDescriptor = nullptr;

	/*if (v7)
	{
		if (v7 == BasepAdjustObjectAttributesForPrivateNamespace)
		{
			BasepAdjustObjectAttributesForPrivateNamespace(objectAttributes);
		}
		else
		{
			__guard_check_icall_fptr(v7);
			v7(objectAttributes);
		}
	}*/
	//*(_DWORD *)a4 = objectAttributes;
	return 0;
}

typedef enum _EVENT_TYPE {
	NotificationEvent,
	SynchronizationEvent
} EVENT_TYPE;

typedef NTSTATUS(__stdcall *NtCreateEventFunc)(
	PHANDLE            EventHandle,
	ACCESS_MASK        DesiredAccess,
	POBJECT_ATTRIBUTES ObjectAttributes,
	EVENT_TYPE         EventType,
	BOOLEAN            InitialState
);

HANDLE __stdcall CreateEventExW(DWORD flags, ACCESS_MASK DesiredAccess)
{
	NTSTATUS ret; // eax@3
	NTSTATUS v6; // ecx@12
	OBJECT_ATTRIBUTES ObjectAttributes; // [sp+8h] [bp-28h]@3
	HANDLE EventHandle; // [sp+Ch] [bp-24h]@5
	//char v10; // [sp+18h] [bp-18h]@3

	if (flags & 0xFFFFFFFC)
	{
		v6 = 0xC00000F1;
		return nullptr;
	}
	
	ret = BaseFormatObjectAttributes(&ObjectAttributes);
	if (ret < 0) {
		return nullptr;
	}

	ret = CALL_API(ntdll, _createEvent, NtCreateEventFunc) (
		&EventHandle,
		DesiredAccess,
		&ObjectAttributes,
		(EVENT_TYPE)(~(BYTE)flags & 1),
		((BYTE)flags >> 1) & 1
	);

	if (ret < 0) {
		return nullptr;
	}

	/*if (ret == 0x40000000)
		RtlSetLastWin32Error(0xB7);
	else
		RtlSetLastWin32Error(0);*/
	return EventHandle;
}

HANDLE __stdcall CreateEventW(/*LPSECURITY_ATTRIBUTES lpEventAttributes = nullptr,*/ BOOL bManualReset, BOOL bInitialState /*, LPCWSTR lpName = nullptr */)
{
	DWORD flags;

	flags = 0;
	if (bManualReset) {
		flags = CREATE_EVENT_MANUAL_RESET;
	}
	if (bInitialState) {
		flags |= CREATE_EVENT_INITIAL_SET;
	}
		
	return CreateEventExW(flags, EVENT_ALL_ACCESS);
}

typedef NTSTATUS(__stdcall *NtSetEventFunc)(
	HANDLE EventHandle, 
	PULONG PreviousState
);

BOOL __stdcall SetEvent(HANDLE hEvent) {
	NTSTATUS ret = CALL_API(ntdll, _setEvent, NtSetEventFunc) (hEvent, 0);
	if (ret < 0) {
		return FALSE;
	}
	return true;
}



// ------------------- Error codes ----------------------------

typedef DWORD(__stdcall *ConvertToSystemErrorHandler)(
	NTSTATUS status
);

long WinToErrno(long ntStatus) {
	return CALL_API(ntdll, _systemError, ConvertToSystemErrorHandler) (ntStatus);
}

// ------------------- Formatted print ------------------------

typedef int (*FormatPrintHandler)(
	char *buffer,
	size_t sizeOfBuffer,
	size_t count,
	const char *format,
	va_list argptr
);

int WinFormatPrint(char *buffer, size_t sizeOfBuffer, const char *format, char *argptr) {
	return CALL_API(ntdll, _formatPrint, FormatPrintHandler) (buffer, sizeOfBuffer, _TRUNCATE, format, (va_list)argptr);
}

// ------------------- Yield Execution ------------------------

typedef long NTSTATUS;
typedef NTSTATUS(*NtYieldExecutionHandler)();

long WinYieldExecution(void) {
	return CALL_API(ntdll, _ntYieldExecution, NtYieldExecutionHandler) ();
}

// ------------------- Flush instruction cache ----------------

typedef NTSTATUS(*FlushInstructionCacheHandler)(
		HANDLE hProcess,
		LPVOID address,
		SIZE_T size
	);

void WinFlushInstructionCache(void) {
	CALL_API(ntdll, _flushInstructionCache, FlushInstructionCacheHandler) ((HANDLE)0xFFFFFFFF, NULL, 0);
}


// ------------------- Initialization -------------------------

namespace revwrapper {


	extern "C" {
		DLL_WRAPPER_PUBLIC int InitRevtracerWrapper(void *configPage) {
			

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
	};
}; // namespace revwrapper

DWORD DllEntry() {
	return 1;
}

#endif
