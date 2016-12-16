#include "loader.h"
#include <sys/mman.h>
#include <sys/stat.h>        /* For mode constants */
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include "../BinLoader/LoaderAPI.h"

#define DEBUG_BREAK asm volatile("int $0x3")

typedef int(*RevWrapperInitCallback)(void);
typedef void *(*CallMapMemoryCallback)(unsigned long mapHandler, unsigned long access, unsigned long offset, unsigned long size, void *address);
static void init(void) __attribute__((constructor));
static void destroy(void) __attribute__((destructor));

unsigned long sharedMemoryAdress = 0x0;

MODULE_PTR hIpcModule;
BASE_PTR hIpcBase;

MODULE_PTR hRevtracerModule;
BASE_PTR hRevtracerBase;

MODULE_PTR hRevWrapperModule;
BASE_PTR hRevWrapperBase;

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
	CreateModule(L"libipc.so", hIpcModule);
	CreateModule(L"librevtracerwrapper.so", hRevWrapperModule);
	CreateModule(L"revtracer.dll", hRevtracerModule);

	if (!hIpcModule || !hRevWrapperModule || !hRevtracerModule) {
		printf("[Child] Could not map libraries in shared memory\n");
		return 0;
	}

	DWORD dwIpcLibSize = (hIpcModule->GetRequiredSize() + 0xFFFF) & ~0xFFFF;
	DWORD dwRevWrapperSize = (hRevWrapperModule->GetRequiredSize() + 0xFFFF) & ~0xFFFF;
	DWORD dwRevTracerSize = (hRevtracerModule->GetRequiredSize() + 0xFFFF) & ~0xFFFF;
	DWORD dwTotalSize = dwIpcLibSize + dwRevWrapperSize + dwRevTracerSize;

	hIpcBase = FindFreeVirtualMemory(shmFd, 1 << 30);

	MapModule(hIpcModule, hIpcBase, shmFd, 0);

	hRevWrapperBase = hIpcBase + dwIpcLibSize;
	hRevtracerBase = hRevWrapperBase + dwRevWrapperSize;

	MapModule(hRevWrapperModule, hRevWrapperBase, shmFd, dwIpcLibSize);
	MapModule(hRevtracerModule, hRevtracerBase, shmFd, dwIpcLibSize + dwRevWrapperSize);

	printf("[Child] Mapped ipclib@0x%08lx revtracer@0x%08lx and revwrapper@0x%08lx\n",
			(DWORD)hIpcBase, (DWORD)hRevtracerBase,
			(DWORD)hRevWrapperBase);

	RevWrapperInitCallback initRevWrapper;
	LoadExportedName(hRevWrapperModule, hRevWrapperBase, "InitRevtracerWrapper", initRevWrapper);
	if (initRevWrapper() == -1) {
		printf("[Child] Could not find revwrapper needed libraries\n");
		return 0x0;
	} else {
		printf("[Child] Revwrapper init returned successfully\n");
	}

	LoadExportedName(hRevWrapperModule, hRevWrapperBase, "CallMapMemoryHandler", mapMemory);
	printf("[Child] Found mapMemory handler at address %08lx\n", (unsigned long)mapMemory);
	return hIpcBase;
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
