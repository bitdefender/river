#ifndef _REVTRACER_H
#define _REVTRACER_H

namespace rev {

#ifdef _BUILDING_REVTRACER_DLL
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
	typedef void *HANDLE;

	typedef void *ADDR_TYPE;

	typedef void(*DbgPrintFunc)(const char *fmt, ...);
	typedef void *(*MemoryAllocFunc)(DWORD dwSize);
	typedef void(*MemoryFreeFunc)(void *ptr);

	typedef QWORD(*TakeSnapshotFunc)();
	typedef QWORD(*RestoreSnapshotFunc)();

#define EXECUTION_ADVANCE					0x00000000
#define EXECUTION_BACKTRACK					0x00000001
#define EXECUTION_TERMINATE					0x00000002

	typedef void(*InitializeContextFunc)(void *context);
	typedef void(*CleanupContextFunc)(void *context);
	typedef DWORD(*ExecutionBeginFunc)(void *context, ADDR_TYPE firstInstruction, void *cbCtx);
	typedef DWORD(*ExecutionControlFunc)(void *context, ADDR_TYPE nextInstruction, void *cbCtx);
	typedef DWORD(*ExecutionEndFunc)(void *context, void *cbCtx);
	typedef void(*SyscallControlFunc)(void *context);
	typedef void(*IpcLibInitFunc)();

	struct LowLevelRevtracerAPI {
		/* Low level ntdll.dll functions */
		ADDR_TYPE ntAllocateVirtualMemory;
		ADDR_TYPE ntFreeVirtualMemory;
		ADDR_TYPE ntQueryInformationThread;
		ADDR_TYPE ntTerminateProcess;

		ADDR_TYPE ntWriteFile;
		ADDR_TYPE ntWaitForSingleObject;

		ADDR_TYPE rtlNtStatusToDosError;

		ADDR_TYPE vsnprintf_s;
	};

	struct RevtracerAPI {
		/* Logging function */
		DbgPrintFunc dbgPrintFunc;

		/* Memory management function */
		MemoryAllocFunc memoryAllocFunc;
		MemoryFreeFunc memoryFreeFunc;

		/* VM Snapshot control */
		TakeSnapshotFunc takeSnapshot;
		RestoreSnapshotFunc restoreSnapshot;

		/* Execution callbacks */
		InitializeContextFunc initializeContext;
		CleanupContextFunc cleanupContext;
		ExecutionBeginFunc executionBegin;
		ExecutionControlFunc executionControl;
		ExecutionEndFunc executionEnd;
		SyscallControlFunc syscallControl;

		/* IpcLib initialization */
		IpcLibInitFunc ipcLibInitialize;

		LowLevelRevtracerAPI lowLevel;
	};

	struct CodeHooks {
		ADDR_TYPE originalAddr;
		ADDR_TYPE detourAddr;
	};

	struct RevtracerConfig {
		ADDR_TYPE entryPoint;
		ADDR_TYPE mainModule;
		DWORD contextSize;
		DWORD segmentOffsets[0x100];

		BOOL dumpBlocks;
		HANDLE hBlocks;

		void *pRuntime;

		DWORD hookCount;
		CodeHooks hooks[0x10];
	};

	struct ExecutionRegs {
		DWORD edi;
		DWORD esi;
		DWORD ebp;
		DWORD esp;

		DWORD ebx;
		DWORD edx;
		DWORD ecx;
		DWORD eax;
		DWORD eflags;
	};


	extern "C" {

		/* Execution context callbacks ********************/
		DLL_LINKAGE void GetCurrentRegisters(void *ctx, ExecutionRegs *regs);

		/* In process API *********************************/

		DLL_LINKAGE extern RevtracerAPI revtracerAPI;
		DLL_LINKAGE extern RevtracerConfig revtracerConfig;
		/* Can be used as an EP for in process execution  */
		DLL_LINKAGE void RevtracerPerform();


		/* DLL API ****************************************/

		DLL_LINKAGE void SetDbgPrint(DbgPrintFunc);
		DLL_LINKAGE void SetMemoryMgmt(MemoryAllocFunc alc, MemoryFreeFunc fre);
		DLL_LINKAGE void SetSnapshotMgmt(TakeSnapshotFunc ts, RestoreSnapshotFunc rs);
		DLL_LINKAGE void SetLowLevelAPI(LowLevelRevtracerAPI *llApi);
		DLL_LINKAGE void SetContextMgmt(InitializeContextFunc initCtx, CleanupContextFunc cleanCtx);
		DLL_LINKAGE void SetControlMgmt(ExecutionBeginFunc execBegin, ExecutionControlFunc execCtl, ExecutionEndFunc execEnd, SyscallControlFunc syscallCtl);

		DLL_LINKAGE void SetContextSize(DWORD sz);
		DLL_LINKAGE void SetEntryPoint(ADDR_TYPE ep);

		DLL_LINKAGE void Initialize();
		DLL_LINKAGE void Execute(int argc, char *argv[]);

	};

};


#endif
