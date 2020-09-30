#ifndef _WRAPPER_GLOBAL_H_
#define _WRAPPER_GLOBAL_H_

typedef void *(*AllocateVirtualFunc)(unsigned long);
typedef void (*FreeVirtualFunc)(void *);
typedef void *(*MapMemoryFunc)(unsigned long, unsigned long, unsigned long, unsigned long, void *);

typedef void (*TerminateProcessFunc)(int);
typedef void *(*GetTerminationCodeFunc)();

typedef int (*FormatPrintFunc)(char *buffer, size_t sizeOfBuffer, const char *format, char *);

typedef bool (*WriteFileFunc)(void *, void *, size_t, unsigned long *);
typedef long (*ToErrnoFunc)(long);

typedef long (*YieldExecutionFunc)(void);
typedef bool (*InitEventFunc)(void *, bool);
typedef bool (*WaitEventFunc)(void *, int);
typedef bool (*PostEventFunc)(void *);
typedef void (*DestroyEventFunc)(void *);
typedef int (*GetValueEventFunc) (void *);

typedef int (*OpenSharedMemoryFunc) (const char *, int, int);
typedef int (*UnlinkSharedMemoryFunc) (const char *);

typedef void (*FlushInstructionCacheFunc)(void);


extern AllocateVirtualFunc allocateVirtual;
extern FreeVirtualFunc freeVirtual;
extern MapMemoryFunc mapMemory;

extern TerminateProcessFunc terminateProcess;
extern GetTerminationCodeFunc getTerminationCode;

extern FormatPrintFunc formatPrint;

extern WriteFileFunc writeFile;
extern ToErrnoFunc toErrno;

extern YieldExecutionFunc yieldExecution;
extern InitEventFunc initEvent;
extern WaitEventFunc waitEvent;
extern PostEventFunc postEvent;
extern DestroyEventFunc destroyEvent;
extern GetValueEventFunc getvalueEvent;

extern OpenSharedMemoryFunc openSharedMemory;
extern UnlinkSharedMemoryFunc unlinkSharedMemory;

extern FlushInstructionCacheFunc flushInstructionCache;

#endif
