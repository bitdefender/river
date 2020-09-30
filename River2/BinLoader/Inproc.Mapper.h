#ifndef _INPROC_MAPPER_H_
#define _INPROC_MAPPER_H_

#include <stdlib.h> //size_t
#include "Abstract.Mapper.h"

namespace ldr {
	class InprocMapper : public AbstractMapper {
	private:
		int shmFd;
		size_t offset;

	public:
		InprocMapper(int shmFd = -1, size_t offset = 0);
		virtual void *CreateSection(void *lpAddress, SIZE_T dwSize, DWORD flProtect);
		virtual bool ChangeProtect(void *lpAddress, SIZE_T dwSize, DWORD flProtect);
		virtual bool WriteBytes(void *lpAddress, void *lpBuffer, SIZE_T nSize);
	};
};

#endif
