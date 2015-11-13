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

#define OBJ_INHERIT             0x00000002L
#define OBJ_PERMANENT           0x00000010L
#define OBJ_EXCLUSIVE           0x00000020L
#define OBJ_CASE_INSENSITIVE    0x00000040L
#define OBJ_OPENIF              0x00000080L
#define OBJ_OPENLINK            0x00000100L
#define OBJ_KERNEL_HANDLE       0x00000200L
#define OBJ_FORCE_ACCESS_CHECK  0x00000400L
#define OBJ_VALID_ATTRIBUTES    0x000007F2L

#define DIRECTORY_QUERY                   0x0001
#define DIRECTORY_TRAVERSE                0x0002
#define DIRECTORY_CREATE_OBJECT           0x0004
#define DIRECTORY_CREATE_SUBDIRECTORY     0x0008

#ifndef InitializeObjectAttributes
#define InitializeObjectAttributes( p, n, a, r, s ) { \
	(p)->Length = sizeof(OBJECT_ATTRIBUTES);          \
	(p)->RootDirectory = r;                             \
	(p)->Attributes = a;                                \
	(p)->ObjectName = n;                                \
	(p)->SecurityDescriptor = s;                        \
	(p)->SecurityQualityOfService = NULL;               \
}
#endif

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

	typedef struct _UNICODE_STRING {
		USHORT Length;
		USHORT MaximumLength;
		PWSTR  Buffer;
	} UNICODE_STRING;
	typedef UNICODE_STRING *PUNICODE_STRING;

	typedef struct _OBJECT_ATTRIBUTES {
		ULONG Length;
		HANDLE RootDirectory;
		PUNICODE_STRING ObjectName;
		ULONG Attributes;
		PVOID SecurityDescriptor;
		PVOID SecurityQualityOfService;
	} OBJECT_ATTRIBUTES;
	typedef OBJECT_ATTRIBUTES *POBJECT_ATTRIBUTES;

	typedef struct _SECURITY_ATTRIBUTES {
		DWORD nLength;
		LPVOID lpSecurityDescriptor;
		BOOL bInheritHandle;
	} SECURITY_ATTRIBUTES, *PSECURITY_ATTRIBUTES, *LPSECURITY_ATTRIBUTES;

	typedef NTSTATUS(*NtOpenSectionFunc) (
		HANDLE             *SectionHandle,
		ACCESS_MASK        DesiredAccess,
		POBJECT_ATTRIBUTES ObjectAttributes
	);


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

	typedef NTSTATUS(*NtOpenDirectoryObjectFunc)(
		HANDLE *FileHandle,
		ACCESS_MASK DesiredAccess,
		POBJECT_ATTRIBUTES ObjectAttributes
	);

	typedef NTSTATUS(*NtCloseFunc)(
		HANDLE Handle
	);

	typedef NTSTATUS(*NtFlushInstructionCacheFunc)(
		HANDLE hProcess,
		LPVOID address,
		SIZE_T size
	);

	typedef void (*RtlInitUnicodeStringExFunc) (
		PUNICODE_STRING  DestinationString,
		PCWSTR           pszSrc,
		DWORD            dwFlags
	);


	typedef void (*RtlFreeUnicodeString) (
		PUNICODE_STRING UnicodeString
	);

	LoaderConfig loaderConfig = {
		L"",
		0
	};

	LoaderAPI loaderAPI = {
		NULL,
		NULL,
		NULL,
		NULL
	};

	HANDLE BaseNamedObjectDirectory;
	HANDLE Kernel32BaseGetNamedObjectDirectory()
	{
		OBJECT_ATTRIBUTES ObjectAttributes;
		NTSTATUS Status;
		HANDLE DirHandle, BnoHandle;

		//RtlAcquirePebLock();
		if (BaseNamedObjectDirectory) {
			//RtlReleasePebLock();
			return BaseNamedObjectDirectory;
		}

		UNICODE_STRING NamedObjectDirectory;
		UNICODE_STRING Restricted;
		((RtlInitUnicodeStringExFunc)loaderAPI.rtlInitUnicodeStringEx)(&Restricted, L"Restricted", NULL);
		((RtlInitUnicodeStringExFunc)loaderAPI.rtlInitUnicodeStringEx)(&NamedObjectDirectory, L"\\BaseNamedObjects", NULL);

		InitializeObjectAttributes(&ObjectAttributes,
			&NamedObjectDirectory,
			OBJ_CASE_INSENSITIVE,
			NULL,
			NULL);

		Status = ((NtOpenDirectoryObjectFunc)loaderAPI.ntOpenDirectoryObject)(&BnoHandle,
			DIRECTORY_QUERY |
			DIRECTORY_TRAVERSE |
			DIRECTORY_CREATE_OBJECT |
			DIRECTORY_CREATE_SUBDIRECTORY,
			&ObjectAttributes);

		if (!NT_SUCCESS(Status)) {
			Status = ((NtOpenDirectoryObjectFunc)loaderAPI.ntOpenDirectoryObject)(&DirHandle,
				DIRECTORY_TRAVERSE,
				&ObjectAttributes);
			if (NT_SUCCESS(Status)) {
				InitializeObjectAttributes(&ObjectAttributes,
					(PUNICODE_STRING)&Restricted,
					OBJ_CASE_INSENSITIVE,
					DirHandle,
					NULL);
				Status = ((NtOpenDirectoryObjectFunc)loaderAPI.ntOpenDirectoryObject)(&BnoHandle,
					DIRECTORY_QUERY |
					DIRECTORY_TRAVERSE |
					DIRECTORY_CREATE_OBJECT |
					DIRECTORY_CREATE_SUBDIRECTORY,
					&ObjectAttributes);
				((NtCloseFunc)loaderAPI.ntClose)(DirHandle);
			}
		}

		if (NT_SUCCESS(Status))
			BaseNamedObjectDirectory = BnoHandle;

		//RtlReleasePebLock();
		return BaseNamedObjectDirectory;
	}


	POBJECT_ATTRIBUTES Kernel32BaseFormatObjectAttributes(
		POBJECT_ATTRIBUTES ObjectAttributes,
		PSECURITY_ATTRIBUTES SecurityAttributes,
		PUNICODE_STRING ObjectName
		)
	{
		ULONG Attributes;
		HANDLE RootDirectory;
		PVOID SecurityDescriptor;

		/* Get the attributes if present */
		if (SecurityAttributes)
		{
			Attributes = SecurityAttributes->bInheritHandle ? OBJ_INHERIT : 0;
			SecurityDescriptor = SecurityAttributes->lpSecurityDescriptor;
		}
		else
		{
			if (!ObjectName) return NULL;
			Attributes = 0;
			SecurityDescriptor = NULL;
		}

		if (ObjectName)
		{
			Attributes |= OBJ_OPENIF;
			RootDirectory = Kernel32BaseGetNamedObjectDirectory();
		}
		else
		{
			RootDirectory = NULL;
		}

		/* Create the Object Attributes */
		InitializeObjectAttributes(ObjectAttributes,
			ObjectName,
			Attributes,
			RootDirectory,
			SecurityDescriptor);
		return ObjectAttributes;
	}

	HANDLE Kernel32OpenFileMapping(
		DWORD   dwDesiredAccess,
		BOOL    bInheritHandle,
		PCWSTR lpName
	) {
		UNICODE_STRING us;
		SECURITY_ATTRIBUTES sAttr, *psAttr;
		OBJECT_ATTRIBUTES oAttr;
		HANDLE hRet;
		if (!lpName) {
			__asm int 3;
			return 0;
		}

		if (bInheritHandle) {
			sAttr.nLength = sizeof(sAttr);
			sAttr.lpSecurityDescriptor = NULL;
			sAttr.bInheritHandle = TRUE;
			psAttr = &sAttr;
		} else {
			psAttr = NULL;
		}

		((RtlInitUnicodeStringExFunc)loaderAPI.rtlInitUnicodeStringEx)(&us, lpName, 0);
		Kernel32BaseFormatObjectAttributes(&oAttr, psAttr, &us);
		DWORD access = dwDesiredAccess;
		if (dwDesiredAccess == FILE_MAP_COPY) {
			access = FILE_MAP_READ;
		} else {
			if (dwDesiredAccess & FILE_MAP_EXECUTE)
				access = dwDesiredAccess & (~FILE_MAP_EXECUTE) | SECTION_MAP_EXECUTE;
		}
		NTSTATUS ntRet = ((NtOpenSectionFunc)loaderAPI.ntOpenSection)(&hRet, access, &oAttr);
		if (!NT_SUCCESS(ntRet)) {
			__asm int 3;
			return 0;
		}
		return hRet;
	}

	LPVOID Kernel32MapViewOfFileEx(
		HANDLE hFileMappingObject,
		DWORD dwDesiredAccess,
		DWORD dwFileOffsetHigh,
		DWORD dwFileOffsetLow,
		SIZE_T dwNumberOfBytesToMap,
		LPVOID lpBaseAddress
		)
	{
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
		if (dwDesiredAccess == FILE_MAP_COPY)
		{
			Protect = PAGE_WRITECOPY;
		}
		else if (dwDesiredAccess & FILE_MAP_WRITE)
		{
			Protect = (dwDesiredAccess & FILE_MAP_EXECUTE) ?
			PAGE_EXECUTE_READWRITE : PAGE_READWRITE;
		}
		else if (dwDesiredAccess & FILE_MAP_READ)
		{
			Protect = (dwDesiredAccess & FILE_MAP_EXECUTE) ?
			PAGE_EXECUTE_READ : PAGE_READONLY;
		}
		else
		{
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
			Protect);
		if (!NT_SUCCESS(Status))
		{
			/* We failed */
			//BaseSetLastNTError(Status);
			__asm int 3;
			return NULL;
		}

		/* Return the base */
		return ViewBase;
	}

#define PF_FLOATING_POINT_PRECISION_ERRATA   0   
#define PF_FLOATING_POINT_EMULATED           1   
#define PF_COMPARE_EXCHANGE_DOUBLE           2   
#define PF_MMX_INSTRUCTIONS_AVAILABLE        3   
#define PF_PPC_MOVEMEM_64BIT_OK              4   
#define PF_ALPHA_BYTE_INSTRUCTIONS           5   
#define PF_XMMI_INSTRUCTIONS_AVAILABLE       6   
#define PF_3DNOW_INSTRUCTIONS_AVAILABLE      7   
#define PF_RDTSC_INSTRUCTION_AVAILABLE       8   
#define PF_PAE_ENABLED                       9   
#define PF_XMMI64_INSTRUCTIONS_AVAILABLE    10   
#define PF_SSE_DAZ_MODE_AVAILABLE           11   
#define PF_NX_ENABLED                       12   
#define PF_SSE3_INSTRUCTIONS_AVAILABLE      13   
#define PF_COMPARE_EXCHANGE128              14   
#define PF_COMPARE64_EXCHANGE128            15   
#define PF_CHANNELS_ENABLED                 16   
#define PF_XSAVE_ENABLED                    17   
#define PF_ARM_VFP_32_REGISTERS_AVAILABLE   18   
#define PF_ARM_NEON_INSTRUCTIONS_AVAILABLE  19   
#define PF_SECOND_LEVEL_ADDRESS_TRANSLATION 20   
#define PF_VIRT_FIRMWARE_ENABLED            21   
#define PF_RDWRFSGSBASE_AVAILABLE           22   
#define PF_FASTFAIL_AVAILABLE               23   
#define PF_ARM_DIVIDE_INSTRUCTION_AVAILABLE 24   
#define PF_ARM_64BIT_LOADSTORE_ATOMIC       25   
#define PF_ARM_EXTERNAL_CACHE_AVAILABLE     26   
#define PF_ARM_FMAC_INSTRUCTIONS_AVAILABLE  27   
#define PF_RDRAND_INSTRUCTION_AVAILABLE     28   

	DLL_LINKAGE BOOL MyIsProcessorFeaturePresent(DWORD ProcessorFeature) {
		BOOL result;

		if (ProcessorFeature >= 0x40) {
			result = FALSE;
		} else {
			switch (ProcessorFeature) {
				case PF_MMX_INSTRUCTIONS_AVAILABLE:
				case PF_SSE3_INSTRUCTIONS_AVAILABLE:
				case PF_XMMI64_INSTRUCTIONS_AVAILABLE:
				case PF_XMMI_INSTRUCTIONS_AVAILABLE:
				case PF_COMPARE_EXCHANGE_DOUBLE:
				case PF_COMPARE_EXCHANGE128:
				case PF_COMPARE64_EXCHANGE128:
					return FALSE;
				default:
					result = ((BYTE *)0x7FFE0274)[ProcessorFeature];
			}
		}
		return result;
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

	void CppLoaderPerform() {
		hMap = Kernel32OpenFileMapping(FILE_MAP_ALL_ACCESS, FALSE, loaderConfig.sharedMemoryName);
		if (NULL == hMap) {
			__asm int 3;
		}

		/*void *mapAddress = Kernel32MapViewOfFileEx(hMap, FILE_MAP_ALL_ACCESS, 0, 0, 0, loaderConfig.mappingAddress);
		if (NULL == mapAddress) {
			__asm int 3;
		}*/

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

			//if (loaderConfig.sections[s].desiredAccess & FILE_MAP_EXECUTE) {
			
			//}
		}

		((NtFlushInstructionCacheFunc)loaderAPI.ntFlushInstructionCache) (
			(HANDLE)0xFFFFFFFF,
			NULL,
			0
		);

		
		//loaderConfig.entryPoint = (BYTE *)loaderConfig.entryPoint + (unsigned int)mapAddress;
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