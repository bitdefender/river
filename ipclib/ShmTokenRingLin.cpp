#include "ShmTokenRingLin.h"
#include <signal.h>
#include <stdio.h>
#include <string.h>

//#define DONOTPRINT

#ifdef DONOTPRINT
#define dbg_log(fmt,...) ((void)0)
#else
#define dbg_log(fmt,...) printf(fmt, ##__VA_ARGS__)
#endif

namespace ipc {

	static void SignalHandler(int signo)
	{
		pid_t pid = getpid();
		dbg_log("[ShmTokenRingLin] Caught signal SIGUSR1 in process %d\n", pid);
	}

	void ShmTokenRingLin::Init() {
		struct sigaction sa;

		memset(&sa, 0, sizeof(sa));

		sa.sa_handler = SignalHandler;
		int ret = sigaction(SIGUSR1, &sa, NULL);
		if (ret < 0) {
			dbg_log("[ShmTokenRingLin] Failed to setup signal handler\n");
		}
	}

	// on linux we are forced to use Use in order to set pids
	long ShmTokenRingLin::Use(pid_t pid) {
		userPids[userCount] = pid;
		// TODO this critical section
		userCount++;
		return userCount - 1;
	}

	bool ShmTokenRingLin::Wait(long int userId, bool blocking) const {

		do {
			sigset_t old_mask;
			long localCurrentOwner = currentOwner;

			if (localCurrentOwner == userId)
				return true;
			int ret = sigprocmask(SIG_SETMASK, nullptr, &old_mask);

			if (ret < 0) {
				dbg_log("[ipclib] Wait failed with exitcode %d\n", ret);
				return false;
			}

			sigsuspend(&old_mask);

			localCurrentOwner = currentOwner;
			if (localCurrentOwner != userId && !blocking)
				return false;
		} while (1);
	}

	void ShmTokenRingLin::Release(long userId) {

		if (currentOwner == userId) {
			long localUserCount = userCount;
			long localCurrentOwner = currentOwner;

			localCurrentOwner = currentOwner + 1;
			if (localCurrentOwner == localUserCount) {
				localCurrentOwner = 0;
			}

			currentOwner = localCurrentOwner;
			int ret = kill(userPids[currentOwner], SIGUSR1);
			if (ret < 0)
				dbg_log("[ShmTokenRingWin] Could not send signal to my neighbour %d.\n", userPids[currentOwner]);
		}

	}
} //namespace ipc
