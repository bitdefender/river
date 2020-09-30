#ifndef _CODETBL_H
#define _CODETBL_H

#include "execenv.h"

typedef unsigned int (__stdcall *OpCopy)(struct _exec_env *pEnv, struct _cb_info *pCB, unsigned int *dwFlags, char *pI, char *pD, unsigned int *dwWritten);

extern OpCopy Table00[], Table0F[];

unsigned int AddSysEndPrefix(struct _exec_env *pEnv, char *p);

#endif
