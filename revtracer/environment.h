#ifndef _ENVIRONMENT_H
#define _ENVIRONMENT_H

#include "revtracer.h"

using namespace rev;

//void *EnvMemoryAlloc (DWORD dwSize);
//void EnvMemoryFree (void *b);

extern "C" {
	void __stdcall BranchHandler(struct _exec_env *, ADDR_TYPE);
	void __stdcall SysHandler(struct _exec_env *);
	void __cdecl   SysEndHandler(struct _exec_env *, DWORD, DWORD, DWORD, DWORD, DWORD, DWORD, DWORD, DWORD, DWORD);
};

int dbg0(char *pFormat);
int dbg1(char *pFormat, DWORD p1);
int dbg2(char *pFormat, DWORD p1, DWORD p2);

#endif
