#include <Windows.h>
#include "Extern.Mapper.h"

ExternMapper::ExternMapper(DWORD hnd) {
	dwHandle = hnd;
}

void *ExternMapper::CreateSection(void *lpAddress, size_t dwSize, DWORD flProtect) {
	return VirtualAllocEx((HANDLE)dwHandle, lpAddress, dwSize, MEM_RESERVE | MEM_COMMIT | MEM_TOP_DOWN, flProtect);
}

bool ExternMapper::ChangeProtect(void *lpAddress, size_t dwSize, DWORD flProtect) {
	DWORD oldProtect;
	return TRUE == VirtualProtectEx((HANDLE)dwHandle, lpAddress, dwSize, flProtect, &oldProtect);
}

bool ExternMapper::WriteBytes(void *lpAddress, void *lpBuffer, size_t nSize) {
	//memcpy(lpAddress, lpBuffer, nSize);
	SIZE_T wrt;

	if (FALSE == WriteProcessMemory((HANDLE)dwHandle, lpAddress, lpBuffer, nSize, &wrt)) {
		return false;
	}
	return wrt == nSize;
}

/* only works for resolving imports from ntdll */

DWORD ExternMapper::FindImport(const char *moduleName, const char *funcName) {
	HMODULE hModule = GetModuleHandleA(moduleName);

	if (NULL == hModule) {
		return 0xFFFFFFFF;
	}

	return (DWORD)GetProcAddress(hModule, funcName);
}

DWORD ExternMapper::FindImport(const char *moduleName, const unsigned int funcOrdinal) {
	HMODULE hModule = GetModuleHandleA(moduleName);

	if (NULL == hModule) {
		return 0xFFFFFFFF;
	}

	return (DWORD)GetProcAddress(hModule, (LPCSTR)funcOrdinal);
}