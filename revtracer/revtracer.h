#ifndef _REVTRACER_H
#define _REVTRACER_H

#include "DebugPrintFlags.h"
#include "BasicTypes.h"
#include "common.h"

#define TRACER_FEATURE_REVERSIBLE				0x00000001
#define TRACER_FEATURE_TRACKING					0x00000002
#define TRACER_FEATURE_ADVANCED_TRACKING		0x00000004 // never use this flag --- use _SYMBOLIC instead
#define TRACER_FEATURE_SYMBOLIC					(TRACER_FEATURE_TRACKING | TRACER_FEATURE_ADVANCED_TRACKING)

namespace rev {

#if defined _WIN32 || defined __CYGWIN__
	#ifdef _BUILDING_REVTRACER_DLL
		#ifdef _MSC_VER
			#define DLL_PUBLIC __declspec(dllexport)
			#define NAKED  __declspec(naked)
		#else
			#define DLL_PUBLIC __attribute__ ((dllexport))
			#define NAKED
		#endif
	#else
		#ifdef _MSC_VER
			#define DLL_PUBLIC __declspec(dllimport)
		#else
			#define DLL_PUBLIC __attribute__ ((dllimport))
		#endif
	#endif
	#define DLL_LOCAL
#else
	#if __GNUC__ >= 4
		#define DLL_PUBLIC __attribute__ ((visibility ("default")))
		#define DLL_LOCAL  __attribute__ ((visibility ("hidden")))
	  #define NAKED  __attribute__ ((naked))
	#else
		#define DLL_PUBLIC
		#define DLL_LOCAL
	#endif
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

	typedef void(__stdcall *SymbolicHandlerFunc)(void *context, void *offset, void *instr);

	//Revtracer Wrapper API functions type
	typedef bool (*WriteFileCall)(void *handle, int fd, void *buffer, size_t size, unsigned long *written);

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

		/* IpcLib initialization */
		IpcLibInitFunc ipcLibInitialize;

		/* Variable tracking callbacks */
		TrackCallbackFunc trackCallback;
		MarkCallbackFunc markCallback;

		/* Symbolic synchronization function */
		SymbolicHandlerFunc symbolicHandler;

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
		DLL_PUBLIC void GetCurrentRegisters(void *ctx, ExecutionRegs *regs);
		DLL_PUBLIC void *GetMemoryInfo(void *ctx, ADDR_TYPE addr);
		DLL_PUBLIC DWORD GetLastBasicBlockCost(void *ctx);
		DLL_PUBLIC void MarkMemoryValue(void *ctx, ADDR_TYPE addr, DWORD value);

		/* In process API *********************************/

		DLL_PUBLIC extern RevtracerAPI revtracerAPI;
		DLL_PUBLIC extern RevtracerConfig revtracerConfig;
		/* Can be used as an EP for in process execution  */
		DLL_PUBLIC void RevtracerPerform();


		/* DLL API ****************************************/

		DLL_PUBLIC void SetDbgPrint(DbgPrintFunc);
		DLL_PUBLIC void SetMemoryMgmt(MemoryAllocFunc alc, MemoryFreeFunc fre);
		DLL_PUBLIC void SetSnapshotMgmt(TakeSnapshotFunc ts, RestoreSnapshotFunc rs);
		DLL_PUBLIC void SetLowLevelAPI(LowLevelRevtracerAPI *llApi);
		DLL_PUBLIC void SetContextMgmt(InitializeContextFunc initCtx, CleanupContextFunc cleanCtx);
		DLL_PUBLIC void SetControlMgmt(BranchHandlerFunc branchCtl, SyscallControlFunc syscallCtl);


		DLL_PUBLIC void SetContext(ADDR_TYPE ctx);
		DLL_PUBLIC void SetEntryPoint(ADDR_TYPE ep);

		DLL_PUBLIC void Initialize();
		DLL_PUBLIC void Execute(int argc, char *argv[]);
	};

};


#endif
