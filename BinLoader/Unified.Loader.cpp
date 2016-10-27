#include "Abstract.Loader.h"

#include "Common.h"

#include "ELF.Loader.h"
#include "PE.Loader.h"

namespace ldr {

	template <typename NT> AbstractBinary *LoadBinary(NT fileName, FILE *file) {
		if (FloatingPE::CanLoad(file)) {
			return new FloatingPE(fileName);
		}

		if (FloatingELF32::CanLoad(file)) {
			return new FloatingELF32(fileName);
		}

		return nullptr;
	}

	AbstractBinary *LoadBinary(const char *module) {
		FILE *fMod;
		char path[MAX_PATH_NAME];

		if (find_in_env(module, path)) {
			FOPEN(fMod, path, "rt");
		} else {
			printf("Could not find %s. Set LD_LIBRARY_PATH accordingly\n",
					module);
			return nullptr;
		}

		AbstractBinary *bin = LoadBinary<const char *>(path, fMod);

		fclose(fMod);

		return bin;
	}


	AbstractBinary *LoadBinary(const wchar_t * module) {
		FILE *fMod;
		char path[MAX_PATH_NAME];

		if (find_in_env(module, path)) {
			FOPEN(fMod, path, "rt");
		} else {
			printf("Could not find module. Set LD_LIBRARY_PATH accordingly\n");
			return nullptr;
		}

		AbstractBinary *bin = LoadBinary<const char *>(path, fMod);

		fclose(fMod);

		return bin;
	}
}
