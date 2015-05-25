#include "execenv.h"
#include "mm.h"
#include "cb.h"

//#include "lup.h" //for now
#include "extern.h"

void *_exec_env::operator new(size_t sz){
	void *storage = EnvMemoryAlloc(sz);
	if (NULL == storage) {
		throw "alloc fail";
	}
	return storage;
}

void _exec_env::operator delete(void *ptr) {
	EnvMemoryFree((BYTE *)ptr);
}

_exec_env::_exec_env(unsigned int heapSize, unsigned int historySize, unsigned int executionSize, unsigned int logHashSize, unsigned int outBufferSize) {
	bValid = false;
	exitAddr = 0xFFFFCAFE;
	if (0 == heap.Init(heapSize)) {
		return;
	}

	//if (0 == InitBlock(pEnv, logHashSize, historySize)) {
	if (0 == blockCache.Init(&heap, logHashSize, historySize)) {
		heap.Destroy();
		return;
	}

	if (0 == (executionBuffer = (UINT_PTR *)EnvMemoryAlloc(executionSize))) {
		blockCache.Destroy();
		heap.Destroy();
		return;
	}

	if (!codeGen.Init(&heap, &runtimeContext, outBufferSize)) {
		EnvMemoryFree(executionBuffer);
		blockCache.Destroy();
		heap.Destroy();
		return;
	}

	if (NULL == (pStack = (unsigned char *)EnvMemoryAlloc(0x100000))) {
		codeGen.Destroy();
		EnvMemoryFree(executionBuffer);
		blockCache.Destroy();
		heap.Destroy();
		return;
	}

	memset(pStack, 0, 0x100000);

	runtimeContext.execBuff = (DWORD)executionBuffer + executionSize;
	runtimeContext.virtualStack = (DWORD)pStack + 0xFFFF0;
	bValid = true;
}

/*struct _exec_env *_exec_env::NewEnv(unsigned int heapSize, unsigned int historySize, unsigned int executionSize, unsigned int logHashSize, unsigned int outBufferSize) {
	struct _exec_env *pEnv;

	if (NULL == (pEnv = (struct _exec_env *)EnvMemoryAlloc(sizeof(*pEnv)))) {
		return NULL;
	}

	struct _exec_env *tEnv = new (pEnv)_exec_env(heapSize, historySize, executionSize, logHashSize, outBufferSize);

	if ((NULL == tEnv) || (!tEnv->bValid)) {
		EnvMemoryFree((BYTE *)pEnv);
		return NULL;
	}

	return pEnv;
}*/

_exec_env::~_exec_env() {
	DeleteUserContext(this);

	blockCache.Destroy(); 
	heap.Destroy();

	EnvMemoryFree((BYTE *)executionBuffer);

	EnvMemoryFree((BYTE *)pStack);
	pStack = NULL;
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
