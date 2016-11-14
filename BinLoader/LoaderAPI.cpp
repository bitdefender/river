#if defined(__linux__) || defined(EXTERN_EXECUTION_ONLY)
#include "Abstract.Importer.h"
#include "Inproc.Mapper.h"
#include "Inproc.Native.Importer.h"
#include "Extern.Mapper.h"
#include "ELF.Loader.h"
#include <stdio.h>

#include "LoaderAPI.h"
// keep in mind that this funcion does NOT return the module base
void ManualLoadLibrary(const wchar_t *libName, MODULE_PTR &module, BASE_PTR &baseAddr) {
	CreateModule(libName, module);

	if (!module)
		return;

	MapModule(module, baseAddr);
}

void CreateModule(const wchar_t *libname, MODULE_PTR &module) {
	ldr::AbstractBinary *fExec = ldr::LoadBinary(libname);

	if (!fExec->IsValid()) {
		delete fExec;
		return;
	}

	module = fExec;
}

void MapModule(MODULE_PTR &module, BASE_PTR &baseAddr) {
	ldr::InprocMapper mpr;
	ldr::InprocNativeImporter imp;

	if (!module->Map(mpr, imp, baseAddr)) {
		delete module;
		return;
	}
}

#ifdef _WIN32
void MapModuleExtern(MODULE_PTR &module, BASE_PTR &baseAddr, void *hProcess) {
	ldr::ExternMapper mLoader(hProcess);
	module->Map(mLoader, mLoader, baseAddr);
}
#endif

void *ManualGetProcAddress(MODULE_PTR module, BASE_PTR base, const char *funcName) {

	DWORD rva;
	if (!module->GetExport(funcName, rva)) {
		return nullptr;
	}
	return (void *)(base + rva);
}

DWORD GetEntryPoint(const char *elfName) {
	ldr::AbstractBinary *fExec = ldr::LoadBinary(elfName);
	if (ldr::FloatingELF32* fElf = dynamic_cast<ldr::FloatingELF32*>(fExec)) {
		unsigned long entryPoint = fElf->GetEntryPoint();
		delete fExec;
		return entryPoint;
	}

	delete fExec;
	return 0;
}

#endif
