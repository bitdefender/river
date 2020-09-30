#ifndef _ENVIRONMENT_H
#define _ENVIRONMENT_H

#include "revtracer.h"

//void *EnvMemoryAlloc (DWORD dwSize);
//void EnvMemoryFree (void *b);

extern "C" {
	void __stdcall BranchHandler(
		struct ExecutionEnvironment *, 
		rev::ADDR_TYPE
	);
	void __stdcall SysHandler(
		struct ExecutionEnvironment *
	);
	void __cdecl   SysEndHandler(
		struct ExecutionEnvironment *, 
		nodep::DWORD, 
		nodep::DWORD, 
		nodep::DWORD, 
		nodep::DWORD, 
		nodep::DWORD, 
		nodep::DWORD, 
		nodep::DWORD, 
		nodep::DWORD, 
		nodep::DWORD
	);
};

int dbg0(char *pFormat);
int dbg1(char *pFormat, nodep::DWORD p1);
int dbg2(char *pFormat, nodep::DWORD p1, nodep::DWORD p2);

#endif
