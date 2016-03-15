#ifndef _RUNTIME_H
#define _RUNTIME_H

#include "revtracer.h"

using namespace rev;

/* River runtime context */
/* TODO: make the runtime threadsafe */
struct RiverRuntime {
	UINT_PTR virtualStack;				// + 0x00 - mandatory first member (used in vm-rm transitions)
	UINT_PTR returnRegister;			// + 0x04 - ax/eax/rax value
	UINT_PTR jumpBuff;					// + 0x08
	UINT_PTR execBuff;					// + 0x0C
	UINT_PTR trackBuff;					// + 0x10
	UINT_PTR trackBase;					// + 0x14
	UINT_PTR taintedAddresses;			// + 0x18
	UINT_PTR registers;					// + 0x1C

	DWORD taintedFlags[8];				// + 0x20
	DWORD taintedRegisters[8];			// + 0x40

	UINT_PTR trackStack;				// + 0x60
};

#endif
