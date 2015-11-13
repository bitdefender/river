#ifndef _SHM_TOKEN_RING_
#define _SHM_TOKEN_RING_

namespace ipc {
	class ShmTokenRing {
	private:
		volatile long currentOwner;
		volatile long userCount;

	public:
		void Init();
		void Init(long presetUsers);

		long Use();

		void Wait(long userId) const;
		void Release(long userId);
	};

};

#endif
