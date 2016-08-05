#ifndef _CALL_GATES_H
#define _CALL_GATES_H

#include "environment.h"
#include "revtracer.h"

typedef rev::DWORD (_cdecl *_fn_cdecl_0) (void);
typedef rev::DWORD (_cdecl *_fn_cdecl_1) (void *);
typedef rev::DWORD (_cdecl *_fn_cdecl_2) (void *, void *);
typedef rev::DWORD (_cdecl *_fn_cdecl_3) (void *, void *, void *);
typedef rev::DWORD (_cdecl *_fn_cdecl_4) (void *, void *, void *, void *);

typedef rev::DWORD (_stdcall *_fn_stdcall_0) (void);
typedef rev::DWORD (_stdcall *_fn_stdcall_1) (void *);
typedef rev::DWORD (_stdcall *_fn_stdcall_2) (void *, void *);
typedef rev::DWORD (_stdcall *_fn_stdcall_3) (void *, void *, void *);
typedef rev::DWORD (_stdcall *_fn_stdcall_4) (void *, void *, void *, void *);

rev::DWORD __declspec(noinline) call_cdecl_0(struct ExecutionEnvironment *env, _fn_cdecl_0 f);
rev::DWORD __declspec(noinline) call_cdecl_1(struct ExecutionEnvironment *env, _fn_cdecl_1 f, void *);
rev::DWORD __declspec(noinline) call_cdecl_2(struct ExecutionEnvironment *env, _fn_cdecl_2 f, void *, void *);
rev::DWORD __declspec(noinline) call_cdecl_3(struct ExecutionEnvironment *env, _fn_cdecl_3 f, void *, void *, void *);
rev::DWORD __declspec(noinline) call_cdecl_4(struct ExecutionEnvironment *env, _fn_cdecl_4 f, void *, void *, void *, void *);

rev::DWORD __declspec(noinline) call_stdcall_0(struct ExecutionEnvironment *env, _fn_stdcall_0 f);
rev::DWORD __declspec(noinline) call_stdcall_1(struct ExecutionEnvironment *env, _fn_stdcall_1 f, void *);
rev::DWORD __declspec(noinline) call_stdcall_2(struct ExecutionEnvironment *env, _fn_stdcall_2 f, void *, void *);
rev::DWORD __declspec(noinline) call_stdcall_3(struct ExecutionEnvironment *env, _fn_stdcall_3 f, void *, void *, void *);
rev::DWORD __declspec(noinline) call_stdcall_4(struct ExecutionEnvironment *env, _fn_stdcall_4 f, void *, void *, void *, void *);



#endif
