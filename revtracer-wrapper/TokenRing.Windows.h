#ifndef _TOKEN_RING_WINDOWS_H_
#define _TOKEN_RING_WINDOWS_H_

#if defined _WIN32 || defined __CYGWIN__

namespace revwrapper {
	typedef void *HANDLE;

	#define USER_MAX 4

	struct TokenRingOsData {
		volatile long userCount;

		HANDLE waitSem[USER_MAX];
		HANDLE postSem[USER_MAX];
	};
};

#endif

#endif
