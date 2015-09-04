#ifndef _RUNTIME_H
#define _RUNTIME_H

#include "extern.h"

/* River runtime context */
/* TODO: make the runtime threadsafe */
struct RiverRuntime {
	UINT_PTR virtualStack;				// + 0x00 - mandatory first member (used in vm-rm transitions)
	UINT_PTR returnRegister;			// + 0x04 - ax/eax/rax value
	UINT_PTR jumpBuff;					// + 0x08
	UINT_PTR execBuff;					// + 0x0C

	DWORD taintedFlags[8];			// + 0x10
	DWORD taintedRegisters[8];		// + 0x30
	UINT_PTR taintedAddresses;		// + 0x50
};

#endif
