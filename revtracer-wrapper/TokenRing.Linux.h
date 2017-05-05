#ifndef _TOKEN_RING_LINUX_H_
#define _TOKEN_RING_LINUX_H_

#if defined __linux__
#include <semaphore.h>
#include "../CommonCrossPlatform/Common.h"

namespace revwrapper {

	#define MAX_USER_COUNT 4

	struct TokenRingOsData {
		long userCount;

		sem_t semaphores[MAX_USER_COUNT];
	};
};


#endif

#endif
