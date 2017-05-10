#ifndef _BASIC_TYPES_H_
#define _BASIC_TYPES_H_

/** Use this header in binaries that have no dependencies */

namespace nodep {
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

	typedef unsigned long ULONG;
	typedef ULONG *PULONG;
	typedef unsigned short USHORT;
	typedef USHORT *PUSHORT;
	typedef unsigned char UCHAR;
	typedef UCHAR *PUCHAR;
	
	typedef long LONG;
	typedef long long LONGLONG;

	typedef unsigned long long QWORD;
	typedef unsigned long DWORD;
	typedef unsigned short WORD;
	typedef unsigned char BYTE;

	/* Windows-like types */
	typedef int BOOL;
	typedef void *HANDLE;
	typedef void *LPVOID;

	typedef ULONG_PTR SIZE_T, *PSIZE_T;

	typedef wchar_t WCHAR;
	typedef WCHAR *PWSTR;

	typedef BYTE  BOOLEAN;
	
	typedef union _LARGE_INTEGER {
		struct {
			DWORD LowPart;
			LONG  HighPart;
		};
		struct {
			DWORD LowPart;
			LONG  HighPart;
		} u;
		LONGLONG QuadPart;
	} LARGE_INTEGER, *PLARGE_INTEGER;

#ifndef FALSE
#define FALSE               0
#endif

#ifndef TRUE
#define TRUE                1
#endif

};

#endif
