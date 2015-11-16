#ifndef _DUAL_ALLOCATOR_H_
#define _DUAL_ALLOCATOR_H_

#include <Windows.h>

class DualAllocator {
private:
	HANDLE hMapping;
	DWORD dwSize;
	DWORD dwUsed;
	DWORD dwGran;

	HANDLE hProcess[2];
public:
	DualAllocator(DWORD size, HANDLE remoteProcess, const wchar_t *shmName, DWORD granularity);
	~DualAllocator();

	void *Allocate(DWORD size, DWORD &offset);
	void Free(void *ptr);
};


#endif
