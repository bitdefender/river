#ifndef _CB_H
#define _CB_H

#include "execenv.h"

//#include <Ntddk.h>
//#include <Basetsd.h>


struct _cb_info {
	UINT_PTR			address;
	unsigned long		dwSize;
	unsigned long		dwCRC;
	unsigned long		dwParses;
	unsigned char		*pCode;
	struct _cb_info		*pNext;
};

struct _cb_info *NewBlock (struct _exec_env *pEnv);
struct _cb_info *FindBlock (struct _exec_env *pEnv, unsigned long);

void AddBlock (struct _exec_env *pEnv, struct _cb_info *);
int InitBlock(struct _exec_env *pEnv, unsigned int logHashSize, unsigned int historySize);
void CloseBlock (struct _exec_env *pEnv);

void PrintHistory (struct _exec_env *pEnv);
DWORD DumpHistory(struct _exec_env *pEnv, unsigned char *o, unsigned long s, unsigned long *sz);

int Translate (struct _exec_env *pEnv, struct _cb_info *pCB, DWORD dwTranslationFlags);


/*
	8 dwords = 32 bytes
*/


#define CB_FLAG_SYSOUT	0x80000000


#endif // _CB_H

