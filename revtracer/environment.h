#ifndef _ENVIRONMENT_H
#define _ENVIRONMENT_H

#include "extern.h"

void *EnvMemoryAlloc (DWORD dwSize);
void EnvMemoryFree (void *b);

void __stdcall BranchHandler(struct _exec_env *, DWORD);
void __cdecl   SysHandler(struct _exec_env *, DWORD, DWORD, DWORD, DWORD, DWORD, DWORD, DWORD, DWORD);
void __cdecl   SysEndHandler(struct _exec_env *, DWORD, DWORD, DWORD, DWORD, DWORD, DWORD, DWORD, DWORD, DWORD);

int dbg0(char *pFormat);
int dbg1(char *pFormat, DWORD p1);
int dbg2(char *pFormat, DWORD p1, DWORD p2);

#endif
