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

	if (0 == pEnv->heap.Init(heapSize)) {
		EnvMemoryFree((BYTE *)pEnv);
		return NULL;
	}

	//if (0 == InitBlock(pEnv, logHashSize, historySize)) {
	if (0 == pEnv->blockCache.Init(&pEnv->heap, logHashSize, historySize)) {
		pEnv->heap.Destroy();
		EnvMemoryFree((BYTE *)pEnv);
		return NULL;
	}

	if (0 == (pEnv->executionBuffer = (UINT_PTR *)EnvMemoryAlloc(executionSize))) {
		pEnv->blockCache.Destroy();
		pEnv->heap.Destroy();
		EnvMemoryFree((BYTE *)pEnv);
		return NULL;
	}

	if (!pEnv->codeGen.Init(&pEnv->heap, outBufferSize)) {
		EnvMemoryFree(pEnv->executionBuffer);
		pEnv->blockCache.Destroy();
		pEnv->heap.Destroy();
		EnvMemoryFree((BYTE *)pEnv);
		return NULL;
	}
	
	if (NULL == (pEnv->pStack = (unsigned char *)EnvMemoryAlloc (0x100000))) {
		pEnv->codeGen.Destroy();
		EnvMemoryFree(pEnv->executionBuffer); 
		pEnv->blockCache.Destroy(); 
		pEnv->heap.Destroy();
		EnvMemoryFree((BYTE *)pEnv);
		return NULL;
	}

	memset (pEnv->pStack, 0, 0x100000);

	pEnv->runtimeContext.execBuff = (DWORD)pEnv->executionBuffer + executionSize;
	pEnv->runtimeContext.virtualStack = (DWORD)pEnv->pStack + 0xFFFF0;

	return pEnv;
}

void DeleteEnv(struct _exec_env *pEnv) {
	if (pEnv != NULL) {
		DeleteUserContext(pEnv);

		pEnv->blockCache.Destroy(); 
		pEnv->heap.Destroy();

		EnvMemoryFree((BYTE *)pEnv->executionBuffer);

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

	pEnv->userContext = pEnv->heap.Alloc(size);
	memset(pEnv->userContext, 0, size);
	return pEnv->userContext;
}

void DeleteUserContext(struct _exec_env *pEnv) {
	if (NULL == pEnv->userContext) {
		return;
	}

	pEnv->heap.Free(pEnv->userContext);
	pEnv->userContext = NULL;
}
