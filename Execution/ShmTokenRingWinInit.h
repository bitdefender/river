#ifndef _SHM_TOKEN_RING_WIN_INIT_H_
#define _SHM_TOKEN_RING_WIN_INIT_H_

#include "../ipclib/ShmTokenRingWin.h"

namespace ipc {
	bool InitTokenRingWin(ShmTokenRingWin *_this, long userCount, unsigned int *pids, long token);
};

#endif

