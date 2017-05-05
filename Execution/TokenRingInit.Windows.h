#ifndef _TOKEN_RING_INIT_WINDOWS_H_
#define _TOKEN_RING_INIT_WINDOWS_H_

#if defined _WIN32 || defined __CYGWIN__

#include "../revtracer-wrapper/TokenRing.h"

namespace revwrapper {
	bool InitTokenRing(TokenRing *_this, long userCount, unsigned int *pids);
};

#endif

#endif

