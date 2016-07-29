#include "ipclib.h"

#include "../revtracer/DebugPrintFlags.h"

namespace ipc {
	typedef long NTSTATUS;
	typedef NTSTATUS(*NtYieldExecutionFunc)();

	typedef char *va_list;
#define _ADDRESSOF(v)   ( &reinterpret_cast<const char &>(v) )
#define _INTSIZEOF(n)   ( (sizeof(n) + sizeof(int) - 1) & ~(sizeof(int) - 1) )

#define va_start(ap,v)  ( ap = (va_list)_ADDRESSOF(v) + _INTSIZEOF(v) )
#define va_arg(ap,t)    ( *(t *)((ap += _INTSIZEOF(t)) - _INTSIZEOF(t)) )
#define va_end(ap)      ( ap = (va_list)0 )


	RingBuffer<(1 << 20)> debugLog;
	ShmTokenRing ipcToken;

	DLL_LINKAGE IpcAPI ipcAPI = {
		NULL
	};

	DLL_LINKAGE IpcData ipcData;

	typedef int (*_vsnprintf_sFunc)(
		char *buffer,
		size_t sizeOfBuffer,
		size_t count,
		const char *format,
		va_list argptr
	);

	int GeneratePrefix(char *buff, int size, ...) {
		va_list va;

		va_start(va, size);
		int sz = ((_vsnprintf_sFunc)ipcAPI.vsnprintf_s)(
			buff, 
			size, 
			size - 1, 
			"[%3s|%5s|%3s|%c] ",
			va
		);
		va_end(va);

		return sz;
	}

	DLL_LINKAGE void DebugPrint(DWORD printMask, const char *fmt, ...) {
		va_list va;
		char tmpBuff[512];

		char pfxBuff[] = "[___|_____|___|_]      ";
		static char lastChar = '\n';

		const char messageTypes[][4] = {
			"___",
			"ERR",
			"INF",
			"DBG"
		};

		const char executionStages[][6] = {
			"_____",
			"BRHND",
			"DIASM",
			"TRANS",
			"REASM",
			"RUNTM",
			"INSPT",
			"CNTNR"
		};

		const char codeTypes[][4] = {
			"___",
			"NAT",
			"RIV",
			"TRK",
			"SYM"
		};

		const char codeDirections[] = {
			'_', 'F', 'B'
		};

		if ('\n' == lastChar) {
			int sz = GeneratePrefix(
				pfxBuff,
				sizeof(pfxBuff),
				messageTypes[(printMask & PRINT_MESSAGE_MASK) >> PRINT_MESSAGE_SHIFT],
				executionStages[(printMask & PRINT_EXECUTION_MASK) >> PRINT_EXECUTION_SHIFT],
				codeTypes[(printMask & PRINT_CODE_TYPE_MASK) >> PRINT_CODE_TYPE_SHIFT],
				codeDirections[(printMask & PRINT_CODE_DIRECTION_MASK) >> PRINT_CODE_DIRECTION_SHIFT]
			);
			debugLog.Write(pfxBuff, sz);
		}

		va_start(va, fmt);
		int sz = ((_vsnprintf_sFunc)ipcAPI.vsnprintf_s)(tmpBuff, sizeof(tmpBuff)-1, sizeof(tmpBuff)-1, fmt, va);
		va_end(va);

		if (sz) {
			debugLog.Write(tmpBuff, sz);
			lastChar = tmpBuff[sz - 1];
		}
	}


#define SECTION_MAP_WRITE            0x0002
#define SECTION_MAP_READ             0x0004
#define SECTION_MAP_EXECUTE          0x0020

#define FILE_MAP_WRITE      SECTION_MAP_WRITE
#define FILE_MAP_READ       SECTION_MAP_READ
#define FILE_MAP_EXECUTE	SECTION_MAP_EXECUTE

	typedef void *(*LdrMapMemory)(DWORD desiredAccess, DWORD offset, size_t size, void *address);
	DLL_LINKAGE void *MemoryAllocFunc(DWORD dwSize) {
		ipcData.type = REQUEST_MEMORY_ALLOC;
		ipcData.data.asMemoryAllocRequest = dwSize;
		ipcToken.Release(INPROC_TOKEN_USER);
		// remote execution here

		ipcToken.Wait(INPROC_TOKEN_USER);
		if (ipcData.type != REPLY_MEMORY_ALLOC) {
			__asm int 3;
		}

		void *ptr = ((LdrMapMemory)ipcAPI.ldrMapMemory)(
			FILE_MAP_READ | FILE_MAP_WRITE | FILE_MAP_EXECUTE,
			ipcData.data.asMemoryAllocReply.offset,
			dwSize,
			ipcData.data.asMemoryAllocReply.pointer
		);

		if (ptr != ipcData.data.asMemoryAllocReply.pointer) {
			__asm int 3;
			return NULL;
		}

		return ptr;
	}

	DLL_LINKAGE void MemoryFreeFunc(void *ptr) {
		ipcData.type = REQUEST_MEMORY_FREE;
		ipcData.data.asMemoryFreeRequest = ptr;
		ipcToken.Release(INPROC_TOKEN_USER);
		// remote execution here

		ipcToken.Wait(INPROC_TOKEN_USER);
		if (ipcData.type != REPLY_MEMORY_FREE) {
			__asm int 3;
		}
	}

	DLL_LINKAGE QWORD TakeSnapshot() {
		ipcData.type = REQUEST_TAKE_SNAPSHOT;
		ipcToken.Release(INPROC_TOKEN_USER);
		// remote execution here
		
		ipcToken.Wait(INPROC_TOKEN_USER);
		if (ipcData.type != REPLY_TAKE_SNAPSHOT) {
			__asm int 3;
		}

		return ipcData.data.asTakeSnapshotReply;
	}

	DLL_LINKAGE QWORD RestoreSnapshot() {
		ipcData.type = REQUEST_RESTORE_SNAPSHOT;
		ipcToken.Release(INPROC_TOKEN_USER);
		// remote execution here

		ipcToken.Wait(INPROC_TOKEN_USER);
		if (ipcData.type != REPLY_RESTORE_SNAPSHOT) {
			__asm int 3;
		}

		return ipcData.data.asRestoreSnapshotReply;
	}

	DLL_LINKAGE void InitializeContextFunc(void *context) {
		ipcData.type = REQUEST_INITIALIZE_CONTEXT;
		ipcData.data.asInitializeContextRequest = context;
		ipcToken.Release(INPROC_TOKEN_USER);
		// remote execution here

		ipcToken.Wait(INPROC_TOKEN_USER);
		if (ipcData.type != REPLY_INITIALIZE_CONTEXT) {
			__asm int 3;
		}
	}

	DLL_LINKAGE void CleanupContextFunc(void *context) {
		ipcData.type = REQUEST_CLEANUP_CONTEXT;
		ipcData.data.asCleanupContextRequest = context;
		ipcToken.Release(INPROC_TOKEN_USER);
		// remote execution here

		ipcToken.Wait(INPROC_TOKEN_USER);
		if (ipcData.type != REPLY_CLEANUP_CONTEXT) {
			__asm int 3;
		}
	}

	/*DLL_LINKAGE DWORD ExecutionBeginFunc(void *context, ADDR_TYPE nextInstruction, void *cbCtx) {
		ipcData.type = REQUEST_EXECUTION_BEGIN;
		ipcData.data.asExecutionBeginRequest.context = context;
		ipcData.data.asExecutionBeginRequest.nextInstruction = nextInstruction;
		ipcData.data.asExecutionBeginRequest.cbCtx = cbCtx;
		ipcToken.Release(INPROC_TOKEN_USER);
		// remote execution here

		ipcToken.Wait(INPROC_TOKEN_USER);
		if (ipcData.type != REPLY_EXECUTION_BEGIN) {
			__asm int 3;
		}

		return ipcData.data.asExecutionBeginReply;
	}

	DLL_LINKAGE DWORD ExecutionControlFunc(void *context, ADDR_TYPE nextInstruction, void *cbCtx) {
		ipcData.type = REQUEST_EXECUTION_CONTORL;
		ipcData.data.asExecutionControlRequest.context = context;
		ipcData.data.asExecutionControlRequest.nextInstruction = nextInstruction;
		ipcData.data.asExecutionControlRequest.cbCtx = cbCtx;
		ipcToken.Release(INPROC_TOKEN_USER);
		// remote execution here

		ipcToken.Wait(INPROC_TOKEN_USER);
		if (ipcData.type != REPLY_EXECUTION_CONTORL) {
			__asm int 3;
		}

		return ipcData.data.asExecutionControlReply;
	}

	DLL_LINKAGE DWORD ExecutionEndFunc(void *context, void *cbCtx) {
		ipcData.type = REQUEST_EXECUTION_END;
		ipcData.data.asExecutionEndRequest.context = context;
		ipcData.data.asExecutionEndRequest.cbCtx = cbCtx;
		ipcToken.Release(INPROC_TOKEN_USER);
		// remote execution here

		ipcToken.Wait(INPROC_TOKEN_USER);
		if (ipcData.type != REPLY_EXECUTION_END) {
			__asm int 3;
		}

		return ipcData.data.asExecutionEndReply;
	}*/

	DLL_LINKAGE DWORD BranchHandlerFunc(void *context, void *userContext, ADDR_TYPE nextInstruction) {
		ipcData.type = REQUEST_BRANCH_HANDLER;
		ipcData.data.asBranchHandlerRequest.executionEnv = context;
		ipcData.data.asBranchHandlerRequest.userContext = userContext;
		ipcData.data.asBranchHandlerRequest.nextInstruction = nextInstruction;
		ipcToken.Release(INPROC_TOKEN_USER);

		ipcToken.Wait(INPROC_TOKEN_USER);
		if (ipcData.type != REPLY_BRANCH_HANDLER) {
			__asm int 3;
		}

		return ipcData.data.asBranchHandlerReply;
	}

	DLL_LINKAGE void SyscallControlFunc(void *context, void *userContext) {
		ipcData.type = REQUEST_SYSCALL_CONTROL;
		ipcData.data.asSyscallControlRequest.context = context;
		ipcData.data.asSyscallControlRequest.userContext = userContext;
		ipcToken.Release(INPROC_TOKEN_USER);
		// remote execution here

		ipcToken.Wait(INPROC_TOKEN_USER);
		if (ipcData.type != REPLY_SYSCALL_CONTROL) {
			__asm int 3;
		}
	}

	DLL_LINKAGE void Initialize() {
		debugLog.Init();
		ipcToken.Init(2);
	}

	void NtDllNtYieldExecution() {
		((NtYieldExecutionFunc)ipcAPI.ntYieldExecution)();
	}


#define FALSE 0
#define TRUE 1

#define PF_FLOATING_POINT_PRECISION_ERRATA   0   
#define PF_FLOATING_POINT_EMULATED           1   
#define PF_COMPARE_EXCHANGE_DOUBLE           2   
#define PF_MMX_INSTRUCTIONS_AVAILABLE        3   
#define PF_PPC_MOVEMEM_64BIT_OK              4   
#define PF_ALPHA_BYTE_INSTRUCTIONS           5   
#define PF_XMMI_INSTRUCTIONS_AVAILABLE       6   
#define PF_3DNOW_INSTRUCTIONS_AVAILABLE      7   
#define PF_RDTSC_INSTRUCTION_AVAILABLE       8   
#define PF_PAE_ENABLED                       9   
#define PF_XMMI64_INSTRUCTIONS_AVAILABLE    10   
#define PF_SSE_DAZ_MODE_AVAILABLE           11   
#define PF_NX_ENABLED                       12   
#define PF_SSE3_INSTRUCTIONS_AVAILABLE      13   
#define PF_COMPARE_EXCHANGE128              14   
#define PF_COMPARE64_EXCHANGE128            15   
#define PF_CHANNELS_ENABLED                 16   
#define PF_XSAVE_ENABLED                    17   
#define PF_ARM_VFP_32_REGISTERS_AVAILABLE   18   
#define PF_ARM_NEON_INSTRUCTIONS_AVAILABLE  19   
#define PF_SECOND_LEVEL_ADDRESS_TRANSLATION 20   
#define PF_VIRT_FIRMWARE_ENABLED            21   
#define PF_RDWRFSGSBASE_AVAILABLE           22   
#define PF_FASTFAIL_AVAILABLE               23   
#define PF_ARM_DIVIDE_INSTRUCTION_AVAILABLE 24   
#define PF_ARM_64BIT_LOADSTORE_ATOMIC       25   
#define PF_ARM_EXTERNAL_CACHE_AVAILABLE     26   
#define PF_ARM_FMAC_INSTRUCTIONS_AVAILABLE  27   
#define PF_RDRAND_INSTRUCTION_AVAILABLE     28   

	DLL_LINKAGE BOOL IsProcessorFeaturePresent(DWORD ProcessorFeature) {
		BOOL result;

		if (ProcessorFeature >= 0x40) {
			result = FALSE;
		}
		else {
			switch (ProcessorFeature) {
			case PF_MMX_INSTRUCTIONS_AVAILABLE:
			case PF_SSE3_INSTRUCTIONS_AVAILABLE:
			case PF_XMMI64_INSTRUCTIONS_AVAILABLE:
			case PF_XMMI_INSTRUCTIONS_AVAILABLE:
			case PF_COMPARE_EXCHANGE_DOUBLE:
			case PF_COMPARE_EXCHANGE128:
			case PF_COMPARE64_EXCHANGE128:
				return FALSE;
			default:
				result = ((BYTE *)0x7FFE0274)[ProcessorFeature];
			}
		}
		return result;
	}
};

unsigned int Entry() {
	return 1;
}