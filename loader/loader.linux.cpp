#include <sys/mman.h>
#include <sys/stat.h>        /* For mode constants */
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>


static void init(void) __attribute__((constructor));
static void destroy(void) __attribute__((destructor));

unsigned long sharedMemoryAdress = 0x0;

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

void init() {
	int fd = InitializeAllocator();
	sharedMemoryAdress = (unsigned long)mmap(0, 1 << 30, PROT_READ | PROT_WRITE | PROT_EXEC, MAP_SHARED, fd, 0);
	if ((int)sharedMemoryAdress == -1)
		printf("[Child] Failed to map the shared mem\n");
	printf("[Child] Shared mem address is %p\n", (void*)sharedMemoryAdress);
	fflush(stdout);
}

void destroy() {
	shm_unlink("/thug_life");
}
