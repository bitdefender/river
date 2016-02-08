#include "execenv.h"
#include "mm.h"
#include "cb.h"

//#include "lup.h" //for now
#include "revtracer.h"

void *_exec_env::operator new(size_t sz){
	void *storage = revtracerAPI.memoryAllocFunc(sz);
	if (NULL == storage) {
		return NULL;
	}
	return storage;
}

void _exec_env::operator delete(void *ptr) {
	revtracerAPI.memoryFreeFunc((BYTE *)ptr);
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

	if (0 == (executionBuffer = (UINT_PTR *)revtracerAPI.memoryAllocFunc(executionSize))) {
		blockCache.Destroy();
		heap.Destroy();
		return;
	}

	if (!codeGen.Init(&heap, &runtimeContext, outBufferSize)) {
		revtracerAPI.memoryFreeFunc(executionBuffer);
		blockCache.Destroy();
		heap.Destroy();
		return;
	}

	if (NULL == (pStack = (unsigned char *)revtracerAPI.memoryAllocFunc(0x100000))) {
		codeGen.Destroy();
		revtracerAPI.memoryFreeFunc(executionBuffer);
		blockCache.Destroy();
		heap.Destroy();
		return;
	}

	memset(pStack, 0, 0x100000);

	runtimeContext.execBuff = (DWORD)executionBuffer + executionSize - 4096; //TODO: make independant track buffer 
	executionBase = runtimeContext.execBuff;
	runtimeContext.trackBuff = runtimeContext.trackBase = (DWORD)executionBuffer + executionSize;
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

	revtracerAPI.memoryFreeFunc((BYTE *)executionBuffer);

	revtracerAPI.memoryFreeFunc((BYTE *)pStack);
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
