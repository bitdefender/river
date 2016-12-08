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


#define DEBUG_BREAK asm volatile("int $0x3")

namespace ipc {

	static void SignalHandler(int signo)
	{
		dbg_log("[!!!][ShmTokenRingLin] Caught signal %d in process %d\n", signo, getpid());
	}

	ShmTokenRingLin::ShmTokenRingLin() {
		// happens when ipclib is loaded in memory
		dbg_log("[ShmTokenRingLin] Constructor is called when loading libipc\n");
		SetupSignalHandler();
		Init(0);
	}

	int ShmTokenRingLin::SetupSignalHandler() {
		struct sigaction sa;

		memset(&sa, 0, sizeof(sa));

		sa.sa_handler = SignalHandler;
		int ret = sigaction(SIGUSR1, &sa, NULL);
		if (ret < 0) {
			dbg_log("[ShmTokenRingLin] Failed to setup signal handler\n");
		}
		dbg_log("[ShmTokenRingLin] Signal handler setup exits with code %d\n", ret);
		return ret;
	}

	void ShmTokenRingLin::Init(long presetUsers) {
		userCount = presetUsers;
		currentOwner = 0;
		for (int i = 0; i < MAX_USER_COUNT; i++)
			userPids[i] = -1;
	}

	// on linux we are forced to use Use in order to set pids
	long ShmTokenRingLin::Use(unsigned int id, pid_t pid) {
		userPids[id] = pid;
		// TODO this critical section
		userCount += 1;
		dbg_log("[ShmTokenRingLin] User %d with pid %d started using the token ring. Current Usercount %ld\n", id, userPids[id], userCount);
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
				DEBUG_BREAK;
				return true;
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
			if (userPids[currentOwner] == -1) {
				dbg_log("[ShmTokenRingLin] Could not send signal because my neighbour is not connected %ld.\n", currentOwner);
				return;
			}

			int ret = kill(userPids[currentOwner], SIGUSR1);
			dbg_log("[ShmTokenRingLin] User %ld sending signal SIGUSR1 to %ld this %p\n", userId, currentOwner, this);
			if (ret < 0)
				dbg_log("[ShmTokenRingLin] Could not send signal to my neighbour %d.\n", userPids[currentOwner]);

		}

	}
} //namespace ipc
