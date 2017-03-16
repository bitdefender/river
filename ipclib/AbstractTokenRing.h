#ifndef _ABSTRACT_TOKEN_RING_
#define	_ABSTRACT_TOKEN_RING_

namespace ipc {
	class AbstractTokenRing {
	public:
		virtual bool Wait(long userId, bool blocking = true) const = 0;
		virtual void Release(long userId) = 0;
	};

	class AbstractTokenRingInit : AbstractTokenRing {
	public :
		virtual bool Init(long userCount, long token) = 0;
	};

	AbstractTokenRing *AbstractTokenRingFactory(void);
} //namespace ipc

#endif
