#ifndef __MM_H
#define __MM_H

#include "execenv.h"

#define HEAP_SIZE 0x100000

void memcpy(void *dest, const void *src, unsigned int size);
void memset(void *dest, int val, unsigned int size);

int 	SC_HeapInit 	(struct _exec_env *pEnv, unsigned int heapSize);
int 	SC_HeapDestroy 	(struct _exec_env *pEnv);
void 	SC_PrintInfo 	(struct _exec_env *pEnv, struct _zone *fz);
int 	SC_HeapList 	(struct _exec_env *pEnv);
unsigned char *SC_HeapAlloc 	(struct _exec_env *pEnv, unsigned long sz);
int 	SC_HeapFree 	(struct _exec_env *pEnv, unsigned char *p);


#endif // __MM_H

