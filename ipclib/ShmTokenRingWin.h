#ifndef _SHM_TOKEN_RING_WIN
#define _SHM_TOKEN_RING_WIN

namespace ipc {
	class ShmTokenRingWin {
	private:
		volatile long currentOwner;
		volatile long userCount;

	public:
		void Init();
		void Init(long presetUsers);

		long Use(pid_t pid = -1);

		bool Wait(long userId, bool blocking = true) const;
		void Release(long userId);
	};

};

#endif
