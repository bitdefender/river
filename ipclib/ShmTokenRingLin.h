#ifndef _SHM_TOKEN_RING_LIN
#define _SHM_TOKEN_RING_LIN

#include <semaphore.h>
#include "common.h"
#define MAX_USER_COUNT 2

namespace ipc {
	class ShmTokenRingLin {
	private:
		sem_t semaphores[MAX_USER_COUNT];
		bool valid[MAX_USER_COUNT];
		sem_t use_semaphore;
		IpcAPI *ipcAPI;
		volatile long currentOwner;
		volatile long userCount;

	public:
		~ShmTokenRingLin();
		void Init(long presetUsers);
		void SetIpcApiHandler(IpcAPI *ipcAPI);

		unsigned long Use(unsigned long id);

		bool Wait(long int userId, bool blocking = true);
		void Release(long userId);
	};

} //namespace ipc

#endif
