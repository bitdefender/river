#include "ShmTokenRingLin.h"
#include <signal.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>

#define DONOTPRINT

#ifdef DONOTPRINT
#define dbg_log(fmt,...) ((void)0)
#else
#define dbg_log(fmt,...) { printf(fmt, ##__VA_ARGS__); fflush(stdout); }
#endif


#define DEBUG_BREAK asm volatile("int $0x3")

namespace ipc {

	ShmTokenRingLin::~ShmTokenRingLin() {
		dbg_log("[ShmTokenRingLin] Destructor is called\n");
		((DestroySemaphoreHandler)ipcAPI->destroySemaphore)((void*)&use_semaphore);
		for (int i = 0; i < MAX_USER_COUNT; i++)
			if (valid[i]) {
				((DestroySemaphoreHandler)ipcAPI->destroySemaphore)((void*)&semaphores[i]);
			}
	}

	void dump_sem_mem(sem_t *s) {
		dbg_log("!!!!!!!!!!!!!!!!!!!!!!!!!!!1Dumping mem for semaphore  %p\n", s);
		unsigned char *p = (unsigned char*)s;
		for (int i = 0; i < 16; i++)
			dbg_log("%x ", *(p + i));
		dbg_log("\n");
	}
	void ShmTokenRingLin::Init(long presetUsers) {
		dbg_log("[ShmTokenRingLin] Initialization with %lu presetUsers\n", presetUsers);

		userCount = presetUsers;
		currentOwner = 0;

		((InitSemaphoreHandler)ipcAPI->initSemaphore)((void*)&use_semaphore, 1, 1);

		sem_t s;
		for (int i = 0; i < MAX_USER_COUNT; i++) {
			valid[i] = false;
		}
	}


	void ShmTokenRingLin::SetIpcApiHandler(IpcAPI *ipcAPI) {
		this->ipcAPI = ipcAPI;
	}

	unsigned long ShmTokenRingLin::Use(unsigned long id) {

		int ret = 0;
		if (id >= MAX_USER_COUNT) {
			return -1;
		}

		((GetvalueSemaphoreHandler)ipcAPI->getvalueSemaphore)(&use_semaphore, &ret);
		dbg_log("[ShmTokenRingLin] User %lu tries to start this %p sem value %d\n", id, &use_semaphore, ret);
		ret = ((WaitSemaphoreHandler)ipcAPI->waitSemaphore)(&use_semaphore, true);
		if (ret != 0) {
			dbg_log("[ShmTokenRingLin] Wait for use_semaphore failed\n");
		}
		((GetvalueSemaphoreHandler)ipcAPI->getvalueSemaphore)(&use_semaphore, &ret);
		dbg_log("[ShmTokenRingLin] Sem value after wait is %d\n", ret);
		dump_sem_mem(&semaphores[id]);
		ret = ((InitSemaphoreHandler)ipcAPI->initSemaphore)(&semaphores[id], 1, 0);
		dump_sem_mem(&semaphores[id]);

		dbg_log("[ShmTokenRingLin] Inited sem %lu with ret %d addr %p\n", id, ret, &semaphores[id]);
		valid[id] = true;
		userCount += 1;
		ret = ((PostSemaphoreHandler)ipcAPI->postSemaphore)(&use_semaphore);
		if (ret != 0) {
			dbg_log("[ShmTokenRingLin] Post use_semaphore failed\n");
		}

		((GetvalueSemaphoreHandler)ipcAPI->getvalueSemaphore)(&use_semaphore, &ret);
		dbg_log("[ShmTokenRingLin] User %lu started this  %p sem val %d\n", id, this, ret);
		return id;
	}

	bool ShmTokenRingLin::Wait(long int userId, bool blocking) {

		while(1) {
			dbg_log("[ShmTokenRingLin] User %ld waiting for token sem valid %d\n", userId, (int)valid[userId]);
			int ret = ((WaitSemaphoreHandler)ipcAPI->waitSemaphore)(&semaphores[userId], blocking);
			if (ret != 0) {
				if (!blocking)
					return false;
			} else {
				dump_sem_mem(&semaphores[userId]);
				((GetvalueSemaphoreHandler)ipcAPI->getvalueSemaphore)(&semaphores[userId], &ret);
				dbg_log("[ShmTokenRingLin] !!! User %ld wait finished sem %p sem val = %d\n", userId, &semaphores[userId], ret);
				dump_sem_mem(&semaphores[userId]);
				return true;
			}
		}

	}

	void ShmTokenRingLin::Release(long userId) {

		unsigned long localCurrentOwner = (userId + 1) % userCount;
		if (!valid[localCurrentOwner]) {
			dbg_log("[ShmTokenRingLin] Trying to release invalid semaphore %lu\n", localCurrentOwner);
			return;
		}
		int ret = ((PostSemaphoreHandler)ipcAPI->postSemaphore)(&semaphores[localCurrentOwner]);
		dbg_log("[ShmTokenRingLin] User %lu unlocks sem for %lu ret %d\n", userId, localCurrentOwner, ret);
	}
} //namespace ipc
