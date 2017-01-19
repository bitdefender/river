#ifdef _WIN32
#include <Windows.h>
#elif defined(__linux__)
#include <stdlib.h>
#include <sys/mman.h>
#include <string.h>
#include <dlfcn.h>
#endif

#include "Inproc.Native.Importer.h"
#include "Abstract.Loader.h"

namespace ldr {
#ifdef _WIN32
#define MODULE_PTR HMODULE
#define GET_MODULE_HANDLE(name) GetModuleHandleA((name))
#define GET_PROC_ADDRESS_BY_NAME(module, name) GetProcAddress((module), (name))
#define GET_PROC_ADDRESS_BY_ORDINAL(module, name) GetProcAddress((module), (LPCSTR)(name))
	// versioned imports not present on Windows
#define GET_PROC_ADDRESS_BY_NAME_VERSION(module, name, version) IMPORT_NOT_FOUND
#define GET_IMPORT(module, rva) ((0 == rva) ? (IMPORT_NOT_FOUND) : ((DWORD)module) + (rva))
#elif defined(__linux__)
#define MODULE_PTR void *
#define GET_MODULE_HANDLE(name) dlopen((name), RTLD_LAZY)
#define GET_PROC_ADDRESS_BY_NAME(module, name) dlsym((module), (name))
	// function ordinals not present on Linux
#define GET_PROC_ADDRESS_BY_ORDINAL(module, name) IMPORT_NOT_FOUND
#define GET_PROC_ADDRESS_BY_NAME_VERSION(module, name, version) dlvsym((module), (name), (version))
#define GET_IMPORT(module, rva) ((0 == rva) ? (IMPORT_NOT_FOUND) : (*(DWORD *)module) + (rva))
	//TODO
#endif

	InprocNativeImporter::CachedModule *InprocNativeImporter::AddModule(const char *name)	{
		int sz = modules.size();
		modules.resize(sz + 1);

		modules[sz].moduleName = name;
		return &modules[sz];
	}

	InprocNativeImporter::CachedModule *InprocNativeImporter::FindModule(const char *moduleName) {
		for (auto it = modules.begin(); it != modules.end(); ++it) {
			if (it->moduleName == moduleName) {
				return &(*it);
			}
		}
		return nullptr;
	}

	InprocNativeImporter::CachedModule *InprocNativeImporter::GetModule(const char *name) {
		CachedModule *mod = FindModule(name);

		if (nullptr == mod) {
			AbstractBinary *bin = LoadBinary(name); 
			
			if (nullptr == bin) {
				printf("Could not load module %s\n", name);
				return nullptr;
			}
			mod = AddModule(name);
			bin->ForAllExports([mod](const char *funcName, const DWORD ordinal, const char *version, const DWORD rva, const unsigned char *body) {
				mod->AddImport(funcName, ordinal, version, rva);
				printf("func %s rva %08lx\n", funcName, rva);
			});

			delete bin;
		}

		return mod;
	}

	void InprocNativeImporter::CachedModule::AddImport(const char *funcName, const unsigned int ordinal, const char *version, DWORD rva) {
		int sz = entries.size();

		entries.resize(sz + 1);
		entries[sz].funcName = funcName;
		entries[sz].ordinal = ordinal;
		entries[sz].version = version;
		entries[sz].rva = rva;
	}



	DWORD InprocNativeImporter::CachedModule::FindCachedImport(const char *funcName) {
		for (auto it = entries.begin(); it != entries.end(); ++it) {
			if (it->funcName == funcName) {
				return it->rva;
			}
		}
		return 0;
	}

	DWORD InprocNativeImporter::CachedModule::FindCachedImport(const unsigned int funcOrdinal) {
		for (auto it = entries.begin(); it != entries.end(); ++it) {
			if (it->ordinal == funcOrdinal) {
				return it->rva;
			}
		}
		return 0;
	}

	DWORD InprocNativeImporter::CachedModule::FindCachedImport(const char *funcName, const char *version) {
		for (auto it = entries.begin(); it != entries.end(); ++it) {
			if ((it->funcName == funcName) && (it->version == version)) {
				return it->rva;
			}
		}
		return 0;
	}

	DWORD InprocNativeImporter::FindImport(const char *moduleName, const char *funcName) {
		MODULE_PTR hModule = GET_MODULE_HANDLE(moduleName);

		if (NULL == hModule) {
			return IMPORT_NOT_FOUND;
		}

		DWORD ret = (DWORD)GET_PROC_ADDRESS_BY_NAME(hModule, funcName);

		if (0 == ret) {
			CachedModule *mod = GetModule(moduleName);
			ret = GET_IMPORT(hModule, mod->FindCachedImport(funcName));
		}

		return ret;
	}

	DWORD InprocNativeImporter::FindImport(const char *moduleName, const unsigned int funcOrdinal) {
		MODULE_PTR hModule = GET_MODULE_HANDLE(moduleName);

		if (NULL == hModule) {
			return IMPORT_NOT_FOUND;
		}
		DWORD ret = (DWORD)GET_PROC_ADDRESS_BY_ORDINAL(hModule, funcOrdinal);
		if (0 == ret) {
			CachedModule *mod = GetModule(moduleName);
			ret = GET_IMPORT(hModule, mod->FindCachedImport(funcOrdinal));
		}

		return ret;
	}

	DWORD InprocNativeImporter::FindImport(const char *moduleName, const char *funcName, const char *version) {
		MODULE_PTR hModule = GET_MODULE_HANDLE(moduleName);

		if (NULL == hModule) {
			return IMPORT_NOT_FOUND;
		}
		DWORD ret = (DWORD)GET_PROC_ADDRESS_BY_NAME_VERSION(hModule, funcName, version);
		if (0 == ret) {
			CachedModule *mod = GetModule(moduleName);
			ret = GET_IMPORT(hModule, mod->FindCachedImport(funcName, version));
		}

		printf("module %s func %s rva %08lx modulebase %08lx\n", moduleName, funcName, ret, *(DWORD*)(hModule));
		return ret;
	}

};
