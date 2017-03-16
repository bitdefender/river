#ifndef _RUNTIME_H
#define _RUNTIME_H

#include "revtracer.h"

/* River runtime context */
/* TODO: make the runtime threadsafe */
struct RiverRuntime {
	nodep::UINT_PTR virtualStack;				// + 0x00 - mandatory first member (used in vm-rm transitions)
	nodep::UINT_PTR returnRegister;			// + 0x04 - ax/eax/rax value
	nodep::UINT_PTR jumpBuff;					// + 0x08
	nodep::UINT_PTR execBuff;					// + 0x0C
	nodep::UINT_PTR trackBuff;				// + 0x10
	nodep::UINT_PTR trackBase;				// + 0x14
	nodep::UINT_PTR taintedAddresses;			// + 0x18
	nodep::UINT_PTR registers;				// + 0x1C

	nodep::DWORD taintedFlags[8];				// + 0x20
	nodep::DWORD taintedRegisters[8];			// + 0x40

	nodep::UINT_PTR trackStack;				// + 0x60
};

#endif
