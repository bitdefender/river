#ifndef __MM_H
#define __MM_H

#include "revtracer.h"

#define HEAP_SIZE 0x100000

extern "C" void rev_memcpy(void *dest, const void *src, unsigned int size);
extern "C" void rev_memset(void *dest, int val, unsigned int size);

struct HeapZone;

/* A self contained heap */
class RiverHeap {
private :
	rev::BYTE *pHeap;
	HeapZone *pFirstFree;
	rev::DWORD size;
public :
	RiverHeap();
	~RiverHeap();

	bool Init(rev::DWORD heapSize);
	bool Destroy();

	void PrintInfo(HeapZone *fz);
	void List();

	void *Alloc(rev::DWORD size);
	void Free(void *ptr);
};

#endif // __MM_H

