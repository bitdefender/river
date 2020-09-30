#ifndef _RIVER_STRUCTS_H
#define _RIVER_STRUCTS_H

#include "../CommonCrossPlatform/Common.h"

struct RemoteRuntime {
	UINT_PTR virtualStack;				// + 0x00 - mandatory first member (used in vm-rm transitions)
	UINT_PTR returnRegister;			// + 0x04 - ax/eax/rax value
	UINT_PTR jumpBuff;					// + 0x08
	UINT_PTR execBuff;					// + 0x0C
	UINT_PTR trackBuff;					// + 0x10
	UINT_PTR trackBase;					// + 0x14
	UINT_PTR taintedAddresses;			// + 0x18
	UINT_PTR registers;					// + 0x1C

	DWORD taintedFlags[8];			// + 0x20
	DWORD taintedRegisters[8];		// + 0x40
};

//struct RiverExecutionEnvironment {
//	RiverRuntime runtimeContext;
//
//	UINT_PTR saveLog;
//
//	unsigned int /*heapSize,*/ historySize /*, logHashSize*/, outBufferSize;
//
//	unsigned char *pStack; // = NULL;
//
//	RiverHeap heap;
//
//	//_tbm_mutex cbLock; //  = 0;
//	//struct _cb_info **hashTable; // = 0
//	RiverBasicBlockCache blockCache;
//
//	UINT_PTR lastFwBlock;
//	//UINT_PTR *history;
//	//unsigned long posHist, totHist; // = 0;
//
//	UINT_PTR *executionBuffer;
//
//	//unsigned char *saveBuffer;
//
//	unsigned int bForward;
//
//	RiverCodeGen codeGen;
//
//	DWORD exitAddr;
//
//	bool bValid;
//	void *userContext;
//};

#endif
