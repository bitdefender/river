#include "DualAllocator.h"
#include <stdio.h>
#include <errno.h>
#include <algorithm>
#include "../libproc/os-linux.h"

DualAllocator::DualAllocator(DWORD size, PROCESS_HANDLE remoteProcess, const char *shmName, DWORD granularity) {
	dwSize = size;
	dwUsed = 0;

	hProcess[0] = GET_CURRENT_PROC();
	hProcess[1] = remoteProcess;

	hMapping = shm_open("/thug_life", O_CREAT | O_RDWR | O_TRUNC | O_EXCL, 0644);
	if (hMapping == -1) {
		printf("[DualAllocator] Could not allocate shared memory chunk. Exiting. %d\n", errno);
	}
}

DualAllocator::~DualAllocator() {
	for (auto it = mappedViews.begin(); it < mappedViews.end(); ++it) {
		munmap(it->first, it->second);
	}

	shm_unlink("/thug_life");
}

HANDLE DualAllocator::CloneTo(PROCESS_HANDLE process) {
	// TODO do we need this ?
	return nullptr;
}

DWORD DualAllocator::AllocateFixed(DWORD size, DWORD address) {
	// TODO check if there is enough free space in parent! Should be!
	return (unsigned long)mmap((void*)address, size,
				PROT_READ | PROT_WRITE | PROT_EXEC, MAP_SHARED, hMapping, 0);
}

void GetMemoryLayout(std::vector<struct map_region> &mrs, pid_t pid) {
	struct map_iterator mi;
	if (!maps_init(&mi, pid))
		return;

	struct map_prot mp;
	unsigned long low, high, offset;

	while (!maps_next(&mi, &low, &high, &offset, &mp)) {

		struct map_region mr;
		ssize_t len = mrs.size();
		if (len > 1) {
			// add free region before current region
			mr.address = mrs[len - 1].address + mrs[len-1].size;
			mr.size = low = mr.address;
			mr.state = FREE;
			mrs.push_back(mr);
		}

		// add current allocated region
		mr.address = low;
		mr.size = high - low;
		mr.state = ALLOCATED;
		mrs.push_back(mr);


	}
	maps_close(&mi);
}

bool VirtualQuery(std::vector<struct map_region> &mrs, uint32_t address,
		struct map_region &mr) {

	for (auto it = mrs.begin(); it != mrs.end(); ++it) {
		if (address < it->address)
			continue;
		if (address > it->address + it->size)
			continue;
		mr = *it;
		return true;
	}
	return false;
}


void *DualAllocator::Allocate(DWORD size, DWORD &offset) {
	printf("Looking for a 0x%08lx block\n", size);

	// alignment
	size = (size + 0xFFF) & ~0xFFF;

	offset = 0xFFFFFFFF;
	if (dwUsed == dwSize) {
		printf("[DualAllocator] There is not enough size left. Used %08lx \
				out of %08lx\n", dwUsed, dwSize);
		return NULL;
	}
	offset = dwUsed;
	dwUsed += size;

	// now look for a suitable address;

	DWORD dwOffset = 0x01000000; // dwGran;
	DWORD dwCandidate = 0, dwCandidateSize = 0xFFFFFFFF;

	std::vector<struct map_region> MemoryLayout[2];

	for (int i = 0; i < 2; i++) {
		GetMemoryLayout(MemoryLayout[i], hProcess[i]);
	}

	while (dwOffset < 0x2FFF0000) {
		struct map_region mr;
		DWORD regionSize = 0xFFFFFFFF;
		bool regionFree = true;

		for (int i = 0; i < 2; ++i) {
			if (!VirtualQuery(MemoryLayout[i], dwOffset,  mr))
				return nullptr;

			DWORD dwSize = mr.size - (dwOffset - mr.address); // or allocationbase
			if (regionSize > dwSize) {
				regionSize = dwSize;
			}

			//printf("        Proc %d offset: 0x%08x, size 0x%08x\n", i, dwOffset, dwSize);

			regionFree &= (FREE == mr.state);
		}

		if (regionFree & (regionSize >= size) & (regionSize < dwCandidateSize)) {
			printf("    Candidate found @0x%08lx size 0x%08lx\n", dwOffset, regionSize);
			dwCandidate = dwOffset;
			dwCandidateSize = regionSize;

			if (regionSize == size) {
				break;
			}
		}

		dwOffset += regionSize;
		//dwOffset += dwGran - 1;
		//dwOffset &= ~(dwGran - 1);
	}

	if (0 == dwCandidate) {
		return NULL;
	}

	void *ptr = (void *)AllocateFixed(dwCandidate, size);

	if (dwCandidate != (DWORD)ptr) {
		DEBUG_BREAK;
	}

	mappedViews.push_back(std::pair<FileView, DWORD>(ptr, size));
	return ptr;
}

void DualAllocator::Free(void *ptr) {
	for (auto it = mappedViews.begin(); it != mappedViews.end(); ++it) {
		if (it->first == ptr) {
			munmap(ptr, it->second);
			mappedViews.erase(it);
			return;
		}
	}
}
