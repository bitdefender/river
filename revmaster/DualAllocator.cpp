#include "DualAllocator.h"

DualAllocator::DualAllocator(DWORD size, HANDLE remoteProcess, const wchar_t *shmName, DWORD granularity) {
	hMapping = CreateFileMappingW(
		INVALID_HANDLE_VALUE,
		NULL,
		PAGE_EXECUTE_READWRITE,
		0,
		size,
		shmName
		);

	dwSize = size;
	dwUsed = 0;
	dwGran = granularity;

	hProcess[0] = GetCurrentProcess();
	hProcess[1] = remoteProcess;
}

DualAllocator::~DualAllocator() {
	CloseHandle(hMapping);
}

#include <stdio.h>
void *DualAllocator::Allocate(DWORD size, DWORD &offset) {
	printf("Looking for a 0x%08x block\n", size);

	size = (size + 0xFFF) & ~0xFFF;

	offset = 0xFFFFFFFF;
	if (dwUsed == dwSize) {
		return NULL;
	}
	offset = dwUsed;
	dwUsed += size;
	dwUsed += dwGran - 1;
	dwUsed &= ~(dwGran - 1);

	// now look for a suitable address;

	DWORD dwOffset = dwGran;
	DWORD dwCandidate = 0, dwCandidateSize = 0xFFFFFFFF;

	while (dwOffset < 0x7FFF0000) {
		MEMORY_BASIC_INFORMATION32 mbi;
		DWORD regionSize = 0xFFFFFFFF;
		bool regionFree = true;

		for (int i = 0; i < 2; ++i) {
			if (0 == VirtualQueryEx(hProcess[i], (LPCVOID)dwOffset, (PMEMORY_BASIC_INFORMATION)&mbi, sizeof(mbi))) {
				return NULL;
			}

			DWORD dwSize = mbi.RegionSize - (dwOffset - mbi.BaseAddress); // or allocationbase
			if (regionSize > dwSize) {
				regionSize = dwSize;
			}

			regionFree &= (MEM_FREE == mbi.State);
		}

		if (regionFree & (regionSize >= size) & (regionSize < dwCandidateSize)) {
			printf("    Candidate found @0x%08x size 0x%08x\n", dwOffset, regionSize);
			dwCandidate = dwOffset;
			dwCandidateSize = regionSize;

			if (regionSize == size) {
				break;
			}
		}

		dwOffset += regionSize;
		dwOffset += dwGran - 1;
		dwOffset &= ~(dwGran - 1);
	}

	if (0 == dwCandidate) {
		return NULL;
	}

	void *ptr = MapViewOfFileEx(
		hMapping,
		FILE_MAP_ALL_ACCESS,
		0,
		offset,
		size,
		(void *)dwCandidate
		);

	if (dwCandidate != (DWORD)ptr) {
		__asm int 3;
	}

	return ptr;
}

void DualAllocator::Free(void *ptr) {
	UnmapViewOfFile(ptr);
}