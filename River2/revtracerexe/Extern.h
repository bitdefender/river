#ifndef _EXTERN_H
#define _EXTERN_H

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

typedef void *ADDR_TYPE;

typedef unsigned long long QWORD;
typedef unsigned long DWORD;
typedef unsigned short WORD;
typedef unsigned char BYTE;

typedef void(*DbgPrintFunc)(const char *fmt, ...);
typedef void *(*MemoryAllocFunc)(DWORD dwSize);
typedef void(*MemoryFreeFunc)(void *ptr);


typedef QWORD(*TakeSnapshotFunc)();
typedef QWORD(*RestoreSnapshotFunc)();




extern DbgPrintFunc DbgPrint;

extern MemoryAllocFunc EnvMemoryAlloc;
extern MemoryFreeFunc EnvMemoryFree;

extern TakeSnapshotFunc TakeSnapshot;
extern RestoreSnapshotFunc RestoreSnapshot;

/*void DbgPrint(const char *fmt, ...);

void *EnvMemoryAlloc(unsigned long dwSize);
void EnvMemoryFree(void *b);

unsigned long long TakeSnapshot();
unsigned long long RestoreSnapshot();*/

#endif