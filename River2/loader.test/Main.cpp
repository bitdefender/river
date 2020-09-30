#include "../loader/loader.h"
#include <Windows.h>
#include <stdio.h>

#include "Loader/PE.ldr.h"
#include "Loader/Mem.Mapper.h"

int main() {
	HANDLE hMapping = CreateFileMappingA(
		INVALID_HANDLE_VALUE,
		NULL,
		PAGE_EXECUTE_READWRITE,
		0,
		0x508000,
		"Global\\Mumbojumbo"
	);

	if (NULL == hMapping) {
		printf("CreateFileMapping() error %d\n", GetLastError());
	}
	
	void *map = MapViewOfFileEx(
		hMapping,
		FILE_MAP_ALL_ACCESS,
		0,
		0,
		0,
		NULL
	);

	FloatingPE fpe("revtracer.dll");
	MemMapper mpr;

	DWORD dwAddr = (DWORD)map;
	fpe.MapPE(mpr, dwAddr);
	
	
	ldr::LoaderConfig *lCfg = &ldr::loaderConfig;
	wcscpy_s(lCfg->sharedMemoryName, L"Global\\Mumbojumbo");
	lCfg->mappingAddress = (ldr::ADDR_TYPE)(dwAddr + 0x1000000);
	lCfg->entryPoint = 0;

	ldr::LoaderAPI *lAPI = &ldr::loaderAPI;
	HMODULE hNtDll = GetModuleHandle("ntdll.dll");
	lAPI->ntOpenSection = GetProcAddress(hNtDll, "NtOpenSection");
	lAPI->ntMapViewOfSection = GetProcAddress(hNtDll, "NtMapViewOfSection");

	lAPI->ntOpenDirectoryObject = GetProcAddress(hNtDll, "NtOpenDirectoryObject");
	lAPI->ntClose = GetProcAddress(hNtDll, "NtClose");

	lAPI->rtlInitUnicodeStringEx = GetProcAddress(hNtDll, "RtlInitUnicodeStringEx");
	lAPI->rtlFreeUnicodeString = GetProcAddress(hNtDll, "RtlFreeUnicodeString");

	ldr::LoaderPerform();

	

	UnmapViewOfFile(map);
	CloseHandle(hMapping);

	return 0;
}