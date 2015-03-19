//#include <Wdm.h>
#include "extern.h"
#include "sync.h"

#include <intrin.h>

RiverMutex::RiverMutex() {
	mtx = 0;
}

RiverMutex::~RiverMutex() {
}

void RiverMutex::Lock() {
	while (1) {
		if (0 == _InterlockedExchange(&mtx, 1)) {
			break;
		}

		//Sleep?
	}
}

void RiverMutex::Unlock() {
	//DbgPrint("UNLOCK %08x\n", mutex);
	_InterlockedExchange(&mtx, 0);
}
