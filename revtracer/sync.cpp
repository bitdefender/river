//#include <Wdm.h>
#include "revtracer.h"
#include "sync.h"

//using namespace rev;
#ifdef _MSC_VER
#include <intrin.h>
#define LOCK_XCHG(target, value) _InterlockedExchange(target, value)
#else
#define LOCK_XCHG(target, value) asm volatile("xchgl %0, %1" : "+m"(*target) : "r"(value) : "memory")
#endif

RiverMutex::RiverMutex() {
	mtx = 0;
}

RiverMutex::~RiverMutex() {
}

void RiverMutex::Lock() {
	while (1) {
    LOCK_XCHG(&mtx, 1);
		if (0 == mtx) {
			break;
		}

		//Sleep?
	}
}

void RiverMutex::Unlock() {
	//DbgPrint("UNLOCK %08x\n", mutex);
	LOCK_XCHG(&mtx, 0);
}
