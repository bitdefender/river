#include "RevtracerWrapper.h"
#include "../CommonCrossPlatform/Common.h"

#include "Wrapper.Global.h"

AllocateVirtualFunc allocateVirtual;
FreeVirtualFunc freeVirtual;
MapMemoryFunc mapMemory;

TerminateProcessFunc terminateProcess;
GetTerminationCodeFunc getTerminationCode;

FormatPrintFunc formatPrint;

WriteFileFunc writeFile;
ToErrnoFunc toErrno;

YieldExecutionFunc yieldExecution;
InitSemaphoreFunc initSemaphore;
WaitSemaphoreFunc waitSemaphore;
PostSemaphoreFunc postSemaphore;
DestroySemaphoreFunc destroySemaphore;
GetValueSemaphoreFunc getvalueSemaphore;

OpenSharedMemoryFunc openSharedMemory;
UnlinkSharedMemoryFunc unlinkSharedMemory;

FlushInstructionCacheFunc flushInstructionCache;

namespace revwrapper {
	DLL_WRAPPER_PUBLIC ImportedApi wrapperImports;

	DLL_WRAPPER_PUBLIC extern void *CallAllocateMemoryHandler(unsigned long dwSize)
	{
		return allocateVirtual(dwSize);
	}

	DLL_REVTRACER_WRAPPER_PUBLIC extern void CallFreeMemoryHandler(void *address) {
		return freeVirtual(address);
	}

	DLL_REVTRACER_WRAPPER_PUBLIC extern void *CallMapMemoryHandler(unsigned long mapHandler, unsigned long access, unsigned long offset, unsigned long size, void *address) {
		return mapMemory(mapHandler, access, offset, size, address);
	}

	DLL_REVTRACER_WRAPPER_PUBLIC extern void CallTerminateProcessHandler(void) {
		terminateProcess(0);
	}

	DLL_REVTRACER_WRAPPER_PUBLIC extern void *CallGetTerminationCodeHandler(void) {
		return getTerminationCode();
	}

	DLL_REVTRACER_WRAPPER_PUBLIC extern int CallFormattedPrintHandler(char * buffer, unsigned int sizeOfBuffer, const char * format, char *argptr) {
		return formatPrint(buffer, sizeOfBuffer, format, (char *)argptr);
	}

	DLL_REVTRACER_WRAPPER_PUBLIC extern bool CallWriteFile(void *handle, void *buffer, unsigned int size, unsigned long *written) {
		return writeFile(handle, buffer, size, written);
	}

	DLL_REVTRACER_WRAPPER_PUBLIC extern long CallConvertToSystemErrorHandler(long status) {
		return toErrno(status);
	}

	// Used until we replace the ipclib spinlock with signals / events
	DLL_WRAPPER_PUBLIC extern long CallYieldExecution(void) {
		return yieldExecution();
	}

	DLL_WRAPPER_PUBLIC extern int CallInitSemaphore(void *semaphore, int shared, int value) {
		return initSemaphore(semaphore, shared, value);
	}

	DLL_WRAPPER_PUBLIC extern int CallWaitSemaphore(void *semaphore, bool blocking) {
		return waitSemaphore(semaphore, blocking);
	}

	DLL_WRAPPER_PUBLIC extern int CallPostSemaphore(void *semaphore) {
		return postSemaphore(semaphore);
	}

	DLL_WRAPPER_PUBLIC extern int CallDestroySemaphore(void *semaphore) {
		return destroySemaphore(semaphore);
	}

	DLL_WRAPPER_PUBLIC extern int CallGetValueSemaphore(void *semaphore, int *value) {
		return getvalueSemaphore(semaphore, value);
	}

	DLL_WRAPPER_PUBLIC extern int CallOpenSharedMemory(const char *name, int oflag, int mode) {
		return openSharedMemory(name, oflag, mode);
	}

	DLL_WRAPPER_PUBLIC extern int CallUnlinkSharedMemory(const char *name) {
		return unlinkSharedMemory(name);
	}

	DLL_WRAPPER_PUBLIC extern void CallFlushInstructionCache(void) {
		flushInstructionCache();
	}

}; //namespace revwrapper
