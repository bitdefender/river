#ifndef _MEM_MAPPER_H_
#define _MEM_MAPPER_H_

#include "Abstract.Mapper.h"

namespace ldr {
	class MemMapper : public AbstractMapper {
	public:
		virtual void *CreateSection(void *lpAddress, size_t dwSize, DWORD flProtect);
		virtual bool ChangeProtect(void *lpAddress, size_t dwSize, DWORD flProtect);
		virtual bool WriteBytes(void *lpAddress, void *lpBuffer, size_t nSize);
	};
}

#endif