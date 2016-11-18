#ifndef _ABSTRACT_SHM_TOKEN_RING_
#define	_ABSTRACT_SHM_TOKEN_RING_


typedef signed int pid_t;

namespace ipc {
	class AbstractShmTokenRing {

	public:
		virtual void Init() = 0;
		virtual void Init(long presetUsers) = 0;

		virtual long Use(pid_t pid = -1) = 0;

		virtual bool Wait(long userId, bool blocking = true) const = 0;
		virtual void Release(long userId) = 0;
	};

	AbstractShmTokenRing *AbstractShmTokenRingFactory(void);
} //namespace ipc

#endif
