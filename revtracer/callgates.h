#ifndef _CALL_GATES_H
#define _CALL_GATES_H

#include "environment.h"
#include "revtracer.h"

using namespace rev;

typedef DWORD (_cdecl *_fn_cdecl_0) (void);
typedef DWORD (_cdecl *_fn_cdecl_1) (void *);
typedef DWORD (_cdecl *_fn_cdecl_2) (void *, void *);
typedef DWORD (_cdecl *_fn_cdecl_3) (void *, void *, void *);
typedef DWORD (_cdecl *_fn_cdecl_4) (void *, void *, void *, void *);

typedef DWORD (_stdcall *_fn_stdcall_0) (void);
typedef DWORD (_stdcall *_fn_stdcall_1) (void *);
typedef DWORD (_stdcall *_fn_stdcall_2) (void *, void *);
typedef DWORD (_stdcall *_fn_stdcall_3) (void *, void *, void *);
typedef DWORD (_stdcall *_fn_stdcall_4) (void *, void *, void *, void *);

DWORD __declspec(noinline) call_cdecl_0 (struct _exec_env *env, _fn_cdecl_0 f);
DWORD __declspec(noinline) call_cdecl_1 (struct _exec_env *env, _fn_cdecl_1 f, void *);
DWORD __declspec(noinline) call_cdecl_2 (struct _exec_env *env, _fn_cdecl_2 f, void *, void *);
DWORD __declspec(noinline) call_cdecl_3 (struct _exec_env *env, _fn_cdecl_3 f, void *, void *, void *);
DWORD __declspec(noinline) call_cdecl_4 (struct _exec_env *env, _fn_cdecl_4 f, void *, void *, void *, void *);

DWORD __declspec(noinline) call_stdcall_0 (struct _exec_env *env, _fn_stdcall_0 f);
DWORD __declspec(noinline) call_stdcall_1 (struct _exec_env *env, _fn_stdcall_1 f, void *);
DWORD __declspec(noinline) call_stdcall_2 (struct _exec_env *env, _fn_stdcall_2 f, void *, void *);
DWORD __declspec(noinline) call_stdcall_3 (struct _exec_env *env, _fn_stdcall_3 f, void *, void *, void *);
DWORD __declspec(noinline) call_stdcall_4 (struct _exec_env *env, _fn_stdcall_4 f, void *, void *, void *, void *);



#endif
