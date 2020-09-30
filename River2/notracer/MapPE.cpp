
#include "Loader/PE.ldr.h"
#include "Loader/Inproc.Mapper.h"

bool MapPE(DWORD &baseAddr) {
	InprocMapper mapper;
	FloatingPE pe("..\\lzo\\a.exe");

	if (!pe.MapPE(mapper, baseAddr)) {
		printf("Couldn't map pe!");
		return false;
	}

	return true;
}