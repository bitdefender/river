#include "loader.h"
#include <sys/mman.h>
#include <sys/stat.h>        /* For mode constants */
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <assert.h>
#include <vector>
#include <string>
#include "../BinLoader/LoaderAPI.h"

#define DEBUG_BREAK asm volatile("int $0x3")

#define MAX_LIBS 10
#define LIB_IPC_INDEX 0
#define LIB_PTHREAD_INDEX 1
#define LIB_REV_WRAPPER_INDEX 2
#define LIB_REVTRACER_INDEX 3

typedef int(*RevWrapperInitCallback)(DWORD, DWORD);
typedef void *(*CallMapMemoryCallback)(unsigned long mapHandler, unsigned long access, unsigned long offset, unsigned long size, void *address);

static void init(void) __attribute__((constructor));
static void destroy(void) __attribute__((destructor));

struct mappedObject {
	MODULE_PTR module;
	BASE_PTR base;
	DWORD size;
};

unsigned long sharedMemoryAdress = 0x0;

mappedObject mos[MAX_LIBS];

int shmFd;

CallMapMemoryCallback mapMemory;

// Do not use library dependent code here!
extern "C" {
	void *MapMemory(unsigned long access, unsigned long offset, unsigned long size, void *address) {
		return mapMemory((unsigned long)shmFd, access, offset, size, address);
	}
}

unsigned long FindFreeVirtualMemory(int shmFd, DWORD size) {
	void *addr = mmap(0, size, PROT_READ | PROT_WRITE | PROT_EXEC,
			MAP_SHARED, shmFd, 0);
	munmap(addr, size);
	return (unsigned long)addr;
}

int  InitializeAllocator() {
	shmFd = shm_open("/thug_life", O_CREAT | O_RDWR | O_TRUNC | O_EXCL, 0644);
	if (shmFd < 0) {
		printf("[Child] Could not allocate shared memory chunk. Exiting.\n");
		return -1;
	}

	int ret = ftruncate(shmFd, 1 << 30);
	return shmFd;
}

unsigned long MapSharedLibraries(int shmFd) {
	//
	// Libs are mapped in the following order:
	//  libipc | libc | libpthread | librevwrapper | revtracer
	//

	std::vector<std::string> libNames = {"libipc.so", "libpthread.so", "librevtracerwrapper.so", "revtracer.dll"};

	for (int i = 0; i < libNames.size(); ++i) {
		CreateModule(libNames[i].c_str(), mos[i].module);
		if (!mos[i].module) {
			printf("[Child] Could not map %s lib in shm\n", libNames[i].c_str());
			return -1;
		}
	}

	DWORD dwTotalSize = 0;
	for (int i = 0; i < libNames.size(); ++i) {
		mos[i].size = (mos[i].module->GetRequiredSize() + 0xFFF) & ~0xFFFF;
		dwTotalSize += mos[i].size;
	}

	mos[0].base = FindFreeVirtualMemory(shmFd, 1 << 30);

	for (int i = 1; i < libNames.size(); ++i) {
		mos[i].base = mos[i - 1].base + mos[i - 1].size;
	}

	DWORD offset = 0;
	for (int i = 0; i < libNames.size(); ++i) {
		MapModule(mos[i].module, mos[i].base, shmFd, offset);
		offset += mos[i].size;
	}

	assert(offset == dwTotalSize);

	for (int i = 0; i < libNames.size(); ++i) {
		printf("[Child] Mapped library %s at address %08lx\n", libNames[i].c_str(), (DWORD)mos[i].base);
	}

	RevWrapperInitCallback initRevWrapper;
	LoadExportedName(mos[LIB_REV_WRAPPER_INDEX].module, mos[LIB_REV_WRAPPER_INDEX].base, "InitRevtracerWrapper", initRevWrapper);
	printf("[Child] Found initRevWrapper address @%p\n", (void *)initRevWrapper);
	if (initRevWrapper(0, mos[LIB_PTHREAD_INDEX].base) == -1) {
		printf("[Child] Could not find revwrapper needed libraries\n");
		return 0x0;
	} else {
		printf("[Child] Revwrapper init returned successfully\n");
	}

	LoadExportedName(mos[LIB_REV_WRAPPER_INDEX].module, mos[LIB_REV_WRAPPER_INDEX].base, "CallMapMemoryHandler", mapMemory);
	printf("[Child] Found mapMemory handler at address %08lx\n", (unsigned long)mapMemory);
	return mos[LIB_IPC_INDEX].base;
}

void init() {
	InitializeAllocator();
	sharedMemoryAdress = MapSharedLibraries(shmFd);
	if ((int)sharedMemoryAdress == -1)
		printf("[Child] Failed to map the shared mem\n");
	printf("[Child] Shared mem address is %p. Fd is [%d]\n", (void*)sharedMemoryAdress, shmFd);
	fflush(stdout);

}

void destroy() {
	shm_unlink("/thug_life");
}
