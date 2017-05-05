#if defined _WIN32 || defined __CYGWIN__

#include "TokenRingInit.Windows.h"

#include <Windows.h>

#include "../revtracer-wrapper/TokenRing.Windows.h"

namespace revwrapper {
	bool InitTokenRing(TokenRing *_this, long uCount, unsigned int *pids) {
		//_this->trw = WaitTokenRingWin;
		//_this->trr = ReleaseTokenRingWin;

		TokenRingOsData *_data = (TokenRingOsData *)_this->osData;

		_data->userCount = uCount;

		HANDLE processes[MAX_USER_COUNT];
		for (int i = 0; i < uCount; ++i) {
			processes[i] = OpenProcess(PROCESS_DUP_HANDLE, FALSE, pids[i]);
		}

		for (int i = 0; i < uCount; ++i) {
			HANDLE hEvt = CreateEvent(nullptr, FALSE, FALSE, nullptr); // (token == i) ? TRUE : FALSE, nullptr);

			DuplicateHandle(
				GetCurrentProcess(),
				hEvt,
				processes[i],
				&_data->waitSem[i],
				0,
				FALSE,
				DUPLICATE_SAME_ACCESS
			);

			DuplicateHandle(
				GetCurrentProcess(),
				hEvt,
				processes[(0 == i) ? (uCount - 1) : (i - 1)],
				&_data->postSem[i],
				0,
				FALSE,
				DUPLICATE_SAME_ACCESS
			);

			CloseHandle(hEvt);
		}

		for (int i = 0; i < uCount; ++i) {
			CloseHandle(processes[i]);
		}

		return true;
	}

}; // namespace ipc

#endif