#include <Windows.h>
#include "Inproc.Mapper.h"


void *InprocMapper::CreateSection(void *lpAddress, size_t dwSize, DWORD flProtect) {
	return VirtualAlloc(lpAddress, dwSize, MEM_RESERVE | MEM_COMMIT | MEM_TOP_DOWN, flProtect);
}

bool InprocMapper::ChangeProtect(void *lpAddress, size_t dwSize, DWORD flProtect) {
	DWORD oldProtect;
	return TRUE == VirtualProtect(lpAddress, dwSize, flProtect, &oldProtect);
}

bool InprocMapper::WriteBytes(void *lpAddress, void *lpBuffer, size_t nSize) {
	memcpy(lpAddress, lpBuffer, nSize);
	return true;
}

DWORD InprocMapper::FindImport(const char *moduleName, const char *funcName) {
	HMODULE hModule = GetModuleHandleA(moduleName);

	if (NULL == hModule) {
		return 0xFFFFFFFF;
	}

	return (DWORD)GetProcAddress(hModule, funcName);
}

DWORD InprocMapper::FindImport(const char *moduleName, const unsigned int funcOrdinal) {
	HMODULE hModule = GetModuleHandleA(moduleName);

	if (NULL == hModule) {
		return 0xFFFFFFFF;
	}

	return (DWORD)GetProcAddress(hModule, (LPCSTR)funcOrdinal);
}