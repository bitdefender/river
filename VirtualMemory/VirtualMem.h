#ifndef _VIRTUAL_MEM_H_
#define _VIRTUAL_MEM_H_

#include "../CommonCrossPlatform/BasicTypes.h"

namespace vmem {
	// the process_t aliases depending on the os
	// Linux: pid - used by libproc
	// Windows: HANDLE (to an opened process) - used by kernel32!VirtualQuery

#ifdef __linux__
	typedef int process_t;
#elif defined _WIN32 || defined __CYGWIN__
	#include <Windows.h>
	typedef HANDLE process_t;
#endif

	nodep::BYTE *GetFreeRegion(process_t hProcess, nodep::DWORD size, nodep::DWORD granularity);
	nodep::BYTE *GetFreeRegion(process_t hProcess1, process_t hProcess2, nodep::DWORD size, nodep::DWORD granularity);

};


#endif
