#include <stdio.h>
#include "ELF.Loader.h"
#include "Inproc.Mapper.h"

int main() {
	ldr::FloatingELF32 fElf("libexecution.so");
	ldr::InprocMapper mpr;
	ldr::DWORD baseAddr = 0x5000000;

	if (fElf.IsValid()) {
		printf("All is good!\n\n");
	}

	
	fElf.Map(mpr, baseAddr);

	return 0;
}
