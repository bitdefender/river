#include "ShmTokenRing.h"
#include "ipclib.h"

#ifdef _MSC_VER
#include <intrin.h>
#define LOCK_INC(count_ptr) _InterlockedIncrement(count_ptr)
#else
#define LOCK_INC(count_ptr)  asm volatile("lock; incl %0" : "=m"(count_ptr) : "m"(count_ptr))
#endif

namespace ipc {
	void ShmTokenRing::Init() {
		userCount = 0;
		currentOwner = 0;
	}

	void ShmTokenRing::Init(long presetUsers) {
		userCount = presetUsers;
		currentOwner = 0;
	}

	long ShmTokenRing::Use() {
		LOCK_INC(userCount);
		return userCount - 1;
	}

	void NtDllNtYieldExecution();

	bool ShmTokenRing::Wait(long userId, bool blocking) const {
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

	void ShmTokenRing::Release(long userId) {
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
