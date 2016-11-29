#ifndef _SHM_TOKEN_RING_LIN
#define _SHM_TOKEN_RING_LIN

#include <unistd.h>
#define MAX_USER_COUNT 10

namespace ipc {
	class ShmTokenRingLin {
	private:
		volatile long currentOwner;
		volatile long userCount;
		pid_t userPids[MAX_USER_COUNT];

	public:
		void Init(long presetUsers);

		long Use(pid_t pid = -1);

		bool Wait(long int userId, bool blocking = true) const;
		void Release(long userId);
	};

};

#endif
