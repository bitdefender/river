#ifndef _LOADER_SETUP_H_
#define _LOADER_SETUP_H_

#include "../loader/Loader.h"

extern "C" bool InitLoaderOffsets(ext::LibraryLayout *libs, ldr::LoaderImports *api);

#endif