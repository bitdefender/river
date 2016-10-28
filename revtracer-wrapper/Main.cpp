#include "RevtracerWrapper.h"
#include <stdio.h>


// Used for revtracer wrapper local tests
int main () {
  revwrapper::InitRevtracerWrapper();
  long ret = revwrapper::CallYieldExecution();
  void *addr = revwrapper::CallMapMemoryHandler(NULL, 0, 0, 0, NULL);
  revwrapper::CallFlushInstructionCache();
  printf("%ld %p\n", ret, addr);
}
