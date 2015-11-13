#ifndef _LOADER_H
#define _LOADER_H

namespace ldr {
#ifdef _BUILDING_LOADER_DLL
#define DLL_LINKAGE __declspec(dllexport)
#else
#define DLL_LINKAGE __declspec(dllimport)
#endif

/* Define NULL pointer value */
#ifndef NULL
#ifdef __cplusplus
#define NULL    0
#else  /* __cplusplus */
#define NULL    ((void *)0)
#endif  /* __cplusplus */
#endif  /* NULL */

#if !defined(_W64)
#if !defined(__midl) && (defined(_X86_) || defined(_M_IX86) || defined(_ARM_) || defined(_M_ARM)) && _MSC_VER >= 1300
#define _W64 __w64
#else
#define _W64
#endif
#endif

#if defined(_WIN64)
	typedef __int64 INT_PTR, *PINT_PTR;
	typedef unsigned __int64 UINT_PTR, *PUINT_PTR;

	typedef __int64 LONG_PTR, *PLONG_PTR;
	typedef unsigned __int64 ULONG_PTR, *PULONG_PTR;

#define __int3264   __int64

#else
	typedef _W64 int INT_PTR, *PINT_PTR;
	typedef _W64 unsigned int UINT_PTR, *PUINT_PTR;

	typedef _W64 long LONG_PTR, *PLONG_PTR;
	typedef _W64 unsigned long ULONG_PTR, *PULONG_PTR;

#define __int3264   __int32

#endif

	typedef unsigned long long QWORD;
	typedef unsigned long DWORD;
	typedef unsigned short WORD;
	typedef unsigned char BYTE;
	typedef int BOOL;

	typedef size_t SIZE_T, *PSIZE_T;
	typedef void *ADDR_TYPE;

	struct PESections {
		ADDR_TYPE mappingAddress;
		DWORD     mappingSize;
		DWORD     sectionOffset;
		DWORD     desiredAccess;
	};

	struct LoaderConfig {
		wchar_t sharedMemoryName[512];
		PESections sections[32];
		DWORD sectionCount;
		ADDR_TYPE entryPoint;
	};

	struct LoaderAPI {
		ADDR_TYPE ntOpenSection;
		ADDR_TYPE ntMapViewOfSection;

		ADDR_TYPE ntOpenDirectoryObject;
		ADDR_TYPE ntClose;

		ADDR_TYPE ntFlushInstructionCache;

		ADDR_TYPE rtlInitUnicodeStringEx;
		ADDR_TYPE rtlFreeUnicodeString;
	};


	extern "C" {
		DLL_LINKAGE extern LoaderConfig loaderConfig;
		DLL_LINKAGE extern LoaderAPI loaderAPI;

		DLL_LINKAGE BOOL MyIsProcessorFeaturePresent(DWORD ProcessorFeature);
		DLL_LINKAGE void *MapMemory(DWORD desiredAccess, DWORD offset, SIZE_T size, void *address);
		DLL_LINKAGE void LoaderPerform();
	}

};


#endif
