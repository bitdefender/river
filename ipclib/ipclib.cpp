#include "ipclib.h"

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

	DLL_LINKAGE void DebugPrint(const char *fmt, ...) {
		va_list va;
		char tmpBuff[512];

		va_start(va, fmt);
		int sz = ((_vsnprintf_sFunc)ipcAPI.vsnprintf_s)(tmpBuff, sizeof(tmpBuff)-1, sizeof(tmpBuff)-1, fmt, va);
		va_end(va);

		debugLog.Write(tmpBuff, sz);
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

	DLL_LINKAGE void SyscallControlFunc(void *context) {
		ipcData.type = REQUEST_SYSCALL_CONTROL;
		ipcData.data.asSyscallControlRequest.context = context;
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
};

unsigned int Entry() {
	return 1;
}