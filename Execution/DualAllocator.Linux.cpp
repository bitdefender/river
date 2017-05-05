#ifdef  __linux__

#include "DualAllocator.h"
#include <stdio.h>
#include <errno.h>
#include <algorithm>
#include "../libproc/os-linux.h"

#define DONOTPRINT

#ifdef DONOTPRINT
#define dbg_log(fmt,...) ((void)0)
#else
#define dbg_log(fmt,...) printf(fmt, ##__VA_ARGS__)
#endif

DualAllocator::DualAllocator(DWORD size, PROCESS_HANDLE remoteProcess, const char *shmName, DWORD granularity, DWORD initialOffset) {
	dwSize = size;
	dwUsed = initialOffset;
	dwGran = 0x1000;

	hProcess[0] = GET_CURRENT_PROC();
	hProcess[1] = remoteProcess;

	hMapping = shm_open("/thug_life", O_CREAT | O_RDWR, 0644);
	if (hMapping == -1) {
		printf("[DualAllocator] Could not init shared memory chunk. Exiting. %d\n", errno);
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

void DualAllocator::SetBaseAddress(DWORD baseAddress) {
	//TODO move this in constructor
	this->baseAddress = baseAddress;
}

DWORD DualAllocator::AllocateFixed(DWORD address, DWORD size) {
	printf("[DualAllocator] AllocateFixed(%p, %08x)\n", address, dwUsed);
	void* addr = mmap((void*)address, size,
				PROT_READ | PROT_WRITE | PROT_EXEC, MAP_SHARED, hMapping, dwUsed);
	if (addr == (void *)(-1)) {
		printf("[DualAllocator] mmap failed with error code %d for address %p. Used: %08lx\n", errno, addr, dwUsed);
		return (DWORD)addr;
	}

	// TODO check if there is enough free space in parent! Should be!
	dwUsed += size;
	dwUsed += dwGran - 1;
	dwUsed &= ~(dwGran - 1);

	return (DWORD)addr;
}

void GetMemoryLayout(std::vector<struct map_region> &mrs, pid_t pid) {
	struct map_iterator mi;
	if (maps_init(&mi, pid) < 0) {
		dbg_log("[DualAllocator] Cannot retrieve /proc/%d/maps\n", pid);
		return;
	}

	struct map_prot mp;
	unsigned long low, high, offset;

	dbg_log("[DualAllocator] /proc/%d/maps\n", pid);
	while (maps_next(&mi, &low, &high, &offset, &mp)) {
		dbg_log("[DualAllocator] path : %s base addr : %08lx high %08lx\n", mi.path, low, high);
		struct map_region mr;
		ssize_t len = mrs.size();
		if (len > 1) {
			// add free region before current region
			mr.address = mrs[len - 1].address + mrs[len-1].size;
			mr.size = low - mr.address;
		} else {
			mr.address = 0x0;
			mr.size = low;
		}
		if (mr.size > 0x00) {
			mr.state = FREE;
			mrs.push_back(mr);
			dbg_log("[DualAllocator] Added FREE %08x %08x\n", mr.address, mr.size);
		}

		// add current allocated region
		mr.address = low;
		mr.size = high - low;
		mr.state = ALLOCATED;
		mrs.push_back(mr);
		dbg_log("[DualAllocator] Added ALLOCATED %08x %08x\n", mr.address, mr.size);


	}
	maps_close(&mi);
}

bool VirtualQuery(std::vector<struct map_region> &mrs, uint32_t address,
		struct map_region &mr) {

	for (auto it = mrs.begin(); it != mrs.end(); ++it) {
		if (address < it->address)
			continue;
		if (address >= it->address + it->size)
			continue;
		mr = *it;
		return true;
	}

	//shouldn't reach this point
	printf("[DualAllocator] Could not find memory region for address %08x\n",
			address);
	return false;
}


void *DualAllocator::Allocate(DWORD size, DWORD &offset) {
	printf("[DualAllocator] Looking for a 0x%08lx block\n", size);

	// alignment
	size = (size + 0xFFF) & ~0xFFF;

	offset = 0xFFFFFFFF;
	if (dwUsed == dwSize) {
		printf("[DualAllocator] There is not enough size left. Used %08lx \
				out of %08lx\n", dwUsed, dwSize);
		return NULL;
	}

	offset = dwUsed;
	// now look for a suitable address;

	DWORD dwOffset = baseAddress; // dwGran;
	DWORD dwCandidate = 0, dwCandidateSize = 0xFFFFFFFF;

	std::vector<struct map_region> MemoryLayout[2];

	for (int i = 0; i < 2; i++) {
		GetMemoryLayout(MemoryLayout[i], hProcess[i]);
	}

	while (dwOffset < 0xF7000000) {
		struct map_region mr;
		DWORD regionSize = 0xFFFFFFFF;
		bool regionFree = true;

		for (int i = 0; i < 2; ++i) {
			VirtualQuery(MemoryLayout[i], dwOffset,  mr);

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
		dwOffset += dwGran - 1;
		dwOffset &= ~(dwGran - 1);
	}

	if (0 == dwCandidate) {
		printf("[DualAllocator] Could not find candidate for size %08lx\n", size);
		return NULL;
	}

	printf("[DualAllocator] Trying to allocate %08lx at address %08lx\n",
			size, dwCandidate);
	void *ptr = (void *)AllocateFixed(dwCandidate, size);
	printf("[DualAllocator] Allocated addr @%p of size %08lx\n", ptr, size);

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

#endif //  __linux__