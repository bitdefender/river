#ifndef __MM_H
#define __MM_H

#include "extern.h"

#define HEAP_SIZE 0x100000

void memcpy(void *dest, const void *src, unsigned int size);
void memset(void *dest, int val, unsigned int size);

struct HeapZone;

/* A self contained heap */
class RiverHeap {
private :
	BYTE *pHeap;
	HeapZone *pFirstFree;
	DWORD size;
public :
	RiverHeap();
	~RiverHeap();

	bool Init(DWORD heapSize);
	bool Destroy();

	void PrintInfo(HeapZone *fz);
	void List();

	void *Alloc(DWORD size);
	void Free(void *ptr);
};

#endif // __MM_H

