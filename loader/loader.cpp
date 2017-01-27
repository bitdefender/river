#include "loader.h"

namespace ldr {

	/* Kernel32.dll replacements *********************************************************/

#ifndef NT_SUCCESS
#define NT_SUCCESS(Status) (((NTSTATUS)(Status)) >= 0)
#endif

#define TRUE  1
#define FALSE 0

#define STANDARD_RIGHTS_REQUIRED         (0x000F0000L)

#define FILE_MAP_COPY       0x00000001
#define FILE_MAP_RESERVE    0x80000000

#define SECTION_QUERY                0x0001
#define SECTION_MAP_WRITE            0x0002
#define SECTION_MAP_READ             0x0004
#define SECTION_MAP_EXECUTE          0x0008
#define SECTION_EXTEND_SIZE          0x0010
#define SECTION_MAP_EXECUTE_EXPLICIT 0x0020 // not included in SECTION_ALL_ACCESS

#define SECTION_ALL_ACCESS (STANDARD_RIGHTS_REQUIRED|SECTION_QUERY|\
	SECTION_MAP_WRITE | \
	SECTION_MAP_READ | \
	SECTION_MAP_EXECUTE | \
	SECTION_EXTEND_SIZE)

#define FILE_MAP_WRITE      SECTION_MAP_WRITE
#define FILE_MAP_READ       SECTION_MAP_READ
#define FILE_MAP_ALL_ACCESS SECTION_ALL_ACCESS
#define FILE_MAP_EXECUTE    SECTION_MAP_EXECUTE_EXPLICIT

#define PAGE_NOACCESS          0x01     
#define PAGE_READONLY          0x02     
#define PAGE_READWRITE         0x04     
#define PAGE_WRITECOPY         0x08     
#define PAGE_EXECUTE           0x10     
#define PAGE_EXECUTE_READ      0x20     
#define PAGE_EXECUTE_READWRITE 0x40     
#define PAGE_EXECUTE_WRITECOPY 0x80     
#define PAGE_GUARD            0x100     
#define PAGE_NOCACHE          0x200     
#define PAGE_WRITECOMBINE     0x400


	typedef int BOOL;
	typedef void *LPVOID;
	typedef long NTSTATUS;
	typedef void *HANDLE;
	typedef void *PVOID;
	typedef unsigned long ULONG;
	typedef long LONG;
	typedef long long LONGLONG;
	typedef DWORD ACCESS_MASK;
	typedef unsigned short USHORT;
	typedef wchar_t *PWSTR;
	typedef const wchar_t *PCWSTR;
	typedef unsigned long ULONG, *PULONG;


#define MEM_DECOMMIT	0x4000
#define MEM_RELEASE		0x8000

	typedef void (*FreeMemoryCall)(void *);
	typedef void *(*MapMemoryCall)(void *, unsigned long, unsigned long, unsigned long, void *);
	typedef void (*FlushInstructionCacheCall)(void);

	LoaderConfig loaderConfig = {
		NULL,
		0
	};

	LoaderAPI loaderAPI = {
		NULL,
		NULL,
		NULL
	};

	DWORD miniStack[512];
	DWORD shadowStack = (DWORD)&(miniStack[510]);
	HANDLE hMap = NULL;

	DLL_PUBLIC void *MapMemory(DWORD desiredAccess, DWORD offset, SIZE_T size, LPVOID address) {
		return ((MapMemoryCall)loaderAPI.ntMapViewOfSection)(hMap, desiredAccess, offset, size, address);
	}

	void SimulateDebugger() {
		__asm mov eax, fs:[0x18];
		__asm mov eax, [eax + 0x30];

		// eax is PEB

		// set is debugger present
		__asm mov byte ptr[eax + 0x02], 1;
		// set debug flags for heap
		__asm and dword ptr[eax + 0x68], 0x70;
	}

	void CppLoaderPerform() {
		hMap = loaderConfig.sharedMemory;

		SimulateDebugger();

		((FreeMemoryCall)loaderAPI.ntFreeVirtualMemory)(
			loaderConfig.shmBase);

		for (DWORD s = 0; s < loaderConfig.sectionCount; ++s) {
			void *addr = MapMemory(
				loaderConfig.sections[s].desiredAccess,
				loaderConfig.sections[s].sectionOffset,
				(SIZE_T)loaderConfig.sections[s].mappingSize,
				loaderConfig.sections[s].mappingAddress
			);

			if (addr != loaderConfig.sections[s].mappingAddress) {
				__asm int 3;
			}
		}

		((FlushInstructionCacheCall)loaderAPI.ntFlushInstructionCache)();
	}

	__declspec(naked) void LoaderPerform() {
		__asm {
			//int 3;
			xchg esp, shadowStack;
			pushad;
			pushfd;
			call CppLoaderPerform;
			popfd;
			popad;
			xchg esp, shadowStack;
			jmp dword ptr[loaderConfig.entryPoint];
		}
	}

};

unsigned int Entry() { 
	return 1;
}
