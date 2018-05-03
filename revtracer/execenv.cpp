#include "execenv.h"
#include "mm.h"
#include "cb.h"

#include "revtracer.h"

void *ExecutionEnvironment::operator new(size_t sz){
	void *storage = revtracerImports.memoryAllocFunc(sz);
	if (NULL == storage) {
		DEBUG_BREAK;
		return NULL;
	}
	return storage;
}

void ExecutionEnvironment::operator delete(void *ptr) {
	revtracerImports.memoryFreeFunc((nodep::BYTE *)ptr);
}

ExecutionEnvironment::ExecutionEnvironment(nodep::DWORD flags, unsigned int heapSize, unsigned int historySize, unsigned int executionSize, unsigned int trackSize, unsigned int logHashSize, unsigned int outBufferSize) {
	bValid = false;
	generationFlags = flags;
	exitAddr = 0xFFFFCAFE;
	if (0 == heap.Init(heapSize)) {
		return;
	}

	//if (0 == InitBlock(pEnv, logHashSize, historySize)) {
	if (0 == blockCache.Init(&heap, logHashSize, historySize)) {
		heap.Destroy();
		return;
	}

	if (0 == (executionBuffer = (nodep::UINT_PTR *)revtracerImports.memoryAllocFunc(executionSize + trackSize + 4096))) {
		blockCache.Destroy();
		heap.Destroy();
		return;
	}

	if (!codeGen.Init(&heap, &runtimeContext, outBufferSize, generationFlags)) {
		revtracerImports.memoryFreeFunc(executionBuffer);
		blockCache.Destroy();
		heap.Destroy();
		return;
	}

	if (NULL == (pStack = (unsigned char *)revtracerImports.memoryAllocFunc(0x100000))) {
		codeGen.Destroy();
		revtracerImports.memoryFreeFunc(executionBuffer);
		blockCache.Destroy();
		heap.Destroy();
		return;
	}


	rev_memset(pStack, 0, 0x100000);

	runtimeContext.execBuff = (nodep::DWORD)executionBuffer + executionSize - 4; //TODO: make independant track buffer 
	executionBase = runtimeContext.execBuff;

	runtimeContext.trackStack = (nodep::DWORD)executionBuffer + executionSize + trackSize - 4;
	runtimeContext.trackBuff = runtimeContext.trackBase = (nodep::DWORD)executionBuffer + executionSize + trackSize + 4096;
	runtimeContext.virtualStack = (nodep::DWORD)pStack + 0xFFFF0;
	runtimeContext.firstEsp = 0xFFFFFFFF;

	ac.Init();
	//this is a major hack...
	// remove after addres tracking is completely decoupled from the reversible tracking
	runtimeContext.taintedAddresses = (nodep::UINT_PTR)this;

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

ExecutionEnvironment::~ExecutionEnvironment() {
	blockCache.Destroy(); 
	heap.Destroy();

	revtracerImports.memoryFreeFunc((nodep::BYTE *)executionBuffer);

	revtracerImports.memoryFreeFunc((nodep::BYTE *)pStack);
	pStack = NULL;
}

/*void SetUserContext(struct ExecutionEnvironment *pEnv, void *ptr) {
	pEnv->userContext = ptr;
}*/

/*void *AllocUserContext(struct ExecutionEnvironment *pEnv, unsigned int size) {
	if (NULL != pEnv->userContext) {
		return NULL;
	}

	pEnv->userContext = pEnv->heap.Alloc(size);
	rev_memset(pEnv->userContext, 0, size);
	return pEnv->userContext;
}

void DeleteUserContext(struct ExecutionEnvironment *pEnv) {
	if (NULL == pEnv->userContext) {
		return;
	}

	pEnv->heap.Free(pEnv->userContext);
	pEnv->userContext = NULL;
}*/
