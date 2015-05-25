
#include "Loader/PE.ldr.h"
#include "Loader/Inproc.Mapper.h"

extern "C" BYTE MapPE(DWORD *baseAddr) {
	InprocMapper mapper;
	FloatingPE pe("..\\lzo\\a.exe");
	DWORD dwAddr = *baseAddr;

	if (!pe.MapPE(mapper, dwAddr)) {
		printf("Couldn't map pe!");
		return 0;
	}

	*baseAddr = dwAddr;
	return 1;
}