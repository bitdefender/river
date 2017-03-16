#include "ShmTokenRingWinInit.h"

#include <Windows.h>

namespace ipc {
	bool InitTokenRingWin(ShmTokenRingWin *_this, long uCount, unsigned int *pids, long token, PostEventFunc pFunc, WaitEventFunc wFunc) {
		_this->post = pFunc;
		_this->wait = wFunc;
		_this->userCount = uCount;

		HANDLE processes[USER_MAX];
		for (int i = 0; i < uCount; ++i) {
			processes[i] = OpenProcess(PROCESS_DUP_HANDLE, FALSE, pids[i]);
		}

		for (int i = 0; i < uCount; ++i) {
			HANDLE hEvt = CreateEvent(nullptr, FALSE, (token == i) ? TRUE : FALSE, nullptr);

			DuplicateHandle(
				GetCurrentProcess(),
				hEvt,
				processes[i],
				&_this->waitSem[i],
				0,
				FALSE,
				DUPLICATE_SAME_ACCESS
			);

			DuplicateHandle(
				GetCurrentProcess(),
				hEvt,
				processes[(0 == i) ? (uCount - 1) : (i - 1)],
				&_this->postSem[i],
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
