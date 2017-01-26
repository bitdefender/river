#ifndef _WRAPPER_SETUP_H_
#define _WRAPPER_SETUP_H_

#include "../revtracer-wrapper/RevtracerWrapper.h"

extern "C" bool InitFunctionOffsets(revwrapper::LibraryLayout *libs, revwrapper::ImportedApi *api);

#endif
