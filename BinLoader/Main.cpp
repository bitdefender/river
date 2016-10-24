#include <stdio.h>
//#include "ELF.Loader.h"
#include "Abstract.Loader.h"
#include "Inproc.Mapper.h"
#include "Inproc.Native.Importer.h"

int main() {
	printf("here\n");
	ldr::AbstractBinary *fElf = ldr::LoadBinary("libexecution.so");
	ldr::InprocMapper mpr;
	ldr::InprocNativeImporter imp;
	ldr::DWORD baseAddr = 0x5000000;

	if (fElf->IsValid()) {
		printf("All is good!\n\n");
	}

	fElf->Map(mpr, imp, baseAddr);
	printf("Map done!\n");

	delete fElf;

	return 0;
}
