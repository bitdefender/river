
#include "Loader/PE.ldr.h"
#include "Loader/Inproc.Mapper.h"

int __stdcall MyIsProcessorFeaturePresent(DWORD ProcessorFeature);
void __stdcall MyExitProcess(DWORD retCode);

class HookedMapper : public InprocMapper {
public :
	virtual DWORD FindImport(const char *moduleName, const char *funcName) {
		if (0 == _strcmpi(moduleName, "kernel32.dll")) {
			if (0 == _strcmpi(funcName, "IsProcessorFeaturePresent")) {
				return (DWORD)MyIsProcessorFeaturePresent;
			}

			/*if (0 == _strcmpi(funcName, "ExitProcess")) {
				return (DWORD)MyExitProcess;
			}*/
		}
		return InprocMapper::FindImport(moduleName, funcName);

	}
};

bool MapPE(DWORD &baseAddr) {
	HookedMapper mapper;
	FloatingPE pe("..\\lzo\\a.exe");
	//FloatingPE pe("..\\lzo\\hw.exe");

	if (!pe.MapPE(mapper, baseAddr)) {
		printf("Couldn't map pe!");
		return false;
	}

	return true;
}