#ifndef _SHM_TOKEN_RING_WIN
#define _SHM_TOKEN_RING_WIN

#include "AbstractShmTokenRing.h"

namespace ipc {
	class ShmTokenRingWin : public AbstractShmTokenRing {
	private:
		volatile long currentOwner;
		volatile long userCount;

	public:
		void Init();
		void Init(long presetUsers);

		long Use();

		bool Wait(long userId, bool blocking = true) const;
		void Release(long userId);
	};

};

#endif
