#include <sys/mman.h>
#include <sys/stat.h>        /* For mode constants */
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include "../BinLoader/LoaderAPI.h"

#define DEBUG_BREAK asm volatile("int $0x3")

typedef int(*RevWrapperInitCallback)(void);
static void init(void) __attribute__((constructor));
static void destroy(void) __attribute__((destructor));

unsigned long sharedMemoryAdress = 0x0;

MODULE_PTR hIpcModule;
BASE_PTR hIpcBase;

MODULE_PTR hRevtracerModule;
BASE_PTR hRevtracerBase;

MODULE_PTR hRevWrapperModule;
BASE_PTR hRevWrapperBase;

unsigned long FindFreeVirtualMemory(int shmFd, DWORD size) {
	void *addr = mmap(0, size, PROT_READ | PROT_WRITE | PROT_EXEC,
			MAP_SHARED, shmFd, 0);
	munmap(addr, 1 << 30);
	return (unsigned long)addr;
}

int  InitializeAllocator() {
	int fd = shm_open("/thug_life", O_CREAT | O_RDWR | O_TRUNC, 0644);
	if (fd < 0) {
		printf("[Child] Could not allocate shared memory chunk. Exiting.\n");
		return -1;
	}

	int ret = ftruncate(fd, 1 << 30);
	fflush(stdout);
	return fd;
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

	hIpcBase = FindFreeVirtualMemory(shmFd, dwTotalSize);

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
	return hIpcBase;
}

void init() {
	int fd = InitializeAllocator();
	sharedMemoryAdress = MapSharedLibraries(fd);
	if ((int)sharedMemoryAdress == -1)
		printf("[Child] Failed to map the shared mem\n");
	printf("[Child] Shared mem address is %p\n", (void*)sharedMemoryAdress);
	fflush(stdout);

}

void destroy() {
	shm_unlink("/thug_life");
}
