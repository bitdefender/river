#ifndef _DUAL_ALLOCATOR_H_
#define _DUAL_ALLOCATOR_H_

#ifdef _WIN32
#include <Windows.h>
#endif
#include <vector>

typedef void *FileView;

class DualAllocator {
private:
	HANDLE hMapping;
	DWORD dwSize;
	DWORD dwUsed;
	DWORD dwGran;

	std::vector<FileView> mappedViews;
	HANDLE hProcess[2];
public:
	DualAllocator(DWORD size, HANDLE remoteProcess, const wchar_t *shmName, DWORD granularity);
	~DualAllocator();

	HANDLE CloneTo(HANDLE process);

	void *Allocate(DWORD size, DWORD &offset);
	void Free(void *ptr);
};


#endif
