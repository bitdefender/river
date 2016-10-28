#include "RevtracerWrapper.h"
#include <stdio.h>


// Used for revtracer wrapper local tests
int main () {
  revwrapper::InitRevtracerWrapper();
  long ret = revwrapper::CallYieldExecution();
  printf("%ld\n", ret);
}
