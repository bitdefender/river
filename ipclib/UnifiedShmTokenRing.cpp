#include "AbstractShmTokenRing.h"
#include "ShmTokenRingWin.h"
#include "ShmTokenRingLin.h"

namespace ipc {
	AbstractShmTokenRing *AbstractShmTokenRingFactory(void) {
#ifdef __linux__
		return new ShmTokenRingLin();
#elif defined(_WIN32)
		return new ShmTokenRingWin();
#endif
	}
}
