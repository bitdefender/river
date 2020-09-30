#ifndef _TOKEN_RING_WINDOWS_H_
#define _TOKEN_RING_WINDOWS_H_

#if defined _WIN32 || defined __CYGWIN__

namespace revwrapper {
	typedef void *HANDLE;

	#define MAX_USER_COUNT 4

	struct TokenRingOsData {
		long userCount;

		HANDLE waitSem[MAX_USER_COUNT];
		HANDLE postSem[MAX_USER_COUNT];
	};
};

#endif

#endif
