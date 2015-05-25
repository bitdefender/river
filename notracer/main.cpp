#include "extern.h"
#include <stdio.h>

int overlap(unsigned int a1, unsigned int a2, unsigned int b1, unsigned int b2);

bool MapPE(DWORD &baseAddr);

typedef int(*MainFunc)(unsigned int argc, char *argv[]);

int main(unsigned int argc, char *argv[]) {
	DWORD baseAddr = 0;
	if (!MapPE(baseAddr)) {
		return false;
	}

	//MainFunc pMain = (MainFunc)(baseAddr + 0x1032);
	MainFunc pMain = (MainFunc)(baseAddr + 0x96CE);
	pMain(argc, argv);
	return 0;
}