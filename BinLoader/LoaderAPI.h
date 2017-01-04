#ifndef _LOADER_API
#define _LOADER_API

#if defined(__linux__) || defined(EXTERN_EXECUTION_ONLY)
#include <string>
#include "Abstract.Loader.h"
typedef unsigned long DWORD;
typedef ldr::AbstractBinary *MODULE_PTR;
typedef DWORD BASE_PTR;
struct mappedObject {
	MODULE_PTR module;
	BASE_PTR base;
	DWORD size;
};

#define LOAD_LIBRARYW(libName, module, base) ManualLoadLibrary((libName), (module), (base))
#define GET_PROC_ADDRESS(module, base, name) ManualGetProcAddress((module), (base), (name))
#define UNLOAD_MODULE(module) delete (module)


void ManualLoadLibrary(const wchar_t *libName, MODULE_PTR &module, BASE_PTR &baseAddr);
void *ManualGetProcAddress(MODULE_PTR module, BASE_PTR base, const char *funcName);
DWORD GetEntryPoint(const char *elfName);
void CreateModule(const wchar_t *libname, MODULE_PTR &module);
void CreateModule(const char *libname, MODULE_PTR &module);
void MapModule(MODULE_PTR &module, BASE_PTR &baseAddr, int shmFd = -1, unsigned long offset = 0);
void MapModuleExtern(MODULE_PTR &module, BASE_PTR &baseAddr, void *hProcess);

template <typename T> bool LoadExportedName(MODULE_PTR &module, BASE_PTR &base, const char *name, T *&ptr) {
	ptr = (T *)GET_PROC_ADDRESS(module, base, name);
	return ptr != NULL;
}


#elif defined(_WIN32)
typedef HMODULE BASE_PTR;
typedef void *MODULE_PTR;

#define LOAD_LIBRARYW(libName, module, base) do { base = LoadLibraryW((libName)); module = nullptr; } while (false);
#define GET_PROC_ADDRESS(module, base, name) GetProcAddress((base), (name))
#define UNLOAD_MODULE(module)
#endif

#endif
