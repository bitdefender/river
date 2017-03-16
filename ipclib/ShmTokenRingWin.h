#ifndef _SHM_TOKEN_RING_WIN
#define _SHM_TOKEN_RING_WIN

#include "AbstractShmTokenRing.h"
#include "ipclib.h"

namespace ipc {
#define USER_MAX 4

	class ShmTokenRingWin : public AbstractTokenRing {
	protected:
		volatile long userCount;

		HANDLE waitSem[USER_MAX];
		HANDLE postSem[USER_MAX];

	public:
		ShmTokenRingWin();

		friend bool InitTokenRingWin(ShmTokenRingWin *_this, long userCount, unsigned int *pids, long token, PostEventFunc pFunc, WaitEventFunc wFunc);
		friend bool WaitTokenRingWin(AbstractTokenRing *ring, long userId, bool blocking = true);
		friend void ReleaseTokenRingWin(AbstractTokenRing *ring, long userId);
	};

};

#endif
