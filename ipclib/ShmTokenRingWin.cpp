#include "ShmTokenRingWin.h"
#include "ipclib.h"

namespace ipc {
	bool WaitTokenRingWin(AbstractTokenRing *ring, long userId, bool blocking) {
		ShmTokenRingWin *_this = (ShmTokenRingWin *)ring;
		return _this->wait(&_this->waitSem[userId], blocking ? 1000 : 0);
	}

	void ReleaseTokenRingWin(AbstractTokenRing *ring, long userId) {
		ShmTokenRingWin *_this = (ShmTokenRingWin *)ring;
		long nextId = userId + 1;
		if (_this->userCount == userId) {
			nextId = 0;
		}

		_this->post(&_this->postSem[nextId]);
	}

	ShmTokenRingWin::ShmTokenRingWin() {
		trw = WaitTokenRingWin;
		trr = ReleaseTokenRingWin;
	}
};
