#include <Windows.h>
#include "Mem.Mapper.h"

namespace ldr {
	void *MemMapper::CreateSection(void *lpAddress, size_t dwSize, DWORD flProtect) {
		return lpAddress;
	}

	bool MemMapper::ChangeProtect(void *lpAddress, size_t dwSize, DWORD flProtect) {
		return true;
	}

	bool MemMapper::WriteBytes(void *lpAddress, void *lpBuffer, size_t nSize) {
		memcpy(lpAddress, lpBuffer, nSize);
		return true;
	}
};