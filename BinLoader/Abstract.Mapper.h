#ifndef _ABSTRACT_MAPPER_H_
#define _ABSTRACT_MAPPER_H_

#include "Types.h"

namespace ldr {
#define PAGE_PROTECTION_READ			0x4
#define PAGE_PROTECTION_WRITE			0x2
#define PAGE_PROTECTION_EXECUTE			0x1

	class AbstractMapper {
	public:
		virtual void *CreateSection(void *lpAddress, SIZE_T dwSize, DWORD flProtect) = 0;
		virtual bool ChangeProtect(void *lpAddress, SIZE_T dwSize, DWORD flProtect) = 0;
		virtual bool WriteBytes(void *lpAddress, void *lpBuffer, SIZE_T nSize) = 0;

		/*virtual DWORD FindImport(const char *moduleName, const char *funcName) = 0;
		virtual DWORD FindImport(const char *moduleName, const unsigned int funcOrdinal) = 0;*/
	};
};

#endif

