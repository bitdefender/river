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

		if (FOPEN(fMod, module, "rt")) {
			return nullptr;
		}

		AbstractBinary *bin = LoadBinary<const char *>(module, fMod);

		fclose(fMod);

		return bin;
	}


	AbstractBinary *LoadBinary(const wchar_t * module) {
		FILE *fMod;

		if (W_FOPEN(fMod, module, L"rt")) {
			return nullptr;
		}

		AbstractBinary *bin = LoadBinary<const wchar_t *>(module, fMod);

		fclose(fMod);

		return bin;
	}
}