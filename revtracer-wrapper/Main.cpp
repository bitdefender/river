#include "RevtracerWrapper.h"
#include <stdio.h>
#include <assert.h>
#include <errno.h>

// Used for revtracer wrapper local tests
//
int TestCallMapMemoryHandler_basic() {
	void *address = (void*)0;
	void *addr = revwrapper::CallMapMemoryHandler(0, 0, 0, 1000, address);
	printf("[TestCallMapMemoryHandler_basic] Result [%p]\n", addr);
	return addr == (void*)-1;
}
int main () {

  int ret = revwrapper::InitRevtracerWrapper();
  if (ret != 0) {
	  printf("Cannot initialize revtracer-wrapper\n");
	  return 0;
  }
  assert(TestCallMapMemoryHandler_basic() == 1);
}
