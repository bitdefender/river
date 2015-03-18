#include "execenv.h"
#include "mm.h"
#include "cb.h"

//#include "lup.h" //for now
#include "common.h"

struct _exec_env *NewEnv(unsigned int heapSize, unsigned int historySize, unsigned int logHashSize, unsigned int outBufferSize) {
	struct _exec_env *pEnv;

	if (NULL == (pEnv = EnvMemoryAlloc(sizeof(*pEnv)))) {
		return NULL;
	}

	memset(pEnv, 0, sizeof(*pEnv));

	if (0 == SC_HeapInit(pEnv, heapSize)) {
		EnvMemoryFree((BYTE *)pEnv);
		return NULL;
	}

	if (0 == InitBlock(pEnv, logHashSize, historySize)) {
		SC_HeapDestroy(pEnv);
		EnvMemoryFree((BYTE *)pEnv);
		return NULL;
	}

	if (NULL == (pEnv->outBuffer = EnvMemoryAlloc(outBufferSize))) {
		CloseBlock(pEnv);
		SC_HeapDestroy(pEnv);
		EnvMemoryFree((BYTE *)pEnv);
		return NULL;
	}
	pEnv->outBufferSize = outBufferSize;

	if (NULL == (pEnv->pStack = EnvMemoryAlloc (0x100000))) {
		EnvMemoryFree((BYTE *)pEnv->outBuffer);
		CloseBlock(pEnv);
		SC_HeapDestroy(pEnv);
		EnvMemoryFree((BYTE *)pEnv);
		return NULL;
	}

	memset (pEnv->pStack, 0, 0x100000);

	pEnv->virtualStack = (DWORD) pEnv->pStack + 0xFFFF0;

	return pEnv;
}

void DeleteEnv(struct _exec_env *pEnv) {
	if (pEnv != NULL) {
		DeleteUserContext(pEnv);

		CloseBlock(pEnv);
		SC_HeapDestroy(pEnv);

		EnvMemoryFree((BYTE *)pEnv->pStack);
		pEnv->pStack = NULL;

		EnvMemoryFree((BYTE *)pEnv);
		pEnv = NULL;
	}
}

void *AllocUserContext(struct _exec_env *pEnv, unsigned int size) {
	if (NULL != pEnv->userContext) {
		return NULL;
	}

	pEnv->userContext = SC_HeapAlloc(pEnv, size);
	memset(pEnv->userContext, 0, size);
	return pEnv->userContext;
}

void DeleteUserContext(struct _exec_env *pEnv) {
	if (NULL == pEnv->userContext) {
		return;
	}

	SC_HeapFree(pEnv, pEnv->userContext);
	pEnv->userContext = NULL;
}
