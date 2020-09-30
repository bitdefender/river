#include <Windows.h>
#include "Mem.Mapper.h"

namespace ldr {
	void *MemMapper::CreateSection(void *lpAddress, SIZE_T dwSize, DWORD flProtect) {
		return lpAddress;
	}

	bool MemMapper::ChangeProtect(void *lpAddress, SIZE_T dwSize, DWORD flProtect) {
		return true;
	}

	bool MemMapper::WriteBytes(void *lpAddress, void *lpBuffer, SIZE_T nSize) {
		memcpy(lpAddress, lpBuffer, nSize);
		return true;
	}
};