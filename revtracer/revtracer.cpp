#include "revtracer.h"

#include "execenv.h"
#include "callgates.h"

#include <intrin.h>


void InitSymbolicHandler(ExecutionEnvironment *pEnv);
void *CreateSymbolicVariable(const char *name);

namespace rev {

	/* Kernel32.dll replacements *********************************************************/

	typedef void *LPVOID, *PVOID;
	typedef long NTSTATUS;
	typedef void *HANDLE;
	typedef int BOOL;
	typedef const void *LPCVOID;
	typedef DWORD *LPDWORD;

	typedef unsigned long ULONG;

#define TRUE  1
#define FALSE 0

#define PAGE_EXECUTE           0x10     
#define PAGE_EXECUTE_READ      0x20     
#define PAGE_EXECUTE_READWRITE 0x40     
#define PAGE_EXECUTE_WRITECOPY 0x80     

#define MEM_COMMIT                  0x1000      
#define MEM_RESERVE                 0x2000      

	typedef void* (*AllocateMemoryCall)(size_t size);

	HANDLE Kernel32GetCurrentThread() {
		return (HANDLE)0xFFFFFFFE;
	}

	typedef struct {
		WORD    LimitLow;
		WORD    BaseLow;
		union {
			struct {
				BYTE    BaseMid;
				BYTE    Flags1;     // Declare as bytes to avoid alignment
				BYTE    Flags2;     // Problems.
				BYTE    BaseHi;
			} Bytes;
			struct {
				DWORD   BaseMid : 8;
				DWORD   Type : 5;
				DWORD   Dpl : 2;
				DWORD   Pres : 1;
				DWORD   LimitHi : 4;
				DWORD   Sys : 1;
				DWORD   Reserved_0 : 1;
				DWORD   Default_Big : 1;
				DWORD   Granularity : 1;
				DWORD   BaseHi : 8;
			} Bits;
		} HighWord;
	} LDT_ENTRY, *LPLDT_ENTRY;

	typedef DWORD THREADINFOCLASS;



	/* Default API functions ************************************************************/

	void NoDbgPrint(const unsigned int printMask, const char *fmt, ...) { }

	void DefaultIpcInitialize() {
	}

	void *DefaultMemoryAlloc(unsigned long dwSize) {
		return ((AllocateMemoryCall)revtracerAPI.lowLevel.ntAllocateVirtualMemory)(dwSize);
	}

	void DefaultMemoryFree(void *ptr) {
		//TODO: implement VirtualFree
	}

	QWORD DefaultTakeSnapshot() {
		return 0;
	}

	QWORD DefaultRestoreSnapshot() {
		return 0;
	}

	void DefaultInitializeContextFunc(void *context) { }
	void DefaultCleanupContextFunc(void *context) { }

	DWORD DefaultExecutionBeginFunc(void *context, ADDR_TYPE nextInstruction, void *cbCtx) {
		return EXECUTION_ADVANCE;
	}

	DWORD DefaultExecutionControlFunc(void *context, ADDR_TYPE nextInstruction, void *cbCtx) {
		return EXECUTION_ADVANCE;
	}

	DWORD DefaultExecutionEndFunc(void *context, void *cbCtx) {
		return EXECUTION_TERMINATE;
	}

	DWORD DefaultBranchHandlerFunc(void *context, void *userContext, ADDR_TYPE nextInstruction) {
		return EXECUTION_ADVANCE;
	}

	void DefaultSyscallControlFunc(void *context, void *userContext) { }

	void DefaultTrackCallback(DWORD value, DWORD address, DWORD segSel) { }
	void DefaultMarkCallback(DWORD oldValue, DWORD newValue, DWORD address, DWORD segSel) { }

	void DefaultNtAllocateVirtualMemory() {
		DEBUG_BREAK;
	}

	void DefaultNtFreeVirtualMemory() {
		DEBUG_BREAK;
	}

	void DefaultNtQueryInformationThread() {
		DEBUG_BREAK;
	}

	void DefaultRtlNtStatusToDosError() {
		DEBUG_BREAK;
	}

	void Defaultvsnprintf_s() {
		DEBUG_BREAK;
	}

	void __stdcall DefaultSymbolicHandler(void *context, void *offset, void *address) {
		return;
	}

	/* Execution context callbacks ********************************************************/
	void GetCurrentRegisters(void *ctx, ExecutionRegs *regs) {
		struct ExecutionEnvironment *pCtx = (struct ExecutionEnvironment *)ctx;
		rev_memcpy(regs, (struct ExecutionEnvironment *)pCtx->runtimeContext.registers, sizeof(*regs));
		regs->esp = pCtx->runtimeContext.virtualStack;
	}

	void *GetMemoryInfo(void *ctx, ADDR_TYPE addr) {
		struct ExecutionEnvironment *pEnv = (struct ExecutionEnvironment *)ctx;
		DWORD ret = pEnv->ac.Get((DWORD)addr/* + revtracerConfig.segmentOffsets[segSel & 0xFFFF]*/);
		return (void *)ret;
	}


	/* Inproc API *************************************************************************/

	RevtracerAPI revtracerAPI = {
		NoDbgPrint,

		DefaultMemoryAlloc,
		DefaultMemoryFree,

		DefaultTakeSnapshot,
		DefaultRestoreSnapshot,

		DefaultInitializeContextFunc,
		DefaultCleanupContextFunc,
		
		DefaultBranchHandlerFunc,
		DefaultSyscallControlFunc,

		DefaultIpcInitialize,

		DefaultTrackCallback,
		DefaultMarkCallback,

		DefaultSymbolicHandler,

		{
			(ADDR_TYPE)DefaultNtAllocateVirtualMemory,
			(ADDR_TYPE)DefaultNtFreeVirtualMemory,

			(ADDR_TYPE)DefaultNtQueryInformationThread,
			(ADDR_TYPE)DefaultRtlNtStatusToDosError,

			(ADDR_TYPE)Defaultvsnprintf_s
		}
	};

	RevtracerConfig revtracerConfig = {
		0,
		0
	};

	DWORD miniStack[4096];
	DWORD shadowStack = (DWORD)&(miniStack[4090]);

	struct ExecutionEnvironment *pEnv = NULL;

	void CreateHook(ADDR_TYPE orig, ADDR_TYPE det) {
		RiverBasicBlock *pBlock = pEnv->blockCache.NewBlock((UINT_PTR)orig);
		pBlock->address = (DWORD)det;
		pEnv->codeGen.Translate(pBlock, revtracerConfig.featureFlags);
		pBlock->address = (DWORD)orig;
		pBlock->dwFlags |= RIVER_BASIC_BLOCK_DETOUR;

		revtracerAPI.dbgPrintFunc(PRINT_INFO | PRINT_CONTAINER, "Added detour from 0x%08x to 0x%08x\n", orig, det);
	}

  void *AddressOfReturnAddress(void) {
    int addr;
    __asm__ ("lea 4(%%ebp), %0" : : "r" (addr));
    return (void*)addr;
  }

	void TracerInitialization() { // parameter is not initialized (only used to get the 
		UINT_PTR rgs = (UINT_PTR)AddressOfReturnAddress() + sizeof(void *);
		
		Initialize();

		pEnv->runtimeContext.registers = rgs;

		revtracerAPI.dbgPrintFunc(PRINT_INFO | PRINT_CONTAINER, "Entry point @%08x\n", revtracerConfig.entryPoint);
		RiverBasicBlock *pBlock = pEnv->blockCache.NewBlock((UINT_PTR)revtracerConfig.entryPoint);
		pBlock->address = (DWORD)revtracerConfig.entryPoint;
		pEnv->codeGen.Translate(pBlock, revtracerConfig.featureFlags);
		
		pEnv->exitAddr = (DWORD)revtracerAPI.lowLevel.ntTerminateProcess;

		/*pEnv->runtimeContext.execBuff -= 4;
		*((DWORD *)pEnv->runtimeContext.execBuff) = (DWORD)revtracerConfig.entryPoint;*/
		
		//switch (revtracerAPI.executionBegin(pEnv->userContext, revtracerConfig.entryPoint, pEnv)) {
		switch (revtracerAPI.branchHandler(pEnv, pEnv->userContext, revtracerConfig.entryPoint)) {
			case EXECUTION_ADVANCE :
				revtracerAPI.dbgPrintFunc(PRINT_INFO | PRINT_CONTAINER, "%d detours needed.\n", revtracerConfig.hookCount);
				for (DWORD i = 0; i < revtracerConfig.hookCount; ++i) {
					CreateHook(revtracerConfig.hooks[i].originalAddr, revtracerConfig.hooks[i].detourAddr);
				}
				pEnv->lastFwBlock = (UINT_PTR)revtracerConfig.entryPoint;
				pEnv->bForward = 1;
				pBlock->MarkForward();

				revtracerAPI.dbgPrintFunc(PRINT_INFO | PRINT_CONTAINER, "Translated entry point %08x.\n", pBlock->pFwCode);
				revtracerConfig.entryPoint = pBlock->pFwCode;
				break;
			case EXECUTION_TERMINATE :
				revtracerConfig.entryPoint = revtracerAPI.lowLevel.ntTerminateProcess;
				break;
			case EXECUTION_BACKTRACK :
				revtracerAPI.dbgPrintFunc(PRINT_INFO | PRINT_CONTAINER, "EXECUTION_BACKTRACK @executionBegin");
				revtracerConfig.entryPoint = revtracerAPI.lowLevel.ntTerminateProcess;
				break;
		}
	}

	NAKED  void RevtracerPerform() {
#ifdef _MSC_VER
		__asm {
			xchg esp, shadowStack;
			pushad;
			pushfd;
			call TracerInitialization;
			popfd;
			popad;
			xchg esp, shadowStack;

			jmp dword ptr[revtracerConfig.entryPoint];
		}
#else
		__asm__ (
				"xchgl %1, %%esp             \n\t"
				"pushal                      \n\t"
				"pushfl                      \n\t"
				"call %P2                    \n\t"
				"popfl                       \n\t"
				"popal                       \n\t"
				"xchgl %1, %%esp             \n\t"
				"jmp *%0" : : "r" (revtracerConfig.entryPoint),
				"r" (shadowStack), "i" (TracerInitialization)
				);
#endif
	}




	/* Segment initialization *************************************************************/

	//DWORD segmentOffsets[0x100];
	/*void InitSegment(DWORD dwSeg) {
		LDT_ENTRY entry;
		Kernel32GetThreadSelectorEntry(Kernel32GetCurrentThread(), dwSeg, &entry);

		DWORD base = entry.BaseLow | (entry.HighWord.Bytes.BaseMid << 16) | (entry.HighWord.Bytes.BaseHi << 24);
		DWORD limit = entry.LimitLow | (entry.HighWord.Bits.LimitHi << 16);

		if (entry.HighWord.Bits.Granularity) {
			limit = (limit << 12) | 0x0FFF;
		}

		segmentOffsets[dwSeg] = base;
	}


	void InitSegments() {
		for (DWORD i = 0; i < 0x100; ++i) {
			InitSegment(i);
		}
	}*/

	/* DLL API ****************************************************************************/


	void SetDbgPrint(DbgPrintFunc func) {
		revtracerAPI.dbgPrintFunc = func;
	}

	void SetMemoryMgmt(MemoryAllocFunc alc, MemoryFreeFunc fre) {
		revtracerAPI.memoryAllocFunc = alc;
		revtracerAPI.memoryFreeFunc = fre;
	}

	void SetSnapshotMgmt(TakeSnapshotFunc ts, RestoreSnapshotFunc rs) {
		revtracerAPI.takeSnapshot = ts;
		revtracerAPI.restoreSnapshot = rs;
	}

	void SetLowLevelAPI(LowLevelRevtracerAPI *llApi) {
		revtracerAPI.lowLevel.ntAllocateVirtualMemory = llApi->ntAllocateVirtualMemory;
		revtracerAPI.lowLevel.ntFreeVirtualMemory = llApi->ntFreeVirtualMemory;
		revtracerAPI.lowLevel.ntQueryInformationThread = llApi->ntQueryInformationThread;

		revtracerAPI.lowLevel.rtlNtStatusToDosError = llApi->rtlNtStatusToDosError;

		revtracerAPI.lowLevel.vsnprintf_s = llApi->vsnprintf_s;
	}

	void SetContextMgmt(InitializeContextFunc initCtx, CleanupContextFunc cleanCtx) {
		revtracerAPI.initializeContext = initCtx;
		revtracerAPI.cleanupContext = cleanCtx;
	}

	//void SetControlMgmt(ExecutionControlFunc execCtl, SyscallControlFunc syscallCtl) {
	void SetControlMgmt(BranchHandlerFunc branchCtl, SyscallControlFunc syscallCtl) {
		revtracerAPI.branchHandler = branchCtl;
		revtracerAPI.syscallControl = syscallCtl;
	}

	void SetContext(ADDR_TYPE ctx) {
		revtracerConfig.context = ctx;
	}

	void SetEntryPoint(ADDR_TYPE ep) {
		revtracerConfig.entryPoint = ep;
	}

	void Initialize() {
		revtracerAPI.ipcLibInitialize();

		revtracerAPI.dbgPrintFunc(PRINT_INFO | PRINT_CONTAINER, "Feature flags %08x\n", revtracerConfig.featureFlags);

		pEnv = new ExecutionEnvironment(revtracerConfig.featureFlags, 0x1000000, 0x10000, 0x4000000, 0x4000000, 16, 0x10000);
		pEnv->userContext = revtracerConfig.context; //AllocUserContext(pEnv, revtracerConfig.contextSize);

		revtracerConfig.pRuntime = &pEnv->runtimeContext;
	}

	void Execute(int argc, char *argv[]) {
		DWORD ret;
		//if (EXECUTION_ADVANCE == revtracerAPI.executionBegin(pEnv->userContext, revtracerConfig.entryPoint, pEnv)) {
		if (EXECUTION_ADVANCE == revtracerAPI.branchHandler(pEnv, pEnv->userContext, revtracerConfig.entryPoint)) {
			ret = call_cdecl_2(pEnv, (_fn_cdecl_2)revtracerConfig.entryPoint, (void *)argc, (void *)argv);
			revtracerAPI.dbgPrintFunc(PRINT_INFO | PRINT_CONTAINER, "Done. ret = %d\n\n", ret);
		}
	}

	DWORD __stdcall MarkAddr(void *pEnv, DWORD dwAddr, DWORD value, DWORD segSel);
	void MarkMemoryValue(void *ctx, ADDR_TYPE addr, DWORD value) {
		MarkAddr((ExecutionEnvironment *)ctx, (DWORD)addr, value, 0x2B);
	}

};
