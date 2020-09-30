#include "AbstractShmTokenRing.h"

#ifdef __linux__
#include "ShmTokenRingLin.h"
#elif defined(_WIN32)
#include "ShmTokenRingWin.h"
#endif

namespace ipc {
	AbstractShmTokenRing *AbstractShmTokenRingFactory(void) {
#ifdef __linux__
		return new ShmTokenRingLin();
#elif defined(_WIN32)
		return new ShmTokenRingWin();
#endif
	}
}
