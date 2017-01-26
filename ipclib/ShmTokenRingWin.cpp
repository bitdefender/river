#include "ShmTokenRingWin.h"
#include "ipclib.h"

#ifdef _MSC_VER
#include <intrin.h>
#define LOCK_INC(count_ptr) _InterlockedIncrement(&(count_ptr))
#endif

namespace ipc {
	void ShmTokenRingWin::Init() {
		userCount = 0;
		currentOwner = 0;
	}

	void ShmTokenRingWin::Init(long presetUsers) {
		userCount = presetUsers;
		currentOwner = 0;
	}

	unsigned long ShmTokenRingWin::Use(unsigned long id) {
		long ret = LOCK_INC(userCount);
		return ret - 1;
	}

	void NtDllNtYieldExecution();

	bool ShmTokenRingWin::Wait(long userId, bool blocking) const {
		long loop = 1 << 8;

		do {
			long localCurrentOwner = currentOwner;
			if (localCurrentOwner == userId) {
				return true;
			}

			for (long i = 0; i < loop; ++i);

			localCurrentOwner = currentOwner;
			if (localCurrentOwner != userId) {
				if (loop < (1 << 16)) {
					loop <<= 1;
				} else {
					if (!blocking) {
						return false;
					}
				}
				//((NtYieldExecutionFunc)ipcAPI.ntYieldExecution)();
				NtDllNtYieldExecution();
			}

		} while (1);
	}

	void ShmTokenRingWin::Release(long userId) {
		if (currentOwner == userId) {
			long localUserCount = userCount;
			long localCurrentOwner = currentOwner;

			localCurrentOwner = currentOwner + 1;
			if (localCurrentOwner == localUserCount) {
				localCurrentOwner = 0;
			}

			currentOwner = localCurrentOwner;
		}
	}
};
