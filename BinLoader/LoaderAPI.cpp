#ifdef __linux__
#include "Abstract.Importer.h"
#include "Inproc.Mapper.h"
#include "Inproc.Native.Importer.h"
#include "ELF.Loader.h"
#include <stdio.h>

#include "LoaderAPI.h"
// keep in mind that this funcion does NOT return the module base
void ManualLoadLibrary(const wchar_t *libName, MODULE_PTR &module, BASE_PTR &baseAddr) {
	ldr::AbstractBinary *fExec = ldr::LoadBinary(libName);

	ldr::InprocMapper mpr;
	ldr::InprocNativeImporter imp;

	if (!fExec->IsValid()) {
		delete fExec;
		return;
	}

	if (!fExec->Map(mpr, imp, baseAddr)) {
		delete fExec;
		return;
	}

	module = fExec;
}

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
		delete fExec;
		return fElf->GetEntryPoint();
	}

	delete fExec;
	return 0;
}

#endif
