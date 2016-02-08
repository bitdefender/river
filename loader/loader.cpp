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
	typedef enum _SECTION_INHERIT {
		ViewShare = 1,
		ViewUnmap = 2
	} SECTION_INHERIT, *PSECTION_INHERIT;

	typedef union _LARGE_INTEGER {
		struct {
			DWORD LowPart;
			LONG HighPart;
		};
		struct {
			DWORD LowPart;
			LONG HighPart;
		} u;
		LONGLONG QuadPart;
	} LARGE_INTEGER, *PLARGE_INTEGER;

	typedef NTSTATUS(*NtMapViewOfSectionFunc) (
		HANDLE          SectionHandle,
		HANDLE          ProcessHandle,
		PVOID           *BaseAddress,
		ULONG_PTR       ZeroBits,
		SIZE_T          CommitSize,
		PLARGE_INTEGER  SectionOffset,
		PSIZE_T         ViewSize,
		SECTION_INHERIT InheritDisposition,
		ULONG           AllocationType,
		ULONG           Win32Protect
	);

	typedef NTSTATUS(*NtFlushInstructionCacheFunc)(
		HANDLE hProcess,
		LPVOID address,
		SIZE_T size
	);

	typedef BOOL(*RtlFlushSecureMemoryCacheFunc)(
		PVOID 	MemoryCache,
		SIZE_T 	MemoryLength
	);

#define MEM_DECOMMIT	0x4000
#define MEM_RELEASE		0x8000

	typedef NTSTATUS (*NtFreeVirtualMemoryFunc)(
		HANDLE ProcessHandle,
		PVOID *BaseAddress,
		PULONG RegionSize,
		ULONG FreeType
	);

	LoaderConfig loaderConfig = {
		NULL,
		0
	};

	LoaderAPI loaderAPI = {
		NULL,
		NULL,
		NULL
	};

	LPVOID Kernel32MapViewOfFileEx(
		HANDLE hFileMappingObject,
		DWORD dwDesiredAccess,
		DWORD dwFileOffsetHigh,
		DWORD dwFileOffsetLow,
		SIZE_T dwNumberOfBytesToMap,
		LPVOID lpBaseAddress
	) {
		NTSTATUS Status;
		LARGE_INTEGER SectionOffset;
		SIZE_T ViewSize;
		ULONG Protect;
		LPVOID ViewBase;

		/* Convert the offset */
		SectionOffset.LowPart = dwFileOffsetLow;
		SectionOffset.HighPart = dwFileOffsetHigh;

		/* Save the size and base */
		ViewBase = lpBaseAddress;
		ViewSize = dwNumberOfBytesToMap;

		/* Convert flags to NT Protection Attributes */
		if (dwDesiredAccess == FILE_MAP_COPY) {
			Protect = PAGE_WRITECOPY;
		} else if (dwDesiredAccess & FILE_MAP_WRITE) {
			Protect = (dwDesiredAccess & FILE_MAP_EXECUTE) ? PAGE_EXECUTE_READWRITE : PAGE_READWRITE;
		} else if (dwDesiredAccess & FILE_MAP_READ) {
			Protect = (dwDesiredAccess & FILE_MAP_EXECUTE) ? PAGE_EXECUTE_READ : PAGE_READONLY;
		} else {
			Protect = PAGE_NOACCESS;
		}

		/* Map the section */
		Status = ((NtMapViewOfSectionFunc)loaderAPI.ntMapViewOfSection)(
			hFileMappingObject,
			(HANDLE)0xFFFFFFFF,
			&ViewBase,
			0,
			0,
			&SectionOffset,
			&ViewSize,
			ViewShare,
			0,
			Protect
		);

		if (!NT_SUCCESS(Status)) {
			/* We failed */
			__asm int 3;
			return NULL;
		}

		/* Return the base */
		return ViewBase;
	}

	BOOL Kernel32VirtualFreeEx(
		HANDLE hProcess,
		LPVOID lpAddress,
		SIZE_T dwSize,
		DWORD  dwFreeType
	) {
		NTSTATUS ret;

		if ((unsigned __int16)(dwFreeType & 0x8000) && dwSize) {
			return FALSE;
		} else {
			ret = ((NtFreeVirtualMemoryFunc)loaderAPI.ntFreeVirtualMemory)(hProcess, &lpAddress, (PULONG)&dwSize, dwFreeType);
			if (ret >= 0) {
				return TRUE;
			}

			if ((0xC0000045 == ret) && ((HANDLE)0xFFFFFFFF == hProcess)) {
				if (FALSE == ((RtlFlushSecureMemoryCacheFunc)loaderAPI.rtlFlushSecureMemoryCache)(lpAddress, dwSize)) {
					return FALSE;
				}
				ret = ((NtFreeVirtualMemoryFunc)loaderAPI.ntFreeVirtualMemory)((HANDLE)0xFFFFFFFF, &lpAddress, (PULONG)&dwSize, dwFreeType);
				return (ret >= 0) ? TRUE : FALSE;
			} else {
				return FALSE;
			}
		}
	}

	DWORD miniStack[512];
	DWORD shadowStack = (DWORD)&(miniStack[510]);
	HANDLE hMap = NULL;

	DLL_LINKAGE void *MapMemory(DWORD desiredAccess, DWORD offset, SIZE_T size, LPVOID address) {
		return Kernel32MapViewOfFileEx(
			hMap,
			desiredAccess,
			0,
			offset,
			size,
			address
		);
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

		if (FALSE == Kernel32VirtualFreeEx(
			(HANDLE)0xFFFFFFFF,
			loaderConfig.shmBase,
			0,
			MEM_RELEASE
		)) {
			__asm int 3;
		}

		for (DWORD s = 0; s < loaderConfig.sectionCount; ++s) {
			void *addr = Kernel32MapViewOfFileEx(
				hMap,
				loaderConfig.sections[s].desiredAccess,
				0,
				loaderConfig.sections[s].sectionOffset,
				loaderConfig.sections[s].mappingSize,
				loaderConfig.sections[s].mappingAddress
			);
			
			if (addr != loaderConfig.sections[s].mappingAddress) {
				__asm int 3;
			}
		}

		((NtFlushInstructionCacheFunc)loaderAPI.ntFlushInstructionCache) (
			(HANDLE)0xFFFFFFFF,
			NULL,
			0
		);
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