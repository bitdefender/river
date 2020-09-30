#include <Windows.h>
#include "Mem.Mapper.h"


void *MemMapper::CreateSection(void *lpAddress, size_t dwSize, DWORD flProtect) {
	return lpAddress;
}

bool MemMapper::ChangeProtect(void *lpAddress, size_t dwSize, DWORD flProtect) {
	return true;
}

bool MemMapper::WriteBytes(void *lpAddress, void *lpBuffer, size_t nSize) {
	memcpy(lpAddress, lpBuffer, nSize);
	return true;
}

DWORD MemMapper::FindImport(const char *moduleName, const char *funcName) {
	HMODULE hModule = GetModuleHandleA(moduleName);

	if (NULL == hModule) {
		return 0xFFFFFFFF;
	}

	return (DWORD)GetProcAddress(hModule, funcName);
}

DWORD MemMapper::FindImport(const char *moduleName, const unsigned int funcOrdinal) {
	HMODULE hModule = GetModuleHandleA(moduleName);

	if (NULL == hModule) {
		return 0xFFFFFFFF;
	}

	return (DWORD)GetProcAddress(hModule, (LPCSTR)funcOrdinal);
}