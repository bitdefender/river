#ifndef _EXEC_ENV_H
#define _EXEC_ENV_H

#include <Basetsd.h>
#include "environment.h"
#include "sync.h"
#include "river.h"
#include "mm.h"
#include "cb.h"
#include "CodeGen.h"
#include "Runtime.h"

struct _exec_env {
	RiverRuntime runtimeContext;

	UINT_PTR saveLog;
	
	unsigned int /*heapSize,*/ historySize /*, logHashSize*/, outBufferSize;

	unsigned char *pStack; // = NULL;

	RiverHeap heap;

	//_tbm_mutex cbLock; //  = 0;
	//struct _cb_info **hashTable; // = 0
	RiverBasicBlockCache blockCache;

	UINT_PTR lastFwBlock;
	//UINT_PTR *history;
	//unsigned long posHist, totHist; // = 0;

	UINT_PTR *executionBuffer;

	//unsigned char *saveBuffer;

	unsigned int bForward;

	RiverCodeGen codeGen;

	bool bValid;
	void *userContext;

public :
	void* operator new(size_t);
	void operator delete(void*);

	_exec_env(unsigned int heapSize, unsigned int historySize, unsigned int executionSize, unsigned int logHashSize, unsigned int outBufferSize);
	~_exec_env();
};

void *AllocUserContext(struct _exec_env *pEnv, unsigned int size);
void DeleteUserContext(struct _exec_env *pEnv);

#endif
