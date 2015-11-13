#include "ShmTokenRing.h"
#include "ipclib.h"


#include <intrin.h>

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
		long ret = _InterlockedIncrement(&userCount);
		return ret - 1;
	}

	void NtDllNtYieldExecution();

	void ShmTokenRing::Wait(long userId) const {
		long loop = 1 << 8;

		do {
			long localCurrentOwner = currentOwner;
			if (localCurrentOwner == userId) {
				return;
			}

			for (long i = 0; i < 1; ++i);

			localCurrentOwner = currentOwner;
			if (localCurrentOwner != userId) {
				if (loop < (1 << 16)) {
					loop <<= 1;
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