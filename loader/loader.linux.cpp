#include "loader.h"
#include <sys/mman.h>
#include <sys/stat.h>        /* For mode constants */
#include <asm/ldt.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <assert.h>
#include <string.h>
#include <errno.h>

#include <vector>
#include <string>

#define DEBUG_BREAK asm volatile("int $0x3")

namespace ldr {

#define LIB_IPC_INDEX 0
#define LIB_PTHREAD_INDEX 1
#define LIB_REV_WRAPPER_INDEX 2
#define LIB_REVTRACER_INDEX 3
#define __NR_get_thread_area 244

typedef int(*RevWrapperInitCallback)(DWORD, DWORD);
typedef void *(*CallMapMemoryCallback)(unsigned long mapHandler, unsigned long access, unsigned long offset, unsigned long size, void *address);

static void init(void) __attribute__((constructor));
static void destroy(void) __attribute__((destructor));

struct LoaderAPI loaderAPI;
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

void InitSegmentDescriptors() {
	struct user_desc* table_entry_ptr = NULL;

	table_entry_ptr = (struct user_desc*)malloc(sizeof(struct user_desc));

	for (int i = 0; i < 0x100; ++i) {
		table_entry_ptr->entry_number = i;
		int ret = syscall( __NR_get_thread_area,
				table_entry_ptr);
		if (ret == -1 && errno == EINVAL) {
			loaderAPI.segments[i] = 0xFFFFFFFF;
		} else if (ret == 0) {
			loaderAPI.segments[i] = table_entry_ptr->base_addr;
		} else {
			printf("[Child] Error found when get_thread_area. errno %d\n", errno);
		}
	}
	free(table_entry_ptr);
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
		CreateModule(libNames[i].c_str(), loaderAPI.mos[i].module);
		if (!loaderAPI.mos[i].module) {
			printf("[Child] Could not map %s lib in shm\n", libNames[i].c_str());
			return -1;
		}
		loaderAPI.mos[i].base = 0x0;
	}

	DWORD dwTotalSize = 0;
	for (int i = 0; i < libNames.size(); ++i) {
		loaderAPI.mos[i].size = (loaderAPI.mos[i].module->GetRequiredSize() + 0xFFFF) & ~0xFFFF;
		dwTotalSize += loaderAPI.mos[i].size;
	}

	loaderAPI.mos[0].base = FindFreeVirtualMemory(shmFd, 1 << 30);
	printf("[Child] Mapped shared memory base address @%08lx\n", loaderAPI.mos[0].base);

	for (int i = 1; i < libNames.size(); ++i) {
		loaderAPI.mos[i].base = loaderAPI.mos[i - 1].base + loaderAPI.mos[i - 1].size;
	}

	DWORD offset = 0;
	for (int i = 0; i < libNames.size(); ++i) {
		MapModule(loaderAPI.mos[i].module, loaderAPI.mos[i].base, shmFd, offset);
		offset += loaderAPI.mos[i].size;
	}

	assert(offset == dwTotalSize);

	for (int i = 0; i < libNames.size(); ++i) {
		printf("[Child] Mapped library %s at address %08lx\n", libNames[i].c_str(), (DWORD)loaderAPI.mos[i].base);
	}

	RevWrapperInitCallback initRevWrapper;
	LoadExportedName(loaderAPI.mos[LIB_REV_WRAPPER_INDEX].module, loaderAPI.mos[LIB_REV_WRAPPER_INDEX].base, "InitRevtracerWrapper", initRevWrapper);
	printf("[Child] Found initRevWrapper address @%p\n", (void *)initRevWrapper);
	if (initRevWrapper(0, loaderAPI.mos[LIB_PTHREAD_INDEX].base) == -1) {
		printf("[Child] Could not find revwrapper needed libraries\n");
		return 0x0;
	} else {
		printf("[Child] Revwrapper init returned successfully\n");
	}

	LoadExportedName(loaderAPI.mos[LIB_REV_WRAPPER_INDEX].module, loaderAPI.mos[LIB_REV_WRAPPER_INDEX].base, "CallMapMemoryHandler", mapMemory);
	printf("[Child] Found mapMemory handler at address %08lx\n", (unsigned long)mapMemory);
	return loaderAPI.mos[LIB_IPC_INDEX].base;
}

void init() {
	InitializeAllocator();
	loaderAPI.sharedMemoryAddress = MapSharedLibraries(shmFd);
	InitSegmentDescriptors();

	if ((int)loaderAPI.sharedMemoryAddress == -1)
		printf("[Child] Failed to map the shared mem\n");
	printf("[Child] Shared mem address is %p. Fd is [%d]\n", (void*)loaderAPI.sharedMemoryAddress, shmFd);
	fflush(stdout);
}

void destroy() {
	shm_unlink("/thug_life");
}

}; //namespace ldr
