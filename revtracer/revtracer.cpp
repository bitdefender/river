#include "revtracer.h"

#include "execenv.h"
#include "callgates.h"

#include <x86intrin.h>


//void InitSymbolicHandler(ExecutionEnvironment *pEnv);
//void *CreateSymbolicVariable(const char *name);

namespace rev {

	/* Kernel32.dll replacements *********************************************************/

	typedef void *LPVOID, *PVOID;
	typedef long NTSTATUS;
	typedef void *HANDLE;
	typedef int BOOL;
	typedef const void *LPCVOID;
	typedef nodep::DWORD *LPDWORD;

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
		nodep::WORD    LimitLow;
		nodep::WORD    BaseLow;
		union {
			struct {
				nodep::BYTE    BaseMid;
				nodep::BYTE    Flags1;     // Declare as bytes to avoid alignment
				nodep::BYTE    Flags2;     // Problems.
				nodep::BYTE    BaseHi;
			} Bytes;
			struct {
				nodep::DWORD   BaseMid : 8;
				nodep::DWORD   Type : 5;
				nodep::DWORD   Dpl : 2;
				nodep::DWORD   Pres : 1;
				nodep::DWORD   LimitHi : 4;
				nodep::DWORD   Sys : 1;
				nodep::DWORD   Reserved_0 : 1;
				nodep::DWORD   Default_Big : 1;
				nodep::DWORD   Granularity : 1;
				nodep::DWORD   BaseHi : 8;
			} Bits;
		} HighWord;
	} LDT_ENTRY, *LPLDT_ENTRY;

	typedef nodep::DWORD THREADINFOCLASS;



	/* Default API functions ************************************************************/

	void NoDbgPrint(const unsigned int printMask, const char *fmt, ...) { }

	void DefaultIpcInitialize() {
	}

	void *DefaultMemoryAlloc(unsigned long dwSize) {
		DEBUG_BREAK;
		return nullptr;
	}

	void DefaultMemoryFree(void *ptr) {
		DEBUG_BREAK;
	}

	nodep::QWORD DefaultTakeSnapshot() {
		return 0;
	}

	nodep::QWORD DefaultRestoreSnapshot() {
		return 0;
	}

	void DefaultInitializeContextFunc(void *context) { }
	void DefaultCleanupContextFunc(void *context) { }

	nodep::DWORD DefaultExecutionBeginFunc(void *context, ADDR_TYPE nextInstruction, void *cbCtx) {
		return EXECUTION_ADVANCE;
	}

	nodep::DWORD DefaultExecutionControlFunc(void *context, ADDR_TYPE nextInstruction, void *cbCtx) {
		return EXECUTION_ADVANCE;
	}

	nodep::DWORD DefaultExecutionEndFunc(void *context, void *cbCtx) {
		return EXECUTION_TERMINATE;
	}

	nodep::DWORD DefaultBranchHandlerFunc(void *context, void *userContext, ADDR_TYPE nextInstruction) {
		return EXECUTION_ADVANCE;
	}

	nodep::DWORD DefaultErrorHandlerFunc(void *context, void *userContext, RevtracerError *rerror) {
		return EXECUTION_TERMINATE;
	}

	void DefaultSyscallControlFunc(void *context, void *userContext) { }

	void DefaultTrackCallback(nodep::DWORD value, nodep::DWORD address, nodep::DWORD segSel) { }
	void DefaultMarkCallback(nodep::DWORD oldValue, nodep::DWORD newValue, nodep::DWORD address, nodep::DWORD segSel) { }

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
	void GetFirstEsp(void *ctx, nodep::DWORD &esp) {
		struct ExecutionEnvironment *pCtx = (struct ExecutionEnvironment *)ctx;
		esp = pCtx->runtimeContext.firstEsp;
	}

	void GetCurrentRegisters(void *ctx, ExecutionRegs *regs) {
		struct ExecutionEnvironment *pCtx = (struct ExecutionEnvironment *)ctx;
		rev_memcpy(regs, (struct ExecutionEnvironment *)pCtx->runtimeContext.registers, sizeof(*regs));
		regs->esp = pCtx->runtimeContext.virtualStack;
	}

	void *GetMemoryInfo(void *ctx, ADDR_TYPE addr) {
		struct ExecutionEnvironment *pEnv = (struct ExecutionEnvironment *)ctx;
		nodep::DWORD ret = pEnv->ac.Get((nodep::DWORD)addr/* + revtracerConfig.segmentOffsets[segSel & 0xFFFF]*/);
		return (void *)ret;
	}

	bool GetLastBasicBlockInfo(void *ctx, BasicBlockInfo *info) {
		struct ExecutionEnvironment *pEnv = (struct ExecutionEnvironment *)ctx;

		RiverBasicBlock *pCB = pEnv->blockCache.FindBlock(pEnv->lastFwBlock);
		if (nullptr != pCB) {
			info->address = (rev::ADDR_TYPE)pCB->address;
			info->cost = pCB->dwOrigOpCount;
			info->branchType = pCB->dwBranchType;
			info->branchInstruction = pCB->dwBranchInstruction;
			info->nInstructions = pCB->dwOrigOpCount;
			// branch taken
			info->branchNext[0] = pCB->pBranchNext[0];
			//branch not taken
			info->branchNext[1] = pCB->pBranchNext[1];
		}
		return false;
	}


	/* Inproc API *************************************************************************/

	RevtracerImports revtracerImports = {
		NoDbgPrint,

		DefaultMemoryAlloc,
		DefaultMemoryFree,

		DefaultTakeSnapshot,
		DefaultRestoreSnapshot,

		DefaultInitializeContextFunc,
		DefaultCleanupContextFunc,
		
		DefaultBranchHandlerFunc,
		DefaultErrorHandlerFunc,
		DefaultSyscallControlFunc,

		DefaultIpcInitialize,

		DefaultTrackCallback,
		DefaultMarkCallback,

		DefaultSymbolicHandler,

		{
			(ADDR_TYPE)DefaultNtQueryInformationThread,
			(ADDR_TYPE)DefaultRtlNtStatusToDosError,

			(ADDR_TYPE)Defaultvsnprintf_s
		}
	};

	RevtracerConfig revtracerConfig = {
		0,
		0
	};

	RevtracerVersion revtracerVersion = {
		0,
		1,
		1
	};

	struct ExecutionEnvironment *pEnv = NULL;

	void CreateHook(ADDR_TYPE orig, ADDR_TYPE det) {
		RevtracerError rerror;
		RiverBasicBlock *pBlock = pEnv->blockCache.NewBlock((nodep::UINT_PTR)orig);

		pBlock->address = (nodep::DWORD)det;
		pEnv->codeGen.Translate(pBlock, revtracerConfig.featureFlags, &rerror);
		pBlock->address = (nodep::DWORD)orig;
		pBlock->dwFlags |= RIVER_BASIC_BLOCK_DETOUR;

		revtracerImports.dbgPrintFunc(PRINT_INFO | PRINT_CONTAINER, "Added detour from 0x%08x to 0x%08x\n", orig, det);
	}

#ifdef _MSC_VER
#define ADDR_OF_RET_ADDR _AddressOfReturnAddress
#else
#define ADDR_OF_RET_ADDR() ({ int addr; __asm__ ("lea 4(%%ebp), %0" : : "r" (addr)); addr; })
#endif

	extern "C" {
		nodep::DWORD miniStack[4096];
		nodep::DWORD shadowStack = (nodep::DWORD)&(miniStack[4090]);

		void TracerInitialization() { // parameter is not initialized (only used to get the 
			nodep::UINT_PTR rgs = (nodep::UINT_PTR)ADDR_OF_RET_ADDR() + sizeof(void *);

			Initialize();
			pEnv->runtimeContext.registers = rgs;

			revtracerImports.dbgPrintFunc(PRINT_INFO | PRINT_CONTAINER, "Entry point @%08x\n", (nodep::DWORD)revtracerConfig.entryPoint);
			RiverBasicBlock *pBlock = pEnv->blockCache.NewBlock((nodep::UINT_PTR)revtracerConfig.entryPoint);
			RevtracerError rerror;
			pBlock->address = (nodep::DWORD)revtracerConfig.entryPoint;
			pEnv->codeGen.Translate(pBlock, revtracerConfig.featureFlags, &rerror);

			revtracerImports.dbgPrintFunc(PRINT_INFO | PRINT_CONTAINER, "New entry point @%08x\n", (nodep::DWORD)pBlock->pFwCode);

			// TODO: replace with address of the actual terminate process
			pEnv->exitAddr = (nodep::DWORD)revtracerImports.lowLevel.ntTerminateProcess;

			/*pEnv->runtimeContext.execBuff -= 4;
			*((DWORD *)pEnv->runtimeContext.execBuff) = (DWORD)revtracerConfig.entryPoint;*/

			//switch (revtracerImports.executionBegin(pEnv->userContext, revtracerConfig.entryPoint, pEnv)) {
			switch (revtracerImports.branchHandler(pEnv, pEnv->userContext, revtracerConfig.entryPoint)) {
			case EXECUTION_ADVANCE:
				revtracerImports.dbgPrintFunc(PRINT_INFO | PRINT_CONTAINER, "%d detours needed.\n", revtracerConfig.hookCount);
				for (nodep::DWORD i = 0; i < revtracerConfig.hookCount; ++i) {
					CreateHook(revtracerConfig.hooks[i].originalAddr, revtracerConfig.hooks[i].detourAddr);
				}
				pEnv->lastFwBlock = (nodep::UINT_PTR)revtracerConfig.entryPoint;
				pEnv->bForward = 1;
				pBlock->MarkForward();

				revtracerImports.dbgPrintFunc(PRINT_INFO | PRINT_CONTAINER, "Translated entry point %08x.\n", pBlock->pFwCode);
				revtracerConfig.entryPoint = pBlock->pFwCode;
				break;
			case EXECUTION_TERMINATE:
				revtracerConfig.entryPoint = revtracerImports.lowLevel.ntTerminateProcess;
				break;
			case EXECUTION_BACKTRACK:
				revtracerImports.dbgPrintFunc(PRINT_INFO | PRINT_CONTAINER, "EXECUTION_BACKTRACK @executionBegin");
				revtracerConfig.entryPoint = revtracerImports.lowLevel.ntTerminateProcess;
				break;
			}
		}
	};

	nodep::DWORD __stdcall MarkAddr(void *pEnv, nodep::DWORD dwAddr, nodep::DWORD value, nodep::DWORD segSel);
	void MarkMemoryValue(void *ctx, ADDR_TYPE addr, nodep::DWORD value) {
		MarkAddr((ExecutionEnvironment *)ctx, (nodep::DWORD)addr, value, 0x2B);
	}

	/* DLL API ****************************************************************************/


	/*void SetDbgPrint(DbgPrintFunc func) {
		revtracerImports.dbgPrintFunc = func;
	}

	void SetMemoryMgmt(MemoryAllocFunc alc, MemoryFreeFunc fre) {
		revtracerImports.memoryAllocFunc = alc;
		revtracerImports.memoryFreeFunc = fre;
	}

	void SetSnapshotMgmt(TakeSnapshotFunc ts, RestoreSnapshotFunc rs) {
		revtracerImports.takeSnapshot = ts;
		revtracerImports.restoreSnapshot = rs;
	}

	void SetLowLevelAPI(LowLevelRevtracerAPI *llApi) {
		revtracerImports.lowLevel.ntTerminateProcess = llApi->ntTerminateProcess;
		revtracerImports.lowLevel.ntWriteFile = llApi->ntWriteFile;

		revtracerImports.lowLevel.vsnprintf_s = llApi->vsnprintf_s;
	}

	void SetContextMgmt(InitializeContextFunc initCtx, CleanupContextFunc cleanCtx) {
		revtracerImports.initializeContext = initCtx;
		revtracerImports.cleanupContext = cleanCtx;
	}

	//void SetControlMgmt(ExecutionControlFunc execCtl, SyscallControl syscallCtl) {
	void SetControlMgmt(BranchHandlerFunc branchCtl, SyscallControlFunc syscallCtl) {
		revtracerImports.branchHandler = branchCtl;
		revtracerImports.syscallControl = syscallCtl;
	}

	void SetContext(ADDR_TYPE ctx) {
		revtracerConfig.context = ctx;
	}

	void SetEntryPoint(ADDR_TYPE ep) {
		revtracerConfig.entryPoint = ep;
	}*/

	void Initialize() {
		revtracerImports.ipcLibInitialize();

		revtracerImports.dbgPrintFunc(PRINT_INFO | PRINT_CONTAINER, "Feature flags %08x, entrypoint %08x\n", revtracerConfig.featureFlags, revtracerConfig.entryPoint);

		pEnv = new ExecutionEnvironment(revtracerConfig.featureFlags, 0x1000000, 0x10000, 0x4000000, 0x4000000, 16, 0x10000);
		pEnv->userContext = revtracerConfig.context; //AllocUserContext(pEnv, revtracerConfig.contextSize);

		revtracerConfig.pRuntime = &pEnv->runtimeContext;
	}

	void Execute(int argc, char *argv[]) {
		nodep::DWORD ret;
		struct ExecutionRegs regs;

		pEnv->runtimeContext.registers = (nodep::UINT_PTR)&regs;
		if (EXECUTION_ADVANCE == revtracerImports.branchHandler(pEnv, pEnv->userContext, revtracerConfig.entryPoint)) {
			for (nodep::DWORD i = 0; i < revtracerConfig.hookCount; ++i) {
				CreateHook(revtracerConfig.hooks[i].originalAddr, revtracerConfig.hooks[i].detourAddr);
			}
			for (nodep::DWORD i = 0; i < revtracerConfig.hookCount; ++i) {
				CreateHook(revtracerConfig.hooks[i].originalAddr, revtracerConfig.hooks[i].detourAddr);
			}
			// TODO save state
			nodep::DWORD ret = call_cdecl_0(pEnv, (_fn_cdecl_0)revtracerConfig.entryPoint);
			// TODO if error=> restore state
			revtracerImports.dbgPrintFunc(PRINT_INFO | PRINT_CONTAINER, "Done. ret = %d\n\n", ret);
		}
	}

};

extern "C" {
	void RevtracerPerform();
#ifdef _MSC_VER
	NAKED void RevtracerPerform() {
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
	}
#endif
};

namespace rev {
	RevtracerExports revtracerExports = {
		GetFirstEsp,
		GetCurrentRegisters,
		GetMemoryInfo,
		GetLastBasicBlockInfo,
		MarkMemoryValue,

		::RevtracerPerform
	};
};
