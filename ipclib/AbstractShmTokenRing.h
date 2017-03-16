#ifndef _ABSTRACT_TOKEN_RING_
#define	_ABSTRACT_TOKEN_RING_

namespace ipc {
	
	typedef bool(*WaitEventFunc)(void *handle, int timeout);
	typedef bool(*PostEventFunc)(void *handle);

	class AbstractTokenRing;
	typedef bool(*TokenRingWaitFunc)(AbstractTokenRing *ring, long userId, bool blocking);
	typedef void(*TokenRingReleaseFunc)(AbstractTokenRing *ring, long userId);
	
	class AbstractTokenRing {
	protected:
		WaitEventFunc wait;
		PostEventFunc post;

		TokenRingWaitFunc trw;
		TokenRingReleaseFunc trr;
	public:
		bool Wait(long userId, bool blocking = true) {
			return trw(this, userId, blocking);
		}

		void Release(long userId) {
			trr(this, userId);
		}
	};
} //namespace ipc

#endif
