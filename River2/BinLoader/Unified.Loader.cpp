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
		solve_path(module, path);

		if (FOPEN(fMod, path, "rt"))
			return nullptr;

		AbstractBinary *bin = LoadBinary<const char *>(path, fMod);

		fclose(fMod);

		return bin;
	}


	AbstractBinary *LoadBinary(const wchar_t * module) {
		FILE *fMod;
		wchar_t path[MAX_PATH_NAME];
		solve_path(module, path);

		if (W_FOPEN(fMod, path, L"rt"))
			return nullptr;
		AbstractBinary *bin = LoadBinary<const wchar_t *>(path, fMod);

		fclose(fMod);

		return bin;
	}
}
