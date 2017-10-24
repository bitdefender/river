#ifdef __linux__
#include "Loader.h"
#include <sys/mman.h>
#include <sys/stat.h>        /* For mode constants */
#include <asm/ldt.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <assert.h>
#include <string.h>
#include <errno.h>
#include <link.h>

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

//static void loaderInit(void) __attribute__((constructor));
static void destroy(void) __attribute__((destructor));

//struct LoaderAPI loaderAPI;
LoaderConfig loaderConfig;

// Do not use library dependent code here!
void *MapMemory(unsigned long access, unsigned long offset, unsigned long size, void *address) {
	//return mapMemory(loaderConfig.sharedMemory, access, offset, size, address);

	return mmap(address, size, access, MAP_SHARED, loaderConfig.sharedMemory, offset);
}

extern "C" {
	void patch__rtld_global_ro();
}

unsigned long FindFreeVirtualMemory(int shmFd, DWORD size) {
	void *addr = mmap(0, size, PROT_READ | PROT_WRITE | PROT_EXEC, MAP_SHARED, shmFd, 0);

	if (MAP_FAILED == addr) {
		printf("[Child] Couldn't find free virtual memory; errno %d\n", errno);
		return (unsigned long)addr;
	}

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
			loaderConfig.osData.linux.segments[i] = 0xFFFFFFFF;
		} else if (ret == 0) {
			loaderConfig.osData.linux.segments[i] = table_entry_ptr->base_addr;
		} else {
			printf("[Child] Error found when get_thread_area. errno %d\n", errno);
		}
	}
	free(table_entry_ptr);
}

int  InitializeAllocator() {
	loaderConfig.sharedMemory = shm_open("/thug_life", O_CREAT | O_RDWR | O_TRUNC | O_EXCL, 0644);
	if (loaderConfig.sharedMemory < 0) {
		printf("[Child] Could not allocate shared memory chunk. Exiting.\n");
		return -1;
	}

	int ret = ftruncate(loaderConfig.sharedMemory, 1 << 30);
	return loaderConfig.sharedMemory;
}

static int FindDependencies(struct dl_phdr_info *info, size_t size, void *data) {
	ext::LibraryLayout *libs = (ext::LibraryLayout *)data;
	//printf("[Child] module name=\"%s\" @ %08x\n", info->dlpi_name, info->dlpi_addr);

	const char *p = strrchr(info->dlpi_name, '/');
	p = (nullptr == p) ? info->dlpi_name : (p + 1);

	if (0 == strncmp("libc.so", p, 7)) {
		printf("Found libc @ %08x\n", info->dlpi_addr);
		libs->linLib.libcBase = info->dlpi_addr;
	} else if (0 == strncmp("librt.so", p, 8)) {
		printf("Found librt @ %08x\n", info->dlpi_addr); 
		libs->linLib.librtBase = info->dlpi_addr;
	} else if(0 == strncmp("libpthread.so", p, 13)) {
		printf("Found libpthread @ %08x\n", info->dlpi_addr);
		libs->linLib.libpthreadBase = info->dlpi_addr;
	}

	return 0;
}


unsigned long MapSharedLibraries(int shmFd) {
	//
	// Libs are mapped in the following order:
	//  librevwrapper | libipc | revtracer
	//

	std::vector<std::string> libNames = { "librevtracerwrapper.so", "libipc.so", "revtracer.dll"};

	for (int i = 0; i < libNames.size(); ++i) {
		CreateModule(libNames[i].c_str(), loaderConfig.osData.linux.mos[i].module);
		if (!loaderConfig.osData.linux.mos[i].module) {
			printf("[Child] Could not map %s lib in shm\n", libNames[i].c_str());
			return -1;
		}
		loaderConfig.osData.linux.mos[i].base = 0x0;
	}

	DWORD dwTotalSize = 0;
	for (int i = 0; i < libNames.size(); ++i) {
		loaderConfig.osData.linux.mos[i].size = (loaderConfig.osData.linux.mos[i].module->GetRequiredSize() + 0xFFFF) & ~0xFFFF;
		dwTotalSize += loaderConfig.osData.linux.mos[i].size;
	}

	loaderConfig.osData.linux.mos[0].base = FindFreeVirtualMemory(shmFd, 1 << 30);
	printf("[Child] Mapped shared memory base address @%08lx\n", loaderConfig.osData.linux.mos[0].base);

	for (int i = 1; i < libNames.size(); ++i) {
		loaderConfig.osData.linux.mos[i].base = loaderConfig.osData.linux.mos[i - 1].base + loaderConfig.osData.linux.mos[i - 1].size;
	}

	DWORD offset = 0;
	for (int i = 0; i < libNames.size(); ++i) {
		bool callConstructors = false;
		if (libNames[i].compare("libpthread.so") == 0)
			callConstructors = true;

		MapModule(loaderConfig.osData.linux.mos[i].module, loaderConfig.osData.linux.mos[i].base, callConstructors, shmFd, offset);
		offset += loaderConfig.osData.linux.mos[i].size;
	}

	assert(offset == dwTotalSize);

	for (int i = 0; i < libNames.size(); ++i) {
		printf("[Child] Mapped library %s at address %08lx\n", libNames[i].c_str(), (DWORD)loaderConfig.osData.linux.mos[i].base);
	}

	loaderConfig.shmBase = (ADDR_TYPE)loaderConfig.osData.linux.mos[0].base;

	// Control is haded back to the parent
	// The parent uses loaderConfig.shmBase to load the so's, and fix all dependencies
	// The parent also sets loaderImports.libraries (a private address equal in both processes)
	DEBUG_BREAK;

	if (MAP_FAILED == mmap(loaderImports.libraries, sizeof(*loaderImports.libraries), PROT_READ | PROT_WRITE, MAP_ANONYMOUS | MAP_PRIVATE, -1, 0)) {
		printf("[Child] Couldn't map loaderImports.libraries; errno %d\n", errno);
	}

	dl_iterate_phdr(FindDependencies, loaderImports.libraries);

	/*RevWrapperInitCallback initRevWrapper;
	LoadExportedName(loaderConfig.osData.linux.mos[LIB_REV_WRAPPER_INDEX].module, loaderConfig.osData.linux.mos[LIB_REV_WRAPPER_INDEX].base, "InitRevtracerWrapper", initRevWrapper);
	printf("[Child] Found initRevWrapper address @%p\n", (void *)initRevWrapper);
	if (initRevWrapper(0, loaderConfig.osData.linux.mos[LIB_PTHREAD_INDEX].base) == -1) {
		printf("[Child] Could not find revwrapper needed libraries\n");
		return 0x0;
	} else {
		printf("[Child] Revwrapper init returned successfully\n");
	}

	LoadExportedName(loaderConfig.osData.linux.mos[LIB_REV_WRAPPER_INDEX].module, loaderConfig.osData.linux.mos[LIB_REV_WRAPPER_INDEX].base, "CallMapMemoryHandler", mapMemory);
	printf("[Child] Found mapMemory handler at address %08lx\n", (unsigned long)mapMemory);*/
	return loaderConfig.osData.linux.mos[LIB_IPC_INDEX].base;
}

extern "C" {
	DWORD miniStack[65536];
	DWORD shadowStack = (DWORD)&(miniStack[65532]);


	void LoaderInit(ADDR_TYPE jumpAddr) {
		loaderConfig.osData.linux.retAddr = jumpAddr;
		InitializeAllocator();
		MapSharedLibraries(loaderConfig.sharedMemory);
		InitSegmentDescriptors();

		//entryPoint = Returnaddres();
		//__asm int 3;

		if ((int)loaderConfig.shmBase == -1) {
			printf("[Child] Failed to map the shared mem\n");
		}
		printf("[Child] Shared mem address is %p. Fd is [%ld]\n", (void*)loaderConfig.shmBase, loaderConfig.sharedMemory);
		//fflush(stdout);

		// disable sse mmx
		//patch__rtld_global_ro();
	}
};

/*void init() {
	
	__asm__(
		"xchgl %0, %%esp             \n\t"
		"pushal                      \n\t"
		"pushfl                      \n\t"
		"call %P1                    \n\t"
		"popfl                       \n\t"
		"popal                       \n\t"
		"xchgl %0, %%esp" : : "m" (shadowStack), "i" (LoaderInit)
	);
	__asm__(
		"jmp *%0" : : "m" (loaderConfig.entryPoint)
	);
}*/

void destroy() {
	shm_unlink("/thug_life");
}

LoaderImports loaderImports;

LoaderExports loaderExports = {
	MapMemory,
	nullptr
};

}; //namespace ldr


#endif
