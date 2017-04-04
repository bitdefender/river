#ifndef _SHM_TOKEN_RING_WIN_INIT_H_
#define _SHM_TOKEN_RING_WIN_INIT_H_

#include "../revtracer-wrapper/TokenRing.h"

namespace revwrapper {
	bool InitTokenRingWin(TokenRing *_this, long userCount, unsigned int *pids, long token);
};

#endif

