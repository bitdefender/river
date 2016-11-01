#ifndef _LOADER_API
#define _LOADER_API

#ifdef __linux__
#include "Abstract.Loader.h"
typedef unsigned long DWORD;
typedef ldr::AbstractBinary *MODULE_PTR;
typedef DWORD BASE_PTR;

#define LOAD_LIBRARYW(libName, module, base) ManualLoadLibrary((libName), (module), (base))
#define GET_PROC_ADDRESS(module, base, name) ManualGetProcAddress((module), (base), (name))
#define UNLOAD_MODULE(module) delete (module)


void ManualLoadLibrary(const wchar_t *libName, MODULE_PTR &module, BASE_PTR &baseAddr);
void *ManualGetProcAddress(MODULE_PTR module, BASE_PTR base, const char *funcName);
DWORD GetEntryPoint(const char *elfName);

#elif defined(_WIN32)
typedef HMODULE BASE_PTR;
typedef void *MODULE_PTR;

#define LOAD_LIBRARYW(libName, module, base) do { base = LoadLibraryW((libName)); module = nullptr; } while (false);
#define GET_PROC_ADDRESS(module, base, name) GetProcAddress((base), (name))
#define UNLOAD_MODULE(module)
#endif

#endif
