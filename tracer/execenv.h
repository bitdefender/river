#ifndef _EXEC_ENV_H
#define _EXEC_ENV_H

//#include <Ntifs.h>
#include <Basetsd.h>
#include "environment.h"
#include "sync.h"


struct _zone;
struct _cb_info;

typedef struct _exec_env {
	UINT_PTR virtualStack; //mandatory first member (used in vm-rm transitions
	UINT_PTR returnRegister; // ax/eax/rax value
	UINT_PTR jumpBuff;

	unsigned int heapSize, historySize, logHashSize, outBufferSize;

	unsigned char *pStack; // = NULL;

	unsigned char *pHeap; //NULL
	struct _zone *pFirstFree; //NULL

	_tbm_mutex cbLock; //  = 0;
	struct _cb_info **hashTable; // = 0

	UINT_PTR *history;
	unsigned long posHist, totHist; // = 0;

	unsigned char *outBuffer; // = NULL;
	void *userContext;
};

struct _exec_env *NewEnv(unsigned int heapSize, unsigned int historySize, unsigned int logHashSize, unsigned int outBufferSize);
void DeleteEnv(struct _exec_env *pEnv);

void *AllocUserContext(struct _exec_env *pEnv, unsigned int size);
void DeleteUserContext(struct _exec_env *pEnv);

#endif
