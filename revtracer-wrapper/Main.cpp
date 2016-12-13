#include "RevtracerWrapper.h"
#include <stdio.h>
#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/stat.h>


// Used for revtracer wrapper local tests
//
int TestCallMapMemoryHandler_basic() {
	void *address = (void*)0;
	void *addr = revwrapper::CallMapMemoryHandler(0, 0, 0, 1000, address);
	printf("[TestCallMapMemoryHandler_basic] Result [%p]\n", addr);
	return addr == (void*)-1;
}

int TestCallMapMemoryHandler_valid_shm() {
	int shmFd = shm_open("/test-valid-shm", O_CREAT | O_RDWR | O_TRUNC | O_EXCL, 0644);
	void *address = (void*)0xb7aab000;
	void *addr = revwrapper::CallMapMemoryHandler(shmFd, 0, 0, 1000, address);
	shm_unlink("/test-valid-shm");
	printf("[TestCallMapMemoryHandler_valid_shm] Result [%p]\n", addr);
	return addr != (void*)-1;
}

int main () {

  int ret = revwrapper::InitRevtracerWrapper();
  if (ret != 0) {
	  printf("Cannot initialize revtracer-wrapper\n");
	  return 0;
  }
  assert(TestCallMapMemoryHandler_basic() == 1);
  assert(TestCallMapMemoryHandler_valid_shm() == 1);
}
