#include "revtracer.h"

#include "execenv.h"
#include "callgates.h"


namespace rev {

	/* Kernel32.dll replacements *********************************************************/

	typedef void *LPVOID;
	typedef long NTSTATUS;
	typedef void *HANDLE;
	typedef int BOOL;

	#define TRUE  1
	#define FALSE 0

	#ifndef NT_SUCCESS
	#define NT_SUCCESS(Status) (((NTSTATUS)(Status)) >= 0)
	#endif

	DLL_LINKAGE DWORD revtracerLastError;

	typedef DWORD(__stdcall *RtlNtStatusToDosErrorFunc)(NTSTATUS status);

	DWORD Kernel32BaseSetLastNTError(NTSTATUS status) {
		revtracerLastError = ((RtlNtStatusToDosErrorFunc)revtracerAPI.lowLevel.rtlNtStatusToDosError)(status);
		return revtracerLastError;
	}

	#define PAGE_EXECUTE           0x10     
	#define PAGE_EXECUTE_READ      0x20     
	#define PAGE_EXECUTE_READWRITE 0x40     
	#define PAGE_EXECUTE_WRITECOPY 0x80     

	#define MEM_COMMIT                  0x1000      
	#define MEM_RESERVE                 0x2000      

	typedef NTSTATUS(__stdcall *NtAllocateVirtualMemoryFunc)(
		HANDLE               ProcessHandle,
		LPVOID               *BaseAddress,
		DWORD                ZeroBits,
		size_t               *RegionSize,
		DWORD                AllocationType,
		DWORD                Protect);

	LPVOID __stdcall Kernel32VirtualAllocEx(HANDLE hProcess, LPVOID lpAddress, size_t dwSize, DWORD flAllocationType, DWORD flProtect) {
		LPVOID addr = lpAddress;
		if (lpAddress && (unsigned int)lpAddress < 0x10000) {
			//RtlSetLastWin32Error(87);
		}
		else {
			NTSTATUS ret = ((NtAllocateVirtualMemoryFunc)revtracerAPI.lowLevel.ntAllocateVirtualMemory)(
				hProcess,
				&addr,
				0,
				&dwSize,
				flAllocationType & 0xFFFFFFF0,
				flProtect
				);
			if (NT_SUCCESS(ret)) {
				return addr;
			}
			Kernel32BaseSetLastNTError(ret);
		}
		return 0;
	}


	LPVOID __stdcall Kernel32VirtualAlloc(LPVOID lpAddress, size_t dwSize, DWORD flAllocationType, DWORD flProtect) {
		return Kernel32VirtualAllocEx((HANDLE)0xFFFFFFFF, lpAddress, dwSize, flAllocationType, flProtect);
	}


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

	typedef NTSTATUS(__stdcall *NtQueryInformationThreadFunc)(
		HANDLE ThreadHandle,
		THREADINFOCLASS ThreadInformationClass,
		void *ThreadInformation,
		DWORD ThreadInformationLength,
		DWORD *ReturnLength
	);

	BOOL Kernel32GetThreadSelectorEntry(HANDLE hThread, DWORD dwSeg, LPLDT_ENTRY entry) {
		NTSTATUS ret;
		DWORD segBuffer[3];

		segBuffer[0] = dwSeg;
		ret = ((NtQueryInformationThreadFunc)revtracerAPI.lowLevel.ntQueryInformationThread)(
			hThread,
			(THREADINFOCLASS)6,
			&segBuffer,
			sizeof(segBuffer),
			NULL
			);

		if (NT_SUCCESS(ret)) {
			*(DWORD *)&entry->LimitLow = segBuffer[1];
			*(DWORD *)&entry->HighWord.Bytes.BaseMid = segBuffer[2];
			return TRUE;
		}
		else {
			Kernel32BaseSetLastNTError(ret);
			return FALSE;
		}
	}


	typedef NTSTATUS(__stdcall *NtTerminateProcessFunc)(
		HANDLE ProcessHandle,
		NTSTATUS ExitStatus
	);

	BOOL Kernel32TerminateProcess(HANDLE hProcess, DWORD uExitCode) {
		return ((NtTerminateProcessFunc)revtracerAPI.lowLevel.ntTerminateProcess)(hProcess, uExitCode);
	}

	/* Default API functions ************************************************************/

	void NoDbgPrint(const char *fmt, ...) { }

	void DefaultIpcInitialize() {
	}

	void *DefaultMemoryAlloc(unsigned long dwSize) {
		return Kernel32VirtualAlloc(NULL, dwSize, MEM_RESERVE | MEM_COMMIT, PAGE_EXECUTE_READWRITE);
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

	void DefaultSyscallControlFunc(void *context) { }

	void DefaultNtAllocateVirtualMemory() {
		__asm int 3;
	}

	void DefaultNtFreeVirtualMemory() {
		__asm int 3;
	}

	void DefaultNtQueryInformationThread() {
		__asm int 3;
	}

	void DefaultRtlNtStatusToDosError() {
		__asm int 3;
	}

	void Defaultvsnprintf_s() {
		__asm int 3;
	}

	/* Execution context callbacks ********************************************************/
	void GetCurrentRegisters(void *ctx, ExecutionRegs *regs) {
		struct _exec_env *pCtx = (struct _exec_env *)ctx;
		memcpy(regs, (struct _exec_env *)pCtx->runtimeContext.registers, sizeof(*regs));
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
		DefaultExecutionBeginFunc,
		DefaultExecutionControlFunc,
		DefaultExecutionEndFunc,
		DefaultSyscallControlFunc,

		DefaultIpcInitialize,

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

	struct _exec_env *pEnv = NULL;

	void TerminateCurrentProcess() {
		Kernel32TerminateProcess((HANDLE)0xFFFFFFFF, 0);
	}

	void TracerInitialization() {
		Initialize();

		RiverBasicBlock *pBlock = pEnv->blockCache.NewBlock((UINT_PTR)revtracerConfig.entryPoint);
		pBlock->address = (DWORD)revtracerConfig.entryPoint;
		pEnv->codeGen.Translate(pBlock, 0);
		pEnv->bForward = 1;
		pBlock->MarkForward();
		
		switch (revtracerAPI.executionBegin(pEnv->userContext, revtracerConfig.entryPoint, pEnv)) {
		case EXECUTION_ADVANCE :
			revtracerConfig.entryPoint = pBlock->pFwCode;
			break;
		case EXECUTION_TERMINATE :
			revtracerConfig.entryPoint = TerminateCurrentProcess;
			break;
		case EXECUTION_BACKTRACK :
			revtracerAPI.dbgPrintFunc("EXECUTION_BACKTRACK @executionBegin");
			revtracerConfig.entryPoint = TerminateCurrentProcess;
			break;
		}
	}

	__declspec(naked) void RevtracerPerform() {
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

	void SetControlMgmt(ExecutionControlFunc execCtl, SyscallControlFunc syscallCtl) {
		revtracerAPI.executionControl = execCtl;
		revtracerAPI.syscallControl = syscallCtl;
	}

	void SetContextSize(DWORD sz) {
		revtracerConfig.contextSize = sz;
	}

	void SetEntryPoint(ADDR_TYPE ep) {
		revtracerConfig.entryPoint = ep;
	}

	void CreateHook(ADDR_TYPE orig, ADDR_TYPE det) {
		RiverBasicBlock *pBlock = pEnv->blockCache.NewBlock((UINT_PTR)orig);
		pBlock->address = (DWORD)det;
		pEnv->codeGen.Translate(pBlock, 0);
		pBlock->address = (DWORD)orig;
	}

	void Initialize() {
		revtracerAPI.ipcLibInitialize();

		pEnv = new _exec_env(0x1000000, 0x10000, 0x2000000, 16, 0x10000);
		pEnv->userContext = AllocUserContext(pEnv, revtracerConfig.contextSize);

		for (DWORD i = 0; i < revtracerConfig.hookCount; ++i) {
			CreateHook(revtracerConfig.hooks[i].originalAddr, revtracerConfig.hooks[i].detourAddr);
		}
	}

	void Execute(int argc, char *argv[]) {
		DWORD ret = call_cdecl_2(pEnv, (_fn_cdecl_2)revtracerConfig.entryPoint, (void *)argc, (void *)argv);
		revtracerAPI.dbgPrintFunc("Done. ret = %d\n\n", ret);
	}

};