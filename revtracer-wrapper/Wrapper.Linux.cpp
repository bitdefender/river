#ifdef __linux__

#include <dlfcn.h>
#include <stdarg.h>

#include "Wrapper.Global.h"

#include "RevtracerWrapper.h"
#include "../BinLoader/LoaderAPI.h"
#include "../CommonCrossPlatform/Common.h"

#include <semaphore.h>
#include <time.h>
#include <asm/ldt.h>

typedef void* lib_t;

DLL_LOCAL MODULE_PTR lpthreadModule;
DLL_LOCAL lib_t lcModule, lrtModule;

typedef int (*PrintfHandler)(const char *format, ...);
PrintfHandler _print;

// ------------------- Memory allocation ----------------------
typedef void* (*AllocateMemoryHandler)(
	void *addr,
	size_t length, 
	int prot, 
	int flags, 
	int fd, 
	off_t offset
);

AllocateMemoryHandler _virtualAlloc;

void *LinAllocateVirtual(unsigned long size) {
	return _virtualAlloc(NULL, size, 0x7, 0x21, 0, 0); // RWX shared anonymous
}

// ------------------- Memory deallocation --------------------
typedef int (*FreeMemoryHandler)(
	void *addr, 
	size_t length
);

FreeMemoryHandler _virtualFree;

void LinFreeVirtual(void *address) {
	// TODO: implement some size retrieval mechanism
}

// ------------------- Memory mapping ------------------------
typedef AllocateMemoryHandler MapMemoryHandler;

void *LinMapMemory(unsigned long mapHandler, unsigned long access, unsigned long offset, unsigned long size, void *address) {
	void *addr =  _virtualAlloc(address, size, PROT_READ | PROT_WRITE | PROT_EXEC, MAP_SHARED, (int)mapHandler, offset);
	_print("[RevtracerWrapper] LinMapMemory: received %p input shm %d size %lu address %p at offset %08lx\n", addr, (int)mapHandler, size, address, offset);
	return addr;
}

// ------------------- Process termination --------------------
typedef void(*TerminateProcessHandler)(
	int status
);

TerminateProcessHandler _terminateProcess;

void LinTerminateProcess(int retCode) {
	_terminateProcess(retCode);
}

void *LinGetTerminationCodeFunc() {
	return (void *)_terminateProcess;
}

// ------------------- Write file -----------------------------
typedef ssize_t(*WriteFileHandler)(
	int fd, 
	const void *buf, 
	size_t count
);

WriteFileHandler _writeFile;

bool WinWriteFile(void *handle, void *buffer, size_t size, unsigned long *written) {
	*written = _writeFile((int)handle, buffer, size);
	return (0 <= written);
}

// ------------------- Error codes ----------------------------

long LinToErrno(long ntStatus) {
	return ntStatus;
}

// ------------------- Formatted print ------------------------
typedef int(*FormatPrintHandler)(
	char *buffer,
	size_t count,
	const char *format,
	va_list argptr
	);

FormatPrintHandler _formatPrint;

int LinFormatPrint(char *buffer, size_t sizeOfBuffer, const char *format, char *argptr) {
	return _formatPrint(buffer, sizeOfBuffer - 1, format, (va_list)argptr);
}

// ------------------- Yield execution ------------------------
typedef int(*YieldExecutionHandler) (void);

YieldExecutionHandler _yieldExecution;

long LinYieldExecution(void) {
	return (long) _yieldExecution();
}

// ------------------- Flush instr cache ----------------------
void LinFlushInstructionCache(void) {
}

// ------------------- Wait semaphore -------------------------
typedef int(*WaitSemaphoreHandler) (sem_t *);
typedef int(*TimedWaitSemaphoreHandler) (sem_t *, const struct timespec *);
typedef int(*ClockGetTimeHandler) (clockid_t clk_id, struct timespec *tp);

WaitSemaphoreHandler _waitSemaphore;
TimedWaitSemaphoreHandler _timedWaitSemaphore;
ClockGetTimeHandler _clockGetTime;

static long sec_to_nsec = 1000000000;

#define __NR_get_thread_area 244
void DebugSegmentDescriptors() {
	struct user_desc* table_entry_ptr = NULL;

	table_entry_ptr = (struct user_desc*)malloc(sizeof(struct user_desc));

	for (int i = 0; i < 0x100; i) {
		table_entry_ptr->entry_number = i;
		int ret = syscall( __NR_get_thread_area,
				table_entry_ptr);
		if (ret == -1 && errno == EINVAL) {
		} else if (ret == 0) {
			_print("[RevtracerWrapper] Segment %d base %08x\n", i, table_entry_ptr->base_addr);
		} else {
			printf("[Child] Error found when get_thread_area. errno %d\n", errno);
		}
	}
	free(table_entry_ptr);
}

int LinWaitSemaphore(void *semaphore, bool blocking) {
	return _waitSemaphore((sem_t *)semaphore);
}

// Not used!
int LinWaitSemaphoreNonBlocking(void *semaphore) {
	int ret;
	int timeout = 3;
	struct timespec abs_timeout;

	if (_clockGetTime(CLOCK_REALTIME, &abs_timeout) == -1) {
		_print("[RevtracerWrapper] Cannot get current time\n");
		return -1;
	}

	abs_timeout.tv_nsec = timeout * sec_to_nsec;
	abs_timeout.tv_sec = timeout;
	abs_timeout.tv_nsec %= 1000000000;

	unsigned int segments[0x100];
	ret = _timedWaitSemaphore((sem_t *)semaphore, &abs_timeout);

	return ret;
}


namespace revwrapper {
	extern "C" int InitRevtracerWrapper(unsigned long _lcBase, unsigned long lpthreadBase) {
		lcModule = dlopen("libc.so", RTLD_LAZY);
		lrtModule = dlopen("librt.so", RTLD_LAZY);
		CreateModule("libpthread.so", lpthreadModule);
		//TODO find base addresses

		if (!lcModule || !lpthreadModule) {
			DEBUG_BREAK;
			return -1;
		}

		// get functionality from ntdll
		_virtualAlloc = (AllocateMemoryHandler)LOAD_PROC(lcModule, "mmap");
		_virtualFree = (FreeMemoryHandler)LOAD_PROC(lcModule, "munmap");

		_terminateProcess = (TerminateProcessHandler)LOAD_PROC(lcModule, "exit");

		_writeFile = (WriteFileHandler)LOAD_PROC(lcModule, "write");

		_formatPrint = (FormatPrintHandler)LOAD_PROC(lcModule, "vsnprintf");
		_print = (PrintfHandler)LOAD_PROC(lcModule, "printf");

		LoadExportedName(lpthreadModule, lpthreadBase, "pthread_yield", _yieldExecution);

		// set global functionality
		allocateVirtual = LinAllocateVirtual;
		freeVirtual = LinFreeVirtual;
		mapMemory = LinMapMemory;

		terminateProcess = LinTerminateProcess;
		getTerminationCode = LinGetTerminationCodeFunc;

		toErrno = LinToErrno;
		formatPrint = LinFormatPrint;
		_print("[RevtracerWrapper] InitRevtracerWrapper success\n");

		yieldExecution = LinYieldExecution;
		LoadExportedName(lpthreadModule, lpthreadBase, "sem_init", initSemaphore);
		_print("[RevtracerWrapper] Found InitSemaphoreFunc @address %08lx\n", (void*)initSemaphore);
		LoadExportedName(lpthreadModule, lpthreadBase, "sem_wait", _waitSemaphore);
		LoadExportedName(lpthreadModule, lpthreadBase, "sem_timedwait", _timedWaitSemaphore);
		LoadExportedName(lpthreadModule, lpthreadBase, "sem_post", postSemaphore);
		LoadExportedName(lpthreadModule, lpthreadBase, "sem_destroy", destroySemaphore);
		LoadExportedName(lpthreadModule, lpthreadBase, "sem_getvalue", getvalueSemaphore);
		_clockGetTime = (ClockGetTimeHandler)LOAD_PROC(lcModule, "clock_gettime");

		waitSemaphore = LinWaitSemaphore;
		openSharedMemory = (OpenSharedMemoryFunc)LOAD_PROC(lrtModule, "shm_open");
		unlinkSharedMemory = (UnlinkSharedMemoryFunc)LOAD_PROC(lrtModule, "shm_unlink");

		flushInstructionCache = LinFlushInstructionCache;
		return 0;
	}
};

#endif
