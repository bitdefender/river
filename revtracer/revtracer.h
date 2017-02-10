#ifndef _REVTRACER_H
#define _REVTRACER_H

#include "DebugPrintFlags.h"
#include "../CommonCrossPlatform/BasicTypes.h"
#include "common.h"

#define TRACER_FEATURE_REVERSIBLE				0x00000001
#define TRACER_FEATURE_TRACKING					0x00000002
#define TRACER_FEATURE_ADVANCED_TRACKING		0x00000004 // never use this flag --- use _SYMBOLIC instead
#define TRACER_FEATURE_SYMBOLIC					(TRACER_FEATURE_TRACKING | TRACER_FEATURE_ADVANCED_TRACKING)

namespace rev {

#if defined _WIN32 || defined __CYGWIN__
	#ifdef _BUILDING_REVTRACER_DLL
		#ifdef _MSC_VER
			#define DLL_REVTRACER_PUBLIC __declspec(dllexport)
			#define NAKED  __declspec(naked)
		#else
			#define DLL_REVTRACER_PUBLIC __attribute__ ((dllexport))
			#define NAKED
		#endif
	#else
		#ifdef _MSC_VER
			#define DLL_REVTRACER_PUBLIC __declspec(dllimport)
		#else
			#define DLL_REVTRACER_PUBLIC __attribute__ ((dllimport))
		#endif
	#endif
	#define DLL_LOCAL
#else
	#if __GNUC__ >= 4
		#define DLL_REVTRACER_PUBLIC __attribute__ ((visibility ("default")))
		#define DLL_LOCAL  __attribute__ ((visibility ("hidden")))
	  #define NAKED  __attribute__ ((naked))
	#else
		#define DLL_REVTRACER_PUBLIC
		#define DLL_LOCAL
	#endif
#endif


	typedef void *ADDR_TYPE;

	typedef void(*DbgPrintFunc)(const unsigned int dwMask, const char *fmt, ...);
	typedef void *(*MemoryAllocFunc)(nodep::DWORD dwSize);
	typedef void(*MemoryFreeFunc)(void *ptr);

	typedef nodep::QWORD(*TakeSnapshotFunc)();
	typedef nodep::QWORD(*RestoreSnapshotFunc)();

#define EXECUTION_ADVANCE					0x00000000
#define EXECUTION_BACKTRACK					0x00000001
#define EXECUTION_TERMINATE					0x00000002
#define EXECUTION_RESTART					0x00000003

	typedef void(*InitializeContextFunc)(void *context);
	typedef void(*CleanupContextFunc)(void *context);

	typedef nodep::DWORD(*BranchHandlerFunc)(void *context, void *userContext, ADDR_TYPE nextInstruction);
	typedef void(*SyscallControlFunc)(void *context, void *userContext);
	typedef void(*IpcLibInitFunc)();

	typedef void(*TrackCallbackFunc)(nodep::DWORD value, nodep::DWORD address, nodep::DWORD segment);
	typedef void(*MarkCallbackFunc)(nodep::DWORD oldValue, nodep::DWORD newValue, nodep::DWORD address, nodep::DWORD segment);

	typedef void(__stdcall *SymbolicHandlerFunc)(void *context, void *offset, void *instr);

	//Revtracer Wrapper API functions type
	typedef bool (*WriteFileCall)(void *handle, int fd, void *buffer, size_t size, unsigned long *written);

	struct LowLevelRevtracerAPI {
		/* Low level ntdll.dll functions */
		//ADDR_TYPE ntAllocateVirtualMemory;
		//ADDR_TYPE ntFreeVirtualMemory;
		//ADDR_TYPE ntQueryInformationThread;
		ADDR_TYPE ntTerminateProcess;

		ADDR_TYPE ntWriteFile;
		ADDR_TYPE vsnprintf_s;
	};

	struct RevtracerImports {
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
		nodep::DWORD segmentOffsets[0x100];

		nodep::DWORD featureFlags;

		nodep::BOOL dumpBlocks;
		nodep::HANDLE hBlocks;

		void *pRuntime;

		nodep::DWORD hookCount;
		CodeHooks hooks[0x100];
	};

	struct ExecutionRegs {
		nodep::DWORD edi;
		nodep::DWORD esi;
		nodep::DWORD ebp;
		nodep::DWORD esp;

		nodep::DWORD ebx;
		nodep::DWORD edx;
		nodep::DWORD ecx;
		nodep::DWORD eax;
		nodep::DWORD eflags;
	};

	struct BasicBlockInfo {
		ADDR_TYPE address;
		nodep::DWORD cost;
	};

	typedef void (*GetCurrentRegistersFunc)(void *ctx, ExecutionRegs *regs);
	typedef void *(*GetMemoryInfoFunc)(void *ctx, ADDR_TYPE addr);
	typedef bool (*GetLastBasicBlockInfoFunc)(void *ctx, BasicBlockInfo *info);
	typedef void (*MarkMemoryValueFunc)(void *ctx, ADDR_TYPE addr, nodep::DWORD value);
	typedef void (*RevtracerPerformFunc)();

	struct RevtracerExports {
		GetCurrentRegistersFunc getCurrentRegisters;
		GetMemoryInfoFunc getMemoryInfo;
		GetLastBasicBlockInfoFunc getLastBasicBlockInfo;
		MarkMemoryValueFunc markMemoryValue;

		/* Can be used as an EP for in process execution  */
		RevtracerPerformFunc revtracerPerform;
	};

	extern "C" {
		/* In process API *********************************/
		DLL_REVTRACER_PUBLIC extern RevtracerConfig revtracerConfig;
		DLL_REVTRACER_PUBLIC extern RevtracerImports revtracerImports;
		DLL_REVTRACER_PUBLIC extern RevtracerExports revtracerExports;


		/* DLL API ****************************************/

		DLL_REVTRACER_PUBLIC void SetDbgPrint(DbgPrintFunc);
		DLL_REVTRACER_PUBLIC void SetMemoryMgmt(MemoryAllocFunc alc, MemoryFreeFunc fre);
		DLL_REVTRACER_PUBLIC void SetSnapshotMgmt(TakeSnapshotFunc ts, RestoreSnapshotFunc rs);
		DLL_REVTRACER_PUBLIC void SetLowLevelAPI(LowLevelRevtracerAPI *llApi);
		DLL_REVTRACER_PUBLIC void SetContextMgmt(InitializeContextFunc initCtx, CleanupContextFunc cleanCtx);
		DLL_REVTRACER_PUBLIC void SetControlMgmt(BranchHandlerFunc branchCtl, SyscallControlFunc syscallCtl);


		DLL_REVTRACER_PUBLIC void SetContext(ADDR_TYPE ctx);
		DLL_REVTRACER_PUBLIC void SetEntryPoint(ADDR_TYPE ep);

		DLL_REVTRACER_PUBLIC void Initialize();
		DLL_REVTRACER_PUBLIC void Execute(int argc, char *argv[]);
	};

};


#endif
