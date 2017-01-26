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

		unsigned long Use(unsigned long id);

		bool Wait(long userId, bool blocking = true) const;
		void Release(long userId);
	};

};

#endif
