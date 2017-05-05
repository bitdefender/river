#ifndef _TOKEN_RING_INIT_WINDOWS_H_
#define _TOKEN_RING_INIT_WINDOWS_H_

#ifdef __linux__

#include "../revtracer-wrapper/TokenRing.h"

namespace revwrapper {
	bool InitTokenRing(TokenRing *_this, long userCount);
};
#endif

#endif

