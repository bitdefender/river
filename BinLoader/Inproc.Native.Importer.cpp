#ifdef _WIN32
#include <Windows.h>
#elif defined(__linux__)
#include <stdlib.h>
#include <sys/mman.h>
#include <string.h>
#include <dlfcn.h>
#endif

#include "Inproc.Native.Importer.h"

namespace ldr {
#ifdef _WIN32
#define MODULE_PTR HMODULE
#define GET_MODULE_HANDLE(name) GetModuleHandleA((name))
#define GET_PROC_ADDRESS_BY_NAME(module, name) GetProcAddress((module), (name))
#define GET_PROC_ADDRESS_BY_ORDINAL(module, name) GetProcAddress((module), (LPCSTR)(name))
	// versioned imports not present on Windows
#define GET_PROC_ADDRESS_BY_NAME_VERSION(module, name, version) IMPORT_NOT_FOUND
#elif defined(__linux__)
#define MODULE_PTR void *
#define GET_MODULE_HANDLE(name) dlopen((name), RTLD_LAZY)
#define GET_PROC_ADDRESS_BY_NAME(module, name) dlsym((module), (name))
	// function ordinals not present on Linux
#define GET_PROC_ADDRESS_BY_ORDINAL(module, name) IMPORT_NOT_FOUND
#define GET_PROC_ADDRESS_BY_NAME_VERSION(module, name, version) dlvsym((module), (name), (version))
#endif

	DWORD InprocNativeImporter::FindImport(const char *moduleName, const char *funcName) {
		MODULE_PTR hModule = GET_MODULE_HANDLE(moduleName);

		if (NULL == hModule) {
			return IMPORT_NOT_FOUND;
		}

		return (DWORD)GET_PROC_ADDRESS_BY_NAME(hModule, funcName);
	}

	DWORD InprocNativeImporter::FindImport(const char *moduleName, const unsigned int funcOrdinal) {
		MODULE_PTR hModule = GET_MODULE_HANDLE(moduleName);

		if (NULL == hModule) {
			return IMPORT_NOT_FOUND;
		}
		return (DWORD)GET_PROC_ADDRESS_BY_ORDINAL(hModule, funcOrdinal);
	}

	DWORD InprocNativeImporter::FindImport(const char *moduleName, const char *funcName, const char *version) {
		MODULE_PTR hModule = GET_MODULE_HANDLE(moduleName);

		if (NULL == hModule) {
			return IMPORT_NOT_FOUND;
		}
		return (DWORD)GET_PROC_ADDRESS_BY_NAME_VERSION(hModule, funcName, version);
	}

};
