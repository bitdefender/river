#include <stdio.h>
#include "Abstract.Loader.h"
#include "Abstract.Importer.h"
#include "Inproc.Mapper.h"
#include "Inproc.Native.Importer.h"

typedef void* (*handler)(unsigned long);

int main() {
	ldr::AbstractBinary *fElf = ldr::LoadBinary("librevtracerwrapper.so");
	if (!fElf)
		return -1;
	ldr::InprocMapper mpr;
	ldr::InprocNativeImporter imp;
	ldr::DWORD baseAddr = 0x5000000;

	if (fElf->IsValid()) {
		printf("All is good!\n\n");
	}

	fElf->Map(mpr, imp, baseAddr);
	printf("Map done!\n");

	void *func = (void*)imp.FindImport("librevtracerwrapper.so", "CallAllocateMemoryHandler");

	void *addr = ((handler)func) (1024);
	printf("Allocated %d bytes @address %p\n", 1024, addr);

	delete fElf;

	return 0;
}
