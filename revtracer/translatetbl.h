#ifndef _CODETBL_H
#define _CODETBL_H

#include "execenv.h"

typedef unsigned int (__stdcall *OpCopy)(
	struct _exec_env *pEnv, 
	struct _cb_info *pCB, 
	unsigned int *dwFlags, 
	char *pI, 
	char *pOut, 
	unsigned int *dwOutWr
);

extern OpCopy TranslateTable00[], TranslateTable0F[];
extern OpCopy SaveTable00[],      SaveTable0F[];

//unsigned int AddSysEndPrefix(struct _exec_env *pEnv, char *p);

#endif
