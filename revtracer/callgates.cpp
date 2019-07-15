#include <x86intrin.h>
#define __declspec(noinline) __attribute__ ((noinline))

#include "common.h"
#include "callgates.h"
#include "cb.h"
#include "mm.h"
//#include "BaseTsd.h"

#include "execenv.h"

#include "revtracer.h"

using namespace nodep;

//void DbgPrint(const char *fmt, ...);

void Stopper(struct ExecutionEnvironment *pEnv, BYTE *s) {
	RiverBasicBlock *pStop;
	RevtracerError rerror;

	revtracerImports.dbgPrintFunc(PRINT_INFO | PRINT_CONTAINER, "STOPPER: %p.\n", s);
	pStop = pEnv->blockCache.NewBlock((UINT_PTR)s);
	pEnv->codeGen.Translate(pStop, 0x80000000, &rerror); // this will fix pStop->CRC and pStop->Size
	pEnv->exitAddr = (DWORD)s;
}

void SetEsp(struct ExecutionEnvironment *pEnv, nodep::DWORD esp) {
	pEnv->runtimeContext.firstEsp = esp;
}

#ifdef _MSC_VER
#define GET_RETURN_ADDR _ReturnAddress
#define CALLING_CONV(conv) __##conv
#else
#define GET_RETURN_ADDR() ({ int addr; asm volatile("mov 4(%%ebp), %0" : "=r" (addr)); addr; })
#define GET_ESP() ({ int esp; asm volatile("mov %%esp, %0" : "=r" (esp)); esp; })
#define CALLING_CONV(conv) __attribute__((conv))
#endif

#define _RET_ADDR_FUNC_2(conv, paramCount, ...) \
	nodep::DWORD CALLING_CONV(conv) RetAddr_##conv##_##paramCount (__VA_ARGS__) { \
		return (nodep::DWORD)GET_RETURN_ADDR(); \
	}

#define _RET_ADDR_FUNC_(conv, paramCount, ...) _RET_ADDR_FUNC_2(conv, paramCount, __VA_ARGS__)

nodep::DWORD CALLING_CONV(naked) EspAddr () {
	return (nodep::DWORD)GET_ESP();
}


_RET_ADDR_FUNC_(cdecl, 0);
_RET_ADDR_FUNC_(cdecl, 1, void *);
_RET_ADDR_FUNC_(cdecl, 2, void *, void *);
_RET_ADDR_FUNC_(cdecl, 3, void *, void *, void *);
_RET_ADDR_FUNC_(cdecl, 4, void *, void *, void *, void *);

_RET_ADDR_FUNC_(stdcall, 0);
_RET_ADDR_FUNC_(stdcall, 1, void *);
_RET_ADDR_FUNC_(stdcall, 2, void *, void *);
_RET_ADDR_FUNC_(stdcall, 3, void *, void *, void *);
_RET_ADDR_FUNC_(stdcall, 4, void *, void *, void *, void *);

nodep::DWORD __declspec(noinline) call_cdecl_0(struct ExecutionEnvironment *env, _fn_cdecl_0 f) {
	DWORD ret;
	RevtracerError rerror;
	RiverBasicBlock *pBlock;

	pBlock = env->blockCache.NewBlock((UINT_PTR)f);
	//pBlock->address = (DWORD) f;
	env->codeGen.Translate(pBlock, env->generationFlags, &rerror);
	env->bForward = 1;
	env->lastFwBlock = (DWORD)f;
	pBlock->MarkForward();
	//AddBlock(env, pBlock);

	_fn_cdecl_0 funcs[] = {
		EspAddr,
		RetAddr_cdecl_0,
		(_fn_cdecl_0)(pBlock->pFwCode)

	};

	for (int i = 0; i < 3; ++i) {
		ret = (funcs[i])();
		if (0 == i) {
			SetEsp(env, ret);
		} else if (1 == i) {
			Stopper(env, (BYTE *)ret);
		}
	}

	return ret;
}

DWORD __declspec(noinline) call_cdecl_1(struct ExecutionEnvironment *env, _fn_cdecl_1 f, void *p1) {
	DWORD ret;
	RevtracerError rerror;
	RiverBasicBlock *pBlock;

	pBlock = env->blockCache.NewBlock((UINT_PTR)f);
	pBlock->address = (DWORD) f;
	env->codeGen.Translate(pBlock, env->generationFlags, &rerror);
	env->bForward = 1;
	env->lastFwBlock = (DWORD)f;
	pBlock->MarkForward();
	//AddBlock(env, pBlock);

	_fn_cdecl_1 funcs[] = {
		RetAddr_cdecl_1,
		(_fn_cdecl_1)(pBlock->pFwCode)

	};

	for (int i = 0; i < 2; ++i) {
		ret = (funcs[i])(p1);
		if (0 == i) {
			Stopper(env, (BYTE *)ret);
		}
	}

	return ret;
}

DWORD __declspec(noinline) call_cdecl_2(struct ExecutionEnvironment *env, _fn_cdecl_2 f, void *p1, void *p2) {
	DWORD ret;
	RevtracerError rerror;
	RiverBasicBlock *pBlock; 
	
	pBlock = env->blockCache.NewBlock((UINT_PTR)f);
	pBlock->address = (DWORD) f;
	env->codeGen.Translate(pBlock, env->generationFlags, &rerror);
	env->bForward = 1;
	env->lastFwBlock = (DWORD)f;
	pBlock->MarkForward();
	//AddBlock(env, pBlock);

	_fn_cdecl_2 funcs[] = {
		RetAddr_cdecl_2,
		(_fn_cdecl_2)(pBlock->pFwCode)

	};

	for (int i = 0; i < 2; ++i) {
		ret = (funcs[i])(p1, p2);
		if (0 == i) {
			Stopper(env, (BYTE *)ret);
		}
	}

	return ret;
}

DWORD __declspec(noinline) call_cdecl_3(struct ExecutionEnvironment *env, _fn_cdecl_3 f, void *p1, void *p2, void *p3) {
	DWORD ret;
	RevtracerError rerror;
	RiverBasicBlock *pBlock;

	pBlock = env->blockCache.NewBlock((UINT_PTR)f);
	pBlock->address = (DWORD) f;
	env->codeGen.Translate(pBlock, env->generationFlags, &rerror);
	env->bForward = 1;
	env->lastFwBlock = (DWORD)f;
	pBlock->MarkForward();
	//AddBlock(env, pBlock);

	_fn_cdecl_3 funcs[] = {
		RetAddr_cdecl_3,
		(_fn_cdecl_3)(pBlock->pFwCode)

	};

	for (int i = 0; i < 2; ++i) {
		ret = (funcs[i])(p1, p2, p3);
		if (0 == i) {
			Stopper(env, (BYTE *)ret);
		}
	}

	return ret;
}

DWORD __declspec(noinline) call_cdecl_4(struct ExecutionEnvironment *env, _fn_cdecl_4 f, void *p1, void *p2, void *p3, void *p4) {
	DWORD ret;
	RevtracerError rerror;
	RiverBasicBlock *pBlock;

	pBlock = env->blockCache.NewBlock((UINT_PTR)f);
	pBlock->address = (DWORD) f;
	env->codeGen.Translate(pBlock, env->generationFlags, &rerror);
	env->bForward = 1;
	env->lastFwBlock = (DWORD)f;
	pBlock->MarkForward();
	//AddBlock(env, pBlock);

	_fn_cdecl_4 funcs[] = {
		RetAddr_cdecl_4,
		(_fn_cdecl_4)(pBlock->pFwCode)

	};

	for (int i = 0; i < 2; ++i) {
		ret = (funcs[i])(p1, p2, p3, p4);
		if (0 == i) {
			Stopper(env, (BYTE *)ret);
		}
	}

	return ret;
}

DWORD __declspec(noinline) call_stdcall_0(struct ExecutionEnvironment *env, _fn_stdcall_0 f) {
	RiverBasicBlock *pBlock;
	RevtracerError rerror;
	DWORD ret;

	pBlock = env->blockCache.NewBlock((UINT_PTR)f);
	pBlock->address = (DWORD) f;
	env->codeGen.Translate(pBlock, env->generationFlags, &rerror);
	env->bForward = 1;
	env->lastFwBlock = (DWORD)f;
	pBlock->MarkForward();
	//AddBlock(env, pBlock);

	_fn_stdcall_0 funcs[] = {
		RetAddr_stdcall_0,
		(_fn_stdcall_0)(pBlock->pFwCode)

	};

	for (int i = 0; i < 2; ++i) {
		ret = (funcs[i])();
		if (0 == i) {
			Stopper(env, (BYTE *)ret);
		}
	}

	return ret;
}

DWORD __declspec(noinline) call_stdcall_1(struct ExecutionEnvironment *env, _fn_stdcall_1 f, void *p1) {
	DWORD ret;
	RevtracerError rerror;
	RiverBasicBlock *pBlock;

	pBlock = env->blockCache.NewBlock((UINT_PTR)f);
	pBlock->address = (DWORD) f;
	env->codeGen.Translate(pBlock, env->generationFlags, &rerror);
	env->bForward = 1;
	env->lastFwBlock = (DWORD)f;
	pBlock->MarkForward();
	//AddBlock(env, pBlock);

	_fn_stdcall_1 funcs[] = {
		RetAddr_stdcall_1,
		(_fn_stdcall_1)(pBlock->pFwCode)

	};

	for (int i = 0; i < 2; ++i) {
		ret = (funcs[i])(p1);
		if (0 == i) {
			Stopper(env, (BYTE *)ret);
		}
	}

	return ret;
}

DWORD __declspec(noinline) call_stdcall_2(struct ExecutionEnvironment *env, _fn_stdcall_2 f, void *p1, void *p2) {
	DWORD ret;
	RevtracerError rerror;
	RiverBasicBlock *pBlock;

	pBlock = env->blockCache.NewBlock((UINT_PTR)f);
	pBlock->address = (DWORD) f;
	env->codeGen.Translate(pBlock, env->generationFlags, &rerror);
	env->bForward = 1;
	env->lastFwBlock = (DWORD)f;
	pBlock->MarkForward();
	//AddBlock(env, pBlock);

	_fn_stdcall_2 funcs[] = {
		RetAddr_stdcall_2,
		(_fn_stdcall_2)(pBlock->pFwCode)

	};

	for (int i = 0; i < 2; ++i) {
		ret = (funcs[i])(p1, p2);
		if (0 == i) {
			Stopper(env, (BYTE *)ret);
		}
	}

	return ret;
}

DWORD __declspec(noinline) call_stdcall_3(struct ExecutionEnvironment *env, _fn_stdcall_3 f, void *p1, void *p2, void *p3) {
	DWORD ret;
	RevtracerError rerror;
	RiverBasicBlock *pBlock;

	pBlock = env->blockCache.NewBlock((UINT_PTR)f);
	pBlock->address = (DWORD) f;
	env->codeGen.Translate(pBlock, env->generationFlags, &rerror);
	env->bForward = 1;
	env->lastFwBlock = (DWORD)f;
	pBlock->MarkForward();
	//AddBlock(env, pBlock);

	_fn_stdcall_3 funcs[] = {
		RetAddr_stdcall_3,
		(_fn_stdcall_3)(pBlock->pFwCode)

	};

	for (int i = 0; i < 2; ++i) {
		ret = (funcs[i])(p1, p2, p3);
		if (0 == i) {
			Stopper(env, (BYTE *)ret);
		}
	}

	return ret;
}

DWORD __declspec(noinline) call_stdcall_4(struct ExecutionEnvironment *env, _fn_stdcall_4 f, void *p1, void *p2, void *p3, void *p4) {
	DWORD ret;
	RevtracerError rerror;
	RiverBasicBlock *pBlock;

	pBlock = env->blockCache.NewBlock((UINT_PTR)f);
	pBlock->address = (DWORD) f;
	env->codeGen.Translate(pBlock, env->generationFlags, &rerror);
	env->bForward = 1;
	env->lastFwBlock = (DWORD)f;
	pBlock->MarkForward();
	//AddBlock(env, pBlock);

	_fn_stdcall_4 funcs[] = {
		RetAddr_stdcall_4,
		(_fn_stdcall_4)(pBlock->pFwCode)

	};

	for (int i = 0; i < 2; ++i) {
		ret = (funcs[i])(p1, p2, p3, p4);
		if (0 == i) {
			Stopper(env, (BYTE *)ret);
		}
	}

	return ret;
}
