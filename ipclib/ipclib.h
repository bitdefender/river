#ifndef _IPCLIB_H_
#define _IPCLIB_H_

#include "RingBuffer.h"
#include "../revtracer-wrapper/TokenRing.h"

#include "../CommonCrossPlatform/LibraryLayout.h"

#ifdef _MSC_VER
#define DEBUG_BREAK __asm \
{ __asm int 3 }
#else
#define DEBUG_BREAK asm volatile("int $0x3")
typedef unsigned int size_t;
#endif

#if defined _WIN32 || defined __CYGWIN__
	#ifdef _BUILDING_IPC_DLL
		#ifdef __GNUC__
			#define DLL_IPC_PUBLIC __attribute__ ((dllexport))
		#else
			#define DLL_IPC_PUBLIC __declspec(dllexport)
		#endif
	#else
		#ifdef __GNUC__
			#define DLL_IPC_PUBLIC __attribute__ ((dllimport))
		#else
			#define DLL_IPC_PUBLIC __declspec(dllimport)
		#endif
	#endif
	#define DLL_IPC_LOCAL
#else
	#if __GNUC__ >= 4
		#define DLL_IPC_PUBLIC __attribute__ ((visibility ("default")))
		#define DLL_IPC_LOCAL  __attribute__ ((visibility ("hidden")))
	#else
		#define DLL_IPC_PUBLIC
		#define DLL_IPC_LOCAL
	#endif
#endif

namespace ipc {
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

	typedef void *ADDR_TYPE;
	typedef void *HANDLE;

	typedef int BOOL;


#define REQUEST_MEMORY_ALLOC				0x80
#define REQUEST_MEMORY_FREE					0x81
#define REQUEST_TAKE_SNAPSHOT				0x88
#define REQUEST_RESTORE_SNAPSHOT			0x89
#define REQUEST_INITIALIZE_CONTEXT			0x90
#define REQUEST_CLEANUP_CONTEXT				0x91
#define REQUEST_SYSCALL_CONTROL				0x95
#define REQUEST_BRANCH_HANDLER				0xA0
#define REQUEST_DUMMY						0xFF

#define REPLY_MEMORY_ALLOC					0xC0
#define REPLY_MEMORY_FREE					0xC1
#define REPLY_TAKE_SNAPSHOT					0xC8
#define REPLY_RESTORE_SNAPSHOT				0xC9
#define REPLY_INITIALIZE_CONTEXT			0xD0
#define REPLY_CLEANUP_CONTEXT				0xD1
#define REPLY_SYSCALL_CONTROL				0xD5
#define REPLY_BRANCH_HANDLER				0xE0
#define REPLY_DUMMY							0xFE

	struct IpcData {
		DWORD type;

		union {
			struct {
				void *pointer;
				DWORD offset;
			} asMemoryAllocReply;
			QWORD asTakeSnapshotReply;
			QWORD asRestoreSnapshotReply;
			DWORD asExecutionBeginReply;
			DWORD asExecutionControlReply;
			DWORD asExecutionEndReply;
			DWORD asBranchHandlerReply;

			DWORD asMemoryAllocRequest;
			void *asMemoryFreeRequest;
			void *asInitializeContextRequest;
			void *asCleanupContextRequest;
			struct {
				void *context;
				ADDR_TYPE nextInstruction;
				void *cbCtx;
			} asExecutionControlRequest, asExecutionBeginRequest;

			struct {
				void *context;
				void *cbCtx;
			} asExecutionEndRequest;

			struct {
				void *context;
				void *userContext;
			} asSyscallControlRequest;

			struct {
				void *executionEnv;
				void *userContext;
				ADDR_TYPE nextInstruction;
			} asBranchHandlerRequest;
		} data;
	};

#define DEBUGGED_PROCESS_TOKENID 1
#define CONTROL_PROCESS_TOKENID 0

	typedef void(*DbgPrintFunc)(const unsigned int dwMask, const char *fmt, ...);

	typedef void *(*MapMemoryFunc)(
		//unsigned long mapHandler,
		unsigned long access,
		unsigned long offset,
		unsigned long size,
		void *address);

	typedef int(*_vsnprintf_sFunc)(
		char *buffer,
		size_t count,
		const char *format,
		char *argptr
	);

	struct IpcImports {
		//WaitEventFunc waitEventFunc;
		//PostEventFunc postEventFunc;

		MapMemoryFunc mapMemory;
		_vsnprintf_sFunc vsnprintf_sFunc;
		revwrapper::TokenRing *ipcToken;
	};

	typedef void (*DebugPrintFunc)(DWORD printMask, const char *fmt, ...);

	typedef void *(*MemoryAllocFunc)(DWORD dwSize);
	typedef void (*MemoryFreeFunc)(void *ptr);

	typedef QWORD (*TakeSnapshotFunc)();
	typedef QWORD (*RestoreSnapshotFunc)();

	typedef void (*InitializeContextFunc)(void *context);
	typedef void (*CleanupContextFunc)(void *context);

	typedef DWORD (*BranchHandlerFunc)(void *context, void *userContext, ADDR_TYPE nextInstruction);
	typedef void (*SyscallControlFunc)(void *context, void *userContext);

	//typedef void InitializeIpcToken();
	typedef void (*InitializeFunc)();

	struct IpcExports {
		DebugPrintFunc debugPrint;
		MemoryAllocFunc memoryAlloc;
		MemoryFreeFunc memoryFree;

		TakeSnapshotFunc takeSnapshot;
		RestoreSnapshotFunc restoreSnapshot;

		InitializeContextFunc initializeContext;
		CleanupContextFunc cleanupContext;

		BranchHandlerFunc branchHandler;
		SyscallControlFunc syscallControl;

		InitializeFunc initialize;

		RingBuffer<(1 << 20)> *debugLog;
		IpcData *ipcData;
	};

	extern "C" {
		DLL_IPC_PUBLIC extern IpcImports ipcImports;
		DLL_IPC_PUBLIC extern IpcExports ipcExports;

		//DLL_IPC_PUBLIC extern IpcData ipcData;

/*#ifdef __linux__
		DLL_IPC_PUBLIC extern ShmTokenRingLin ipcToken;
#else
		DLL_IPC_PUBLIC extern ShmTokenRingWin ipcToken;
#endif*/


		//DLL_IPC_PUBLIC extern void InitializeIpcToken();


		DLL_IPC_PUBLIC BOOL IsProcessorFeaturePresent(DWORD ProcessorFeature);
	}
};

#endif
