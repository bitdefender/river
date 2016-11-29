#include "ShmTokenRingLin.h"
#include <signal.h>
#include <stdio.h>
#include <string.h>

//#define DONOTPRINT

#ifdef DONOTPRINT
#define dbg_log(fmt,...) ((void)0)
#else
#define dbg_log(fmt,...) { printf(fmt, ##__VA_ARGS__); fflush(stdout); }
#endif

namespace ipc {

	void ShmTokenRingLin::Init(long presetUsers) {
		userCount = presetUsers;
		currentOwner = 0;
		for (int i = 0; i < userCount; i++)
			userPids[i] = -1;
	}

	// on linux we are forced to use Use in order to set pids
	long ShmTokenRingLin::Use(pid_t pid) {
		userPids[userCount] = pid;
		// TODO this critical section
		userCount += 1;
		dbg_log("[ShmTokenRingLin] User %ld with pid %d started using the token ring. Current Usercount %ld this %p\n", userCount - 1, userPids[userCount - 1], userCount, this);
		return userCount - 1;
	}

	bool ShmTokenRingLin::Wait(long int userId, bool blocking) const {
		dbg_log("[ShmTokenRingLin] User %ld waiting for token\n", userId);

		do {
			sigset_t mask;
			long localCurrentOwner = currentOwner;

			if (localCurrentOwner == userId) {
				dbg_log("[ShmTokenRingLin] Wait finished for user %ld\n", userId);
				return true;
			}

			sigfillset(&mask);
			sigdelset(&mask, SIGUSR1);

			int ret = sigsuspend(&mask);
			if (ret < 0) {
				dbg_log("[ipclib] Wait failed with exitcode %d pid %d\n", ret, getpid());
				return false;
			}


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
			dbg_log("[ShmTokenRingLin] User %ld sending signal SIGUSR1 to %ld this %p\n", userId, currentOwner, this);
			int ret = kill(userPids[currentOwner], SIGUSR1);
			if (ret < 0)
				dbg_log("[ShmTokenRingLin] Could not send signal to my neighbour %d.\n", userPids[currentOwner]);

		}

	}
} //namespace ipc
