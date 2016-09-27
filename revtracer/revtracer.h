#ifndef _REVTRACER_H
#define _REVTRACER_H

#include "DebugPrintFlags.h"
#include "BasicTypes.h"

#define TRACER_FEATURE_REVERSIBLE				0x00000001
#define TRACER_FEATURE_TRACKING					0x00000002
#define TRACER_FEATURE_ADVANCED_TRACKING		0x00000004 // never use this flag --- use _SYMBOLIC instead
#define TRACER_FEATURE_SYMBOLIC					(TRACER_FEATURE_TRACKING | TRACER_FEATURE_ADVANCED_TRACKING)

namespace rev {

#ifdef _BUILDING_REVTRACER_DLL
#define DLL_LINKAGE __declspec(dllexport)
#else
#define DLL_LINKAGE __declspec(dllimport)
#endif

	typedef void *ADDR_TYPE;

	typedef void(*DbgPrintFunc)(const unsigned int dwMask, const char *fmt, ...);
	typedef void *(*MemoryAllocFunc)(DWORD dwSize);
	typedef void(*MemoryFreeFunc)(void *ptr);

	typedef QWORD(*TakeSnapshotFunc)();
	typedef QWORD(*RestoreSnapshotFunc)();

#define EXECUTION_ADVANCE					0x00000000
#define EXECUTION_BACKTRACK					0x00000001
#define EXECUTION_TERMINATE					0x00000002

	typedef void(*InitializeContextFunc)(void *context);
	typedef void(*CleanupContextFunc)(void *context);

	typedef DWORD(*BranchHandlerFunc)(void *context, void *userContext, ADDR_TYPE nextInstruction);
	typedef void(*SyscallControlFunc)(void *context, void *userContext);
	typedef void(*IpcLibInitFunc)();

	typedef void(*TrackCallbackFunc)(DWORD value, DWORD address, DWORD segment);
	typedef void(*MarkCallbackFunc)(DWORD oldValue, DWORD newValue, DWORD address, DWORD segment);

	typedef ADDR_TYPE(*GetExceptingIpFunc)(void *context, void *userContext, ADDR_TYPE hookAddress);
	typedef void (*SetExceptingIpFunc)(void *context, void *userContext, ADDR_TYPE hookAddress, ADDR_TYPE newIp, ADDR_TYPE *newStack);
	typedef void(*ApplyHookFunc)(void *context, void *userContext, ADDR_TYPE originalAddr, ADDR_TYPE hookedAddr);

	typedef void(__stdcall *SymbolicHandlerFunc)(void *context, void *offset, void *instr);

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
		//ExecutionBeginFunc executionBegin;
		//ExecutionControlFunc executionControl;
		//ExecutionEndFunc executionEnd;
		BranchHandlerFunc branchHandler;
		SyscallControlFunc syscallControl;

		/* Exception handling callbacks */
		ApplyHookFunc applyHook;
		GetExceptingIpFunc getExceptingIp;
		SetExceptingIpFunc setExceptingIp;

		/* IpcLib initialization */
		IpcLibInitFunc ipcLibInitialize;

		/* Variable tracking callbacks */
		TrackCallbackFunc trackCallback;
		MarkCallbackFunc markCallback;

		/* Symbolic synchronization function */
		SymbolicHandlerFunc symbolicHandler;

		LowLevelRevtracerAPI lowLevel;
	};

// these hooks are used for substituting pieces of code
// both original and detour addresses are used
#define HOOK_NORMAL					0x00000000
// these hooks are used for catching back the execution
// only original address is used
// WARNING: native hooks modify process memory
#define HOOK_NATIVE					0x00000001

	struct CodeHooks {
		ADDR_TYPE originalAddr;
		ADDR_TYPE detourAddr;
		DWORD     hookFlags;
	};

	class SymbolicExecutor;
	class SymbolicEnvironment;
	typedef SymbolicExecutor *(*SymbolicExecutorConstructor)(SymbolicEnvironment *env);

	struct RevtracerConfig {
		ADDR_TYPE entryPoint;
		ADDR_TYPE mainModule;
		ADDR_TYPE context;
		DWORD segmentOffsets[0x100];

		DWORD featureFlags;

		BOOL dumpBlocks;
		HANDLE hBlocks;

		void *pRuntime;

		DWORD hookCount;
		CodeHooks hooks[0x10];
	};

	struct ExecutionRegs {
		DWORD edi;			// + 0x00
		DWORD esi;			// + 0x04
		DWORD ebp;			// + 0x08
		DWORD esp;			// + 0x0C

		DWORD ebx;			// + 0x10
		DWORD edx;			// + 0x14
		DWORD ecx;			// + 0x18
		DWORD eax;			// + 0x1C
		DWORD eflags;		// + 0x20
	};

	extern "C" {

		/* Execution context callbacks ********************/
		DLL_LINKAGE void GetCurrentRegisters(void *ctx, ExecutionRegs *regs);
		DLL_LINKAGE void *GetMemoryInfo(void *ctx, ADDR_TYPE addr);
		DLL_LINKAGE void MarkMemoryValue(void *ctx, ADDR_TYPE addr, DWORD value);

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
		DLL_LINKAGE void SetControlMgmt(BranchHandlerFunc branchCtl, SyscallControlFunc syscallCtl);


		DLL_LINKAGE void SetContext(ADDR_TYPE ctx);
		DLL_LINKAGE void SetEntryPoint(ADDR_TYPE ep);

		DLL_LINKAGE void Initialize();
		DLL_LINKAGE void Execute(int argc, char *argv[]);
	};

};


#endif
