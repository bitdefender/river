#include "common.h"
#include "callgates.h"
#include "cb.h"
#include "mm.h"
#include "BaseTsd.h"

int Translate(struct _exec_env *pEnv, struct _cb_info *pCB, DWORD dwTranslationFlags);

void Stopper(struct _exec_env *pEnv, BYTE *s) {
	struct _cb_info *pStop;

	// ## STOPPER ##
	DbgPrint("STOPPER: %p.\n", s);
	pStop = NewBlock(pEnv);
	pStop->address = (UINT_PTR) s;
	Translate(pEnv, pStop, 0x80000000); // this will fix pStop->CRC and pStop->Size
	//SC_HeapFree(pEnv, pStop->pCode);
	//pStop->pCode = (BYTE *)s;
	AddBlock(pEnv, pStop);
	// ## STOPPER ##
}


DWORD __declspec(noinline) call_cdecl_0(struct _exec_env *env, _fn_cdecl_0 f) {
	struct _cb_info *pBlock;
	DWORD ret;

	Stopper (env, _ReturnAddress());

	pBlock = NewBlock(env);
	pBlock->address = (DWORD) f;
	Translate(env, pBlock, 0);
	env->bForward = 1;
	TouchBlock(env, pBlock);
	AddBlock(env, pBlock);
	ret = ((_fn_cdecl_0)(pBlock->pCode))(); //JUMP in TVM

	return ret;
}

DWORD __declspec(noinline) call_cdecl_1(struct _exec_env *env, _fn_cdecl_1 f, void *p1) {
	DWORD ret;
	struct _cb_info *pBlock;

	Stopper (env, _ReturnAddress());

	pBlock = NewBlock(env);
	pBlock->address = (DWORD) f;
	Translate(env, pBlock, 0);
	env->bForward = 1;
	TouchBlock(env, pBlock); 
	AddBlock(env, pBlock);

	ret = ((_fn_cdecl_1)(pBlock->pCode))(p1);

	return ret;
}

DWORD __declspec(noinline) call_cdecl_2(struct _exec_env *env, _fn_cdecl_2 f, void *p1, void *p2) {
	DWORD ret;
	struct _cb_info *pBlock;

	Stopper (env, _ReturnAddress());

	pBlock = NewBlock(env);
	pBlock->address = (DWORD) f;
	Translate(env, pBlock, 0);
	env->bForward = 1;
	TouchBlock(env, pBlock); 
	AddBlock(env, pBlock);

	ret = ((_fn_cdecl_2)(pBlock->pCode))(p1, p2);

	return ret;
}

DWORD __declspec(noinline) call_cdecl_3(struct _exec_env *env, _fn_cdecl_3 f, void *p1, void *p2, void *p3) {
	DWORD ret;
	struct _cb_info *pBlock;

	Stopper (env, _ReturnAddress());

	pBlock = NewBlock(env);
	pBlock->address = (DWORD) f;
	Translate(env, pBlock, 0);
	env->bForward = 1;
	TouchBlock(env, pBlock);
	AddBlock(env, pBlock);

	ret = ((_fn_cdecl_3)(pBlock->pCode))(p1, p2, p3);

	return ret;
}

DWORD __declspec(noinline) call_cdecl_4(struct _exec_env *env, _fn_cdecl_4 f, void *p1, void *p2, void *p3, void *p4) {
	DWORD ret;
	struct _cb_info *pBlock;

	Stopper (env, _ReturnAddress());

	pBlock = NewBlock(env);
	pBlock->address = (DWORD) f;
	Translate(env, pBlock, 0);
	env->bForward = 1;
	TouchBlock(env, pBlock);
	AddBlock(env, pBlock);

	ret = ((_fn_cdecl_4)(pBlock->pCode))(p1, p2, p3, p4);
	return ret;
}

DWORD __declspec(noinline) call_stdcall_0(struct _exec_env *env, _fn_stdcall_0 f) {
	struct _cb_info *pBlock;
	DWORD ret;

	Stopper (env, _ReturnAddress());

	pBlock = NewBlock(env);
	pBlock->address = (DWORD) f;
	Translate(env, pBlock, 0);
	env->bForward = 1;
	TouchBlock(env, pBlock);
	AddBlock(env, pBlock);
	ret = ((_fn_stdcall_0)(pBlock->pCode))(); //JUMP in TVM

	return ret;
}

DWORD __declspec(noinline) call_stdcall_1(struct _exec_env *env, _fn_stdcall_1 f, void *p1) {
	DWORD ret;
	struct _cb_info *pBlock;

	Stopper (env, _ReturnAddress());

	pBlock = NewBlock(env);
	pBlock->address = (DWORD) f;
	Translate(env, pBlock, 0);
	env->bForward = 1;
	TouchBlock(env, pBlock);
	AddBlock(env, pBlock);

	ret = ((_fn_stdcall_1)(pBlock->pCode))(p1);

	return ret;
}

DWORD __declspec(noinline) call_stdcall_2(struct _exec_env *env, _fn_stdcall_2 f, void *p1, void *p2) {
	DWORD ret;
	struct _cb_info *pBlock;

	Stopper (env, _ReturnAddress());

	pBlock = NewBlock(env);
	pBlock->address = (DWORD) f;
	Translate(env, pBlock, 0);
	env->bForward = 1;
	TouchBlock(env, pBlock);
	AddBlock(env, pBlock);

	ret = ((_fn_stdcall_2)(pBlock->pCode))(p1, p2);

	return ret;
}

DWORD __declspec(noinline) call_stdcall_3(struct _exec_env *env, _fn_stdcall_3 f, void *p1, void *p2, void *p3) {
	DWORD ret;
	struct _cb_info *pBlock;

	Stopper (env, _ReturnAddress());

	pBlock = NewBlock(env);
	pBlock->address = (DWORD) f;
	Translate(env, pBlock, 0);
	env->bForward = 1;
	TouchBlock(env, pBlock);
	AddBlock(env, pBlock);

	ret = ((_fn_stdcall_3)(pBlock->pCode))(p1, p2, p3);

	return ret;
}

DWORD __declspec(noinline) call_stdcall_4(struct _exec_env *env, _fn_stdcall_4 f, void *p1, void *p2, void *p3, void *p4) {
	DWORD ret;
	struct _cb_info *pBlock;

	Stopper (env, _ReturnAddress());

	pBlock = NewBlock(env);
	pBlock->address = (DWORD) f;
	Translate(env, pBlock, 0);
	env->bForward = 1;
	TouchBlock(env, pBlock);
	AddBlock(env, pBlock);

	ret = ((_fn_stdcall_4)(pBlock->pCode))(p1, p2, p3, p4);
	return ret;
}