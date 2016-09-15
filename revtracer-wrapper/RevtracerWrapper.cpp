#if defined _WIN32 || defined __CYGWIN__
#include <Windows.h>
#endif

#include "RevtracerWrapper.h"

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

	#ifdef __linux__
	__attribute__((constructor)) void somain(void) {
		libhandler = GET_LIB_HANDLER("libc.so");
	}
	#else
	BOOL WINAPI DllMain(_In_ HINSTANCE hinstDLL, _In_ DWORD fdwReason, _In_ LPVOID lpvReserved) {
		return TRUE;
	}
	#endif

	//Platform specific functions and types
	#ifdef __linux__
	typedef void* (*AllocateMemoryHandler)(size_t size);
	typedef long (*ConvertToSystemErrorHandler)(long status);
	typedef void(*TerminateProcessHandler)(int status);
	typedef void (*FreeMemoryHandler)(void *ptr);
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
	#endif

	//Library API implementation
#ifdef _WIN32
	DLL_PUBLIC extern ADDR_TYPE RevtracerWrapperInit(void)
	{
		libhandler = GET_LIB_HANDLER(L"ntdll.dll");
		return libhandler;
	}
#endif

	DLL_PUBLIC extern void *CallAllocateMemoryHandler(unsigned long dwSize)
	{
		#ifdef __linux__
		ADDR_TYPE proc = GetAllocateMemoryHandler();
		return ((AllocateMemoryHandler)proc)(dwSize);
		#else
		return Kernel32VirtualAlloc(NULL, dwSize, MEM_RESERVE | MEM_COMMIT, PAGE_EXECUTE_READWRITE);
		#endif
	}

	DLL_PUBLIC extern void CallFreeMemoryHandler(void *address)
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
		return LOAD_PROC(libhandler, "malloc");
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
		return LOAD_PROC(libhandler, "free");
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
}; //namespace revwrapper
