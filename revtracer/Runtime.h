#ifndef _RUNTIME_H
#define _RUNTIME_H

#include "revtracer.h"

#define RIVER_RUNTIME_EXCEPTION_FLAG		0x01

/* River runtime context */
/* TODO: make the runtime threadsafe */
struct RiverRuntime {
	rev::UINT_PTR virtualStack;				// + 0x00 - mandatory first member (used in vm-rm transitions)
	rev::UINT_PTR returnRegister;			// + 0x04 - ax/eax/rax value
	rev::UINT_PTR jumpBuff;					// + 0x08
	rev::UINT_PTR execBuff;					// + 0x0C
	rev::UINT_PTR trackBuff;				// + 0x10
	rev::UINT_PTR trackBase;				// + 0x14
	rev::UINT_PTR taintedAddresses;			// + 0x18
	rev::UINT_PTR registers;				// + 0x1C

	rev::DWORD taintedFlags[8];				// + 0x20
	rev::DWORD taintedRegisters[8];			// + 0x40

	rev::UINT_PTR trackStack;				// + 0x60
	rev::UINT_PTR exceptionStack;			// + 0x64
	rev::UINT_PTR execFlags;				// + 0x68
};

#endif
