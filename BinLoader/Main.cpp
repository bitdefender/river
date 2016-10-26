#include <stdio.h>
#include "Abstract.Loader.h"
#include "Abstract.Importer.h"
#include "Inproc.Mapper.h"
#include "Inproc.Native.Importer.h"
#include <string.h>

#define DEBUG_BREAK asm volatile("int $0x3")
typedef int (*handler)();

int main() {
	ldr::AbstractBinary *fElf = ldr::LoadBinary("libhttp-parser.so");
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

	unsigned long rva;

	bool ok = fElf->GetExport("payloadBuffer", rva);

	char *addr = (char*) (baseAddr + rva);
	printf("Main: buffer Address is %p %lx\n", addr, rva);
	strcpy(addr, "mumu");

	ok = fElf->GetExport("Payload", rva);

	void *func = (void*) (baseAddr + rva);

	((handler)func) ();

	delete fElf;

	return 0;
}
