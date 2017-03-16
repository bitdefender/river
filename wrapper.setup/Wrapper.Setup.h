#ifndef _WRAPPER_SETUP_H_
#define _WRAPPER_SETUP_H_

#include "../revtracer-wrapper/RevtracerWrapper.h"

extern "C" bool InitWrapperOffsets(ext::LibraryLayout *libs, revwrapper::WrapperImports *api);

#endif
