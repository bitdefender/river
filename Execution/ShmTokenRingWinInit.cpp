#include "ShmTokenRingWinInit.h"

#include <Windows.h>

#include "../revtracer-wrapper/TokenRing.Windows.h"

namespace revwrapper {
	bool InitTokenRingWin(TokenRing *_this, long uCount, unsigned int *pids, long token) {
		//_this->trw = WaitTokenRingWin;
		//_this->trr = ReleaseTokenRingWin;

		TokenRingOsData *_data = (TokenRingOsData *)_this->osData;

		_data->userCount = uCount;

		HANDLE processes[USER_MAX];
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
