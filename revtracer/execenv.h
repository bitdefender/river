#ifndef _EXEC_ENV_H
#define _EXEC_ENV_H

#include <Basetsd.h>
#include "environment.h"
#include "sync.h"
#include "virtualreg.h"
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


	void *userContext;
};

struct _exec_env *NewEnv(unsigned int heapSize, unsigned int historySize, unsigned int executionSize, unsigned int logHashSize, unsigned int outBufferSize);
void DeleteEnv(struct _exec_env *pEnv);

void *AllocUserContext(struct _exec_env *pEnv, unsigned int size);
void DeleteUserContext(struct _exec_env *pEnv);

#endif
