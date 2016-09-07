#ifdef _WINDOWS
#include <Windows.h>
#elif defined(_LINUX)
// linux include
#endif

#include "Inproc.Mapper.h"


#ifdef _WINDOWS

// virtual memory functions
#define VIRTUAL_ALLOC(addr, size, protect) VirtualAlloc((addr), (size), MEM_RESERVE | MEM_COMMIT, (protect))
#define VIRTUAL_PROTECT(addr, size, newProtect, oldProtect) (TRUE == VirtualProtect((addr), (size), (newProtect), &(oldProtect)))

// module functions
#define MODULE_PTR HMODULE
#define GET_MODULE_HANDLE(name) GetModuleHandleA((name))
#define GET_PROC_ADDRESS_BY_NAME(module, name) GetProcAddress((module), (name))
#define GET_PROC_ADDRESS_BY_ORDINAL(module, name) GetProcAddress((module), (LPCSTR)(name))
#elif defined(_LINUX)

// virtual memory functions
#define VIRTUAL_ALLOC(addr, size, protect) 
#define VIRTUAL_PROTECT(addr, size, newProtect, oldProtect)

// module functions
#define MODULE_PTR void *
#define GET_MODULE_HANDLE(name)
#define GET_PROC_ADDRESS_BY_NAME(module, name)
#define GET_PROC_ADDRESS_BY_ORDINAL(module, name)
#endif


void *InprocMapper::CreateSection(void *lpAddress, size_t dwSize, DWORD flProtect) {
	return VIRTUAL_ALLOC(lpAddress, dwSize, flProtect);
}

bool InprocMapper::ChangeProtect(void *lpAddress, size_t dwSize, DWORD flProtect) {
	DWORD oldProtect;
	return VIRTUAL_PROTECT(lpAddress, dwSize, flProtect, oldProtect);
}

bool InprocMapper::WriteBytes(void *lpAddress, void *lpBuffer, size_t nSize) {
	memcpy(lpAddress, lpBuffer, nSize);
	return true;
}

DWORD InprocMapper::FindImport(const char *moduleName, const char *funcName) {
	MODULE_PTR hModule = GET_MODULE_HANDLE(moduleName);

	if (NULL == hModule) {
		return 0xFFFFFFFF;
	}

	return (DWORD)GET_PROC_ADDRESS_BY_NAME(hModule, funcName);
}

DWORD InprocMapper::FindImport(const char *moduleName, const unsigned int funcOrdinal) {
	MODULE_PTR hModule = GET_MODULE_HANDLE(moduleName);

	if (NULL == hModule) {
		return 0xFFFFFFFF;
	}

	return (DWORD)GET_PROC_ADDRESS_BY_ORDINAL(hModule, funcOrdinal);
}