#ifndef _ABSTRACT_TOKEN_RING_
#define	_ABSTRACT_TOKEN_RING_

namespace revwrapper {
	
	typedef bool(*WaitEventFunc)(void *handle, int timeout);
	typedef bool(*PostEventFunc)(void *handle);

#define RING_OS_DATA_SIZE 64


	struct TokenRing;

	struct TokenRingOps {
		typedef bool(*WaitFunc)(TokenRing *_this, long userId, bool blocking);
		typedef void(*ReleaseFunc)(TokenRing *_this, long userId);
		
		WaitFunc __Wait;
		ReleaseFunc __Release;
	};

	struct TokenRing {
		TokenRingOps *ops;
		unsigned char osData[RING_OS_DATA_SIZE];

		//friend bool InitTokenRing(TokenRing *_this, long userCount, unsigned int *pids, long token);
		//friend struct TokenRingOps;
		
		bool Wait(long userId, bool blocking = true) {
			return ops->__Wait(this, userId, blocking);
		}

		void Release(long userId) {
			ops->__Release(this, userId);
		}
	};

	
} //namespace ipc

#endif
