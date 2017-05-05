#ifndef _DUAL_ALLOCATOR_H_
#define _DUAL_ALLOCATOR_H_

#include "../CommonCrossPlatform/Common.h"
#include <vector>

typedef void *FileView;

class DualAllocator {
private:
	MAPPING_HANDLE hMapping;
	DWORD dwSize;
	DWORD dwUsed;
	DWORD dwGran;
	DWORD baseAddress;

	std::vector<std::pair<FileView, DWORD> > mappedViews;
	PROCESS_HANDLE hProcess[2];
public:
	DualAllocator(DWORD size, PROCESS_HANDLE remoteProcess,	const char *shmName, DWORD granularity, DWORD initialOffset);
	~DualAllocator();

	HANDLE CloneTo(PROCESS_HANDLE process);

	void SetBaseAddress(DWORD baseAddress);
	void *Allocate(DWORD size, DWORD &offset);
	DWORD AllocateFixed(DWORD size, DWORD address);
	void Free(void *ptr);
};


#endif
