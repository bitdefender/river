#include "Abstract.Importer.h"
#include "Shm.Mapper.h"
#include "Inproc.Native.Importer.h"
#include "Extern.Mapper.h"
#include "ELF.Loader.h"
#include <stdio.h>

#include "LoaderAPI.h"
#include "Common.h"
// keep in mind that this funcion does NOT return the module base
void ManualLoadLibrary(const wchar_t *libName, ldr::AbstractBinary *&module, BASE_PTR &baseAddr) {
	CreateModule(libName, module);

	if (!module)
		return;

	MapModule(module, baseAddr);
}

void CreateModule(const wchar_t *libname, ldr::AbstractBinary *&module) {
	ldr::AbstractBinary *fExec = ldr::LoadBinary(libname);

	if (fExec && !fExec->IsValid()) {
		delete fExec;
		return;
	}

	module = fExec;
}

void CreateModule(const char *libname, ldr::AbstractBinary *&module) {
	ldr::AbstractBinary *fExec = ldr::LoadBinary(libname);

	if (fExec && !fExec->IsValid()) {
		delete fExec;
		return;
	}

	module = fExec;
}

void MapModule(ldr::AbstractBinary *&module, BASE_PTR &baseAddr,
		bool callConstructors, int shmFd, unsigned long offset) {
	ldr::ShmMapper mpr(shmFd, offset);
	ldr::InprocNativeImporter imp;

	if (!module->Map(mpr, imp, baseAddr, callConstructors)) {
		delete module;
		return;
	}
}

#ifdef _WIN32
void MapModuleExtern(ldr::AbstractBinary *&module, BASE_PTR &baseAddr, void *hProcess) {
	ldr::ExternMapper mLoader(hProcess);
	module->Map(mLoader, mLoader, (ldr::DWORD &)baseAddr);
}
#endif

void *ManualGetProcAddress(ldr::AbstractBinary *module, BASE_PTR base, const char *funcName) {

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

