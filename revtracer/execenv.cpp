#include "execenv.h"
#include "mm.h"
#include "cb.h"

//#include "lup.h" //for now
#include "extern.h"

struct _exec_env *NewEnv(unsigned int heapSize, unsigned int historySize, unsigned int executionSize, unsigned int logHashSize, unsigned int outBufferSize) {
	struct _exec_env *pEnv;

	if (NULL == (pEnv = (struct _exec_env *)EnvMemoryAlloc(sizeof(*pEnv)))) {
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

	if (0 == (pEnv->executionBuffer = (UINT_PTR *)EnvMemoryAlloc(executionSize))) {
		CloseBlock(pEnv);
		SC_HeapDestroy(pEnv);
		EnvMemoryFree((BYTE *)pEnv);
		return NULL;
	}

	if (NULL == (pEnv->outBuffer = (unsigned char *)EnvMemoryAlloc(outBufferSize))) {
		EnvMemoryFree(pEnv->executionBuffer);
		CloseBlock(pEnv);
		SC_HeapDestroy(pEnv);
		EnvMemoryFree((BYTE *)pEnv);
		return NULL;
	}
	pEnv->outBufferSize = outBufferSize;

	if (NULL == (pEnv->pStack = (unsigned char *)EnvMemoryAlloc (0x100000))) {
		EnvMemoryFree((BYTE *)pEnv->outBuffer);
		EnvMemoryFree(pEnv->executionBuffer); 
		CloseBlock(pEnv);
		SC_HeapDestroy(pEnv);
		EnvMemoryFree((BYTE *)pEnv);
		return NULL;
	}

	memset (pEnv->pStack, 0, 0x100000);

	pEnv->execBuff = (DWORD)pEnv->executionBuffer + executionSize;
	pEnv->virtualStack = (DWORD) pEnv->pStack + 0xFFFF0;

	return pEnv;
}

void DeleteEnv(struct _exec_env *pEnv) {
	if (pEnv != NULL) {
		DeleteUserContext(pEnv);

		CloseBlock(pEnv);
		SC_HeapDestroy(pEnv);

		EnvMemoryFree((BYTE *)pEnv->executionBuffer);

		EnvMemoryFree((BYTE *)pEnv->pStack);
		pEnv->pStack = NULL;

		EnvMemoryFree((BYTE *)pEnv);
		pEnv = NULL;
	}
}

struct RiverAddress *AllocRiverAddr(struct _exec_env *pEnv) {
	struct RiverAddress *ret = &pEnv->trRiverAddr[pEnv->addrCount];
	pEnv->addrCount++;
	return ret;
}

void RiverMemReset(struct _exec_env *pEnv) {
	pEnv->addrCount = pEnv->trInstCount = pEnv->fwInstCount = pEnv->bkInstCount = 0;
}

void ResetRegs(struct _exec_env *pEnv) {
	memset(pEnv->regVersions, 0, sizeof(pEnv->regVersions));
}

unsigned int GetCurrentReg(struct _exec_env *pEnv, unsigned char regName) {
	return pEnv->regVersions[regName] | regName;
}

unsigned int GetPrevReg(struct _exec_env *pEnv, unsigned char regName) {
	return (pEnv->regVersions[regName] - 0x100) | regName;
}

unsigned int NextReg(struct _exec_env *pEnv, unsigned char regName) {
	pEnv->regVersions[regName] += 0x100;
	return GetCurrentReg(pEnv, regName);
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

	SC_HeapFree(pEnv, (unsigned char *)pEnv->userContext);
	pEnv->userContext = NULL;
}
