#ifndef _LOADER_API
#define _LOADER_API

#if defined(__linux__)
#include <string>
#include "Abstract.Loader.h"
typedef unsigned long DWORD;
typedef ldr::AbstractBinary *MODULE_PTR;
typedef DWORD BASE_PTR;

#define LOAD_LIBRARYW(libName, module, base) ManualLoadLibrary((libName), (module), (base))
#define GET_PROC_ADDRESS(module, base, name) ManualGetProcAddress((module), (base), (name))
#define UNLOAD_MODULE(module) delete (module)

#elif defined(_WIN32)

#include <Windows.h>
typedef HMODULE BASE_PTR;
typedef void *MODULE_PTR;

#define LOAD_LIBRARYW(libName, module, base) do { base = LoadLibraryW((libName)); module = nullptr; } while (false);
#define GET_PROC_ADDRESS(module, base, name) GetProcAddress((base), (name))
#define UNLOAD_MODULE(module)
#endif


#include "Abstract.Loader.h"

struct MappedObject {
	ldr::AbstractBinary *module;
	BASE_PTR base;
	DWORD size;
};

void ManualLoadLibrary(const wchar_t *libName, ldr::AbstractBinary *&module, BASE_PTR &baseAddr);
void *ManualGetProcAddress(ldr::AbstractBinary *module, BASE_PTR base, const char *funcName);
DWORD GetEntryPoint(const char *elfName);
void CreateModule(const wchar_t *libname, ldr::AbstractBinary *&module);
void CreateModule(const char *libname, ldr::AbstractBinary *&module);
void MapModule(ldr::AbstractBinary *&module, BASE_PTR &baseAddr);
void MapModule(ldr::AbstractBinary *&module, BASE_PTR &baseAddr, bool callConstructors, int shmFd, unsigned long offset);
void MapModuleExtern(ldr::AbstractBinary *&module, BASE_PTR &baseAddr, void *hProcess);

template <typename T> bool LoadExportedName(ldr::AbstractBinary *&module, BASE_PTR &base, const char *name, T *&ptr) {
	ptr = (T *)GET_PROC_ADDRESS(module, base, name);
	return ptr != NULL;
}


#endif
