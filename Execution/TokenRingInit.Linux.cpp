#ifdef __linux__


#include "TokenRingInit.Linux.h"

//#include <semaphore.h>
#include <link.h>
#include <string.h>
#include <stdio.h>

#include "../CommonCrossPlatform/Common.h"
#include "../revtracer-wrapper/TokenRing.Linux.h"

namespace revwrapper {
	typedef int (*semInitFunc)(sem_t *sem, int pshared, unsigned int value);

	int FindLibPthread(struct dl_phdr_info *info, size_t size, void *data) {
		LIB_T *dest = (LIB_T *)data;
		//printf("[Child] module name=\"%s\" @ %08x\n", info->dlpi_name, info->dlpi_addr);
		
		const char *p = strrchr(info->dlpi_name, '/');
		p = (nullptr == p) ? info->dlpi_name : (p + 1);

		if (0 == strncmp("libpthread.so", p, 13)) {
			printf("Found libpthread @ %08x\n", info->dlpi_addr);
			*dest = GET_LIB_HANDLER(info->dlpi_name);
			return 0;
		}

		return 0;
	}


	bool InitTokenRing(TokenRing *_this, long uCount) {
		LIB_T libPthread;
		dl_iterate_phdr(FindLibPthread, &libPthread);
		semInitFunc _sem_init = (semInitFunc)dlvsym(libPthread, "sem_init", "GLIBC_2.1");
		
		TokenRingOsData *_data = (TokenRingOsData *)_this->osData;

		_data->userCount = uCount;

		for (int i = 0; i < uCount; ++i) {
			if (-1 == _sem_init(&_data->semaphores[i], 1, 0)) {
				return false;
			}
		}

		return true;
	}

}; // namespace ipc

#endif
