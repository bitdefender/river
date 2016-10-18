#include <stdio.h>
#include "ELF.ldr.h"

int main() {
	FloatingELF32 fElf("libexecution.so");

	if (fElf.IsValid()) {
		printf("All is good!\n\n");
	}

	return 0;
}
