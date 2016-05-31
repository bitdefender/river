#ifndef _REVTRACER_H
#define _REVTRACER_H

#include "DebugPrintFlags.h"
#include "BasicTypes.h"

#define TRACER_FEATURE_REVERSIBLE				0x00000001
#define TRACER_FEATURE_TRACKING					0x00000002
#define TRACER_FEATURE_ADVANCED_TRACKING		0x00000004 // never use this flag --- use _SYMBOLIC instead
#define TRACER_FEATURE_SYMBOLIC					TRACER_FEATURE_TRACKING | TRACER_FEATURE_ADVANCED_TRACKING

namespace rev {

#ifdef _BUILDING_REVTRACER_DLL
#define DLL_LINKAGE __declspec(dllexport)
#else
#define DLL_LINKAGE __declspec(dllimport)
#endif

	typedef void *ADDR_TYPE;

	typedef void(*DbgPrintFunc)(DWORD dwMask, const char *fmt, ...);
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

	typedef void(*TrackCallbackFunc)(DWORD value, DWORD address, DWORD segment);
	typedef void(*MarkCallbackFunc)(DWORD oldValue, DWORD newValue, DWORD address, DWORD segment);

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

		/* Variable tracking callbacks */
		TrackCallbackFunc trackCallback;
		MarkCallbackFunc markCallback;

		LowLevelRevtracerAPI lowLevel;
	};

	struct CodeHooks {
		ADDR_TYPE originalAddr;
		ADDR_TYPE detourAddr;
	};

	class SymbolicExecutor;
	class SymbolicEnvironment;
	typedef SymbolicExecutor *(*SymbolicExecutorConstructor)(SymbolicEnvironment *env);

	struct RevtracerConfig {
		ADDR_TYPE entryPoint;
		ADDR_TYPE mainModule;
		DWORD contextSize;
		DWORD segmentOffsets[0x100];

		DWORD featureFlags;

		BOOL dumpBlocks;
		HANDLE hBlocks;

		void *pRuntime;

		DWORD hookCount;
		CodeHooks hooks[0x10];

		SymbolicExecutorConstructor sCons;
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
		DLL_LINKAGE void *GetMemoryInfo(void *ctx, ADDR_TYPE addr);

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

		DLL_LINKAGE void SetSymbolicExecutor(SymbolicExecutorConstructor func);

		DLL_LINKAGE void Initialize();
		DLL_LINKAGE void Execute(int argc, char *argv[]);

		DLL_LINKAGE void MarkMemoryName(ADDR_TYPE addr, const char *name);
		DLL_LINKAGE void MarkMemoryValue(ADDR_TYPE addr, DWORD value);
	};

};


#endif
