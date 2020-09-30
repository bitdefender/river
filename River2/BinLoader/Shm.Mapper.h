#ifndef _SHM_MAPPER_H_
#define _SHM_MAPPER_H_

//#include <stdlib.h> //size_t
#include "Abstract.Mapper.h"

namespace ldr {
	class ShmMapper : public AbstractMapper {
	private:
		int shmFd;
		SIZE_T offset;

	public:
		ShmMapper(int shmFd = -1, SIZE_T offset = 0);
		virtual void *CreateSection(void *lpAddress, SIZE_T dwSize, DWORD flProtect);
		virtual bool ChangeProtect(void *lpAddress, SIZE_T dwSize, DWORD flProtect);
		virtual bool WriteBytes(void *lpAddress, void *lpBuffer, SIZE_T nSize);
	};
};

#endif
