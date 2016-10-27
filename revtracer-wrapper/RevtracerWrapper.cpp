#if defined _WIN32 || defined __CYGWIN__
#include <Windows.h>
#endif

#include "RevtracerWrapper.h"
#include "../CommonCrossPlatform/Common.h"

namespace revwrapper {

	#ifdef __linux__
	#define GET_LIB_HANDLER(libname) dlopen((libname), RTLD_LAZY)
	#define CLOSE_LIB(libhandler) dlclose((libhandler))
	#define LOAD_PROC(libhandler, szProc) dlsym((libhandler), (szProc))
	#else
	#define GET_LIB_HANDLER(libname) GetModuleHandleW((libname))
	#define CLOSE_LIB
	#define LOAD_PROC(libhandler, szProc) GetProcAddress((libhandler), (szProc))
	#endif

	#ifdef _WIN32
	BOOL WINAPI DllMain(_In_ HINSTANCE hinstDLL, _In_ DWORD fdwReason, _In_ LPVOID lpvReserved) {
		return TRUE;
	}
	#endif

	//Platform specific functions and types
	#ifdef __linux__
	typedef void* (*AllocateMemoryHandler)(void *addr,
			size_t length, int prot, int flags,int fd, off_t offset);
	typedef long (*ConvertToSystemErrorHandler)(long status);
	typedef void(*TerminateProcessHandler)(int status);
	typedef int (*FreeMemoryHandler)(void *addr, size_t length);
	typedef ssize_t (*WriteFileHandler)(int fd, const void *buf, size_t count);
	#else
	#ifndef NT_SUCCESS
		#define NT_SUCCESS(Status) (((NTSTATUS)(Status)) >= 0)
	#endif

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
			ADDR_TYPE proc = GetAllocateMemoryHandler();
			NTSTATUS ret = ((AllocateMemoryHandler)proc)(
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

	typedef DWORD(__stdcall *ConvertToSystemErrorHandler)(NTSTATUS status);

	typedef NTSTATUS(__stdcall *TerminateProcessHandler)(
		HANDLE ProcessHandle,
		NTSTATUS ExitStatus
	);
	BOOL Kernel32TerminateProcess(HANDLE hProcess, DWORD uExitCode) {
		ADDR_TYPE proc = GetTerminateProcessHandler();
		return ((TerminateProcessHandler)proc)(hProcess, uExitCode);
	}

	typedef NTSTATUS (*FreeMemoryHandler)(
		HANDLE ProcessHandle,
		PVOID *BaseAddress,
		PULONG RegionSize,
		ULONG FreeType
	);

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

		ADDR_TYPE proc = GetWriteFileHandler();
		NTSTATUS ret = ((WriteFileHandler)proc) (
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

		proc = LOAD_PROC(libhandler, "NtWaitForSingleObject");
		if (ret == STATUS_PENDING) {
			ret = ((NtWaitForSingleObjectFunc)proc)(
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

	#endif

	//Library API implementation
	DLL_PUBLIC extern ADDR_TYPE RevtracerWrapperInit(void)
	{
#ifdef _WIN32
		libhandler = GET_LIB_HANDLER(L"ntdll.dll");
#elif defined(__linux__)
		libhandler = GET_LIB_HANDLER("libc.so");
#endif
		return libhandler;
	}

	DLL_PUBLIC extern void *CallAllocateMemoryHandler(unsigned long dwSize)
	{
		#ifdef __linux__
		ADDR_TYPE proc = GetAllocateMemoryHandler();
		return ((AllocateMemoryHandler)proc)(NULL, dwSize, 0x7, 0x21, 0, 0); // RWX shared anonymous
		#else
		return Kernel32VirtualAlloc(NULL, dwSize, MEM_RESERVE | MEM_COMMIT, PAGE_EXECUTE_READWRITE);
		#endif
	}

	DLL_PUBLIC extern void CallFreeMemoryHandler(void *address, unsigned long dwSize)
	{ }

	DLL_PUBLIC extern void CallTerminateProcessHandler(void) {
		#ifdef __linux__
		ADDR_TYPE proc = GetTerminateProcessHandler();
		((TerminateProcessHandler)proc)(0);
		#else
		Kernel32TerminateProcess((HANDLE)0xFFFFFFFF, 0);
		#endif
	}

	DLL_PUBLIC extern int CallFormattedPrintHandler(char * buffer, size_t sizeOfBuffer, size_t count, const char * format, va_list argptr)
	{
		return 0;
	}

	DLL_PUBLIC extern bool CallWriteFile(void * handle, int fd, void * buffer, size_t size, unsigned long * written)
	{
		#ifdef __linux__
		ADDR_TYPE proc = GetWriteFileHandler();
		*written = ((WriteFileHandler)proc)(fd, buffer, size);
		return (0 <= written);
		#else
		return Kernel32WriteFile(handle, buffer, size, written);
		#endif
	}

	DLL_PUBLIC extern long CallConvertToSystemErrorHandler(long status) {
		ADDR_TYPE proc = GetConvertToSystemErrorHandler();
		return ((ConvertToSystemErrorHandler)proc)(status);
	}

	//Local functions definitions
#ifdef __linux__
	DLL_LOCAL long ConvertToSystemError(long status) {
		return status;
	}
#endif
	DLL_LOCAL long SetLastError(long status) {
		ADDR_TYPE proc = GetConvertToSystemErrorHandler();
		revtracerLastError = ((ConvertToSystemErrorHandler)proc)(status);
		return revtracerLastError;
	}

	DLL_LOCAL ADDR_TYPE GetAllocateMemoryHandler(void) {
		#ifdef __linux__
		return LOAD_PROC(libhandler, "mmap");
		#else
		return LOAD_PROC(libhandler, "NtAllocateVirtualMemory");
		#endif
	}

	DLL_LOCAL ADDR_TYPE GetConvertToSystemErrorHandler(void)
	{
		#ifdef __linux__
		return (ADDR_TYPE)&ConvertToSystemError;
		#else
		return LOAD_PROC(libhandler, "RtlNtStatusToDosError");
		#endif
	}

	DLL_LOCAL ADDR_TYPE GetTerminateProcessHandler(void)
	{
		#ifdef __linux__
		return LOAD_PROC(libhandler, "exit");
		#else
		return LOAD_PROC(libhandler, "NtTerminateProcess");
		#endif
	}

	DLL_LOCAL ADDR_TYPE GetFreeMemoryHandler(void)
	{
		#ifdef __linux__
		return LOAD_PROC(libhandler, "munmap");
		#else
		return LOAD_PROC(libhandler, "NtFreeVirtualMemory");
		#endif
	}

	DLL_LOCAL ADDR_TYPE GetFormattedPrintHandler(void)
	{
		#ifdef __linux__
		return LOAD_PROC(libhandler, "vsnprintf");
		#else
		return LOAD_PROC(libhandler, "_vsnprintf_s");
		#endif
	}
	DLL_LOCAL ADDR_TYPE GetWriteFileHandler(void)
	{
		#ifdef __linux__
		return LOAD_PROC(libhandler, "write");
		#else
		return LOAD_PROC(libhandler, "NtWriteFile");
		#endif
	}
}; //namespace revwrapper
