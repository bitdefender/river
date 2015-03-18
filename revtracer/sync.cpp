//#include <Wdm.h>
#include "extern.h"
#include "sync.h"

#include <intrin.h>

void TbmMutexLock(_tbm_mutex *mutex) {
	//DbgPrint("LOCK %08x\n", mutex);
	while (1) {
		if (0 == _InterlockedExchange(mutex, 1)) {
			break;
		}

		//Sleep?
	}
}

void TbmMutexUnlock(_tbm_mutex *mutex) {
	//DbgPrint("UNLOCK %08x\n", mutex);
	_InterlockedExchange(mutex, 0);
}