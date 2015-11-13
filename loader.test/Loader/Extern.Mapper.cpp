#include "Extern.Mapper.h"

#include <psapi.h>

bool SameFile(const char *s1, int l1, const char *s2, int l2) {
	/*int cLen = l1;
	if (l2 < cLen) cLen = l2;

	int o1 = l1 - cLen, o2 = l2 - cLen;

	for (int i = 0; i < cLen; ++i) {
		if (toupper(s1[o1 + i]) != toupper(s2[o2 + i])) {
			return false;
		}
	}

	if ((o1 > 0) && (s1[o1 - 1] != '\\')) {
		return false;
	}

	if ((o2 > 0) && (s2[o2 - 1] != '\\')) {
		return false;
	}

	return true;*/

	if (l1 != l2) {
		return false;
	}

	for (int i = 0; i < l1; ++i) {
		if (toupper(s1[i]) == toupper(s2[i])) {
			return false;
		}
	}

	return true;
}

HMODULE ExternMapper::RemoteFindModule(const char *module) {
	HMODULE hMods[1024];
	DWORD cbNeeded;

	int mLen = strlen(module);

	if (EnumProcessModules(hProc, hMods, sizeof(hMods), &cbNeeded)) {
		for (unsigned int i = 0; i < (cbNeeded / sizeof(HMODULE)); i++) {
			char szModName[MAX_PATH];
			if (GetModuleFileNameExA(hProc, hMods[i], szModName, sizeof(szModName))) {
				int rLen = strlen(szModName);
				
				if (SameFile(module, mLen, szModName, rLen)) {
					return hMods[i];
				}
			}
		}
	}

	return NULL;
}

ExternMapper::ExternMapper(unsigned int pid) {
	hProc = OpenProcess(
		PROCESS_QUERY_INFORMATION | PROCESS_VM_READ | PROCESS_VM_WRITE,
		FALSE,
		pid
	);

	ownProcess = true;
}

ExternMapper::ExternMapper(HANDLE process) {
	hProc = process;
	ownProcess = false;
}

ExternMapper::~ExternMapper() {
	if (ownProcess) {
		CloseHandle(hProc);
	}
}

void *ExternMapper::CreateSection(void *lpAddress, size_t dwSize, DWORD flProtect) {
	return VirtualAllocEx(hProc, lpAddress, dwSize, MEM_RESERVE | MEM_COMMIT, flProtect);
}

bool ExternMapper::ChangeProtect(void *lpAddress, size_t dwSize, DWORD flProtect) {
	DWORD oldProtect;
	return TRUE == VirtualProtectEx(hProc, lpAddress, dwSize, flProtect, &oldProtect);
}

bool ExternMapper::WriteBytes(void *lpAddress, void *lpBuffer, size_t nSize) {
	return TRUE == WriteProcessMemory(hProc, lpAddress, lpBuffer, nSize, NULL);
}

/* This is very sloppy! */
/* Should work for ntdll. No WOW64 support. No export forwarding. */
DWORD ExternMapper::FindImport(const char *moduleName, const char *funcName) {
	HMODULE rMod = RemoteFindModule(moduleName);
	HMODULE lMod = GetModuleHandleA(moduleName);

	if ((NULL == rMod) || (NULL == lMod)) {
		return 0xFFFFFFFF;
	}

	void *pFunc = GetProcAddress(lMod, funcName);
	return (DWORD)pFunc - (DWORD)lMod + (DWORD)rMod;
}

DWORD ExternMapper::FindImport(const char *moduleName, const unsigned int funcOrdinal) {
	HMODULE lMod = GetModuleHandleA(moduleName);
	char modName[MAX_PATH];

	GetModuleFileNameA(lMod, modName, sizeof(modName));
	HMODULE rMod = RemoteFindModule(modName);
	

	if ((NULL == rMod) || (NULL == lMod)) {
		return 0xFFFFFFFF;
	}

	void *pFunc = GetProcAddress(lMod, (LPCSTR)funcOrdinal);
	return (DWORD)pFunc - (DWORD)lMod + (DWORD)rMod;
}