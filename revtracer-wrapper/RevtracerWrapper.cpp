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
InitEventFunc initEvent;
WaitEventFunc waitEvent;
PostEventFunc postEvent;
DestroyEventFunc destroyEvent;
GetValueEventFunc getvalueEvent;

OpenSharedMemoryFunc openSharedMemory;
UnlinkSharedMemoryFunc unlinkSharedMemory;

FlushInstructionCacheFunc flushInstructionCache;

	void *CallAllocateMemoryHandler(unsigned long dwSize)
	{
		return allocateVirtual(dwSize);
	}

	void CallFreeMemoryHandler(void *address) {
		return freeVirtual(address);
	}

	void *CallMapMemoryHandler(unsigned long mapHandler, unsigned long access, unsigned long offset, unsigned long size, void *address) {
		return mapMemory(mapHandler, access, offset, size, address);
	}

	void CallTerminateProcessHandler(void) {
		terminateProcess(0);
	}

	void *CallGetTerminationCodeHandler(void) {
		return getTerminationCode();
	}

	int CallFormattedPrintHandler(char * buffer, unsigned int sizeOfBuffer, const char * format, char *argptr) {
		return formatPrint(buffer, sizeOfBuffer, format, (char *)argptr);
	}

	bool CallWriteFile(void *handle, void *buffer, unsigned int size, unsigned long *written) {
		return writeFile(handle, buffer, size, written);
	}

	long CallConvertToSystemErrorHandler(long status) {
		return toErrno(status);
	}

	// Used until we replace the ipclib spinlock with signals / events
	long CallYieldExecution(void) {
		return yieldExecution();
	}

	bool CallInitEvent(void *handle, bool isSet) {
		return initEvent(handle, isSet);
	}

	bool CallWaitEvent(void *handle, int timeout) {
		return waitEvent(handle, timeout);
	}

	bool CallPostEvent(void *handle) {
		return postEvent(handle);
	}

	void CallDestroyEvent(void *handle) {
		destroyEvent(handle);
	}

	void CallGetValueEvent(void *handle, int *ret) {
		*ret = getvalueEvent(handle);
	}

	int CallOpenSharedMemory(const char *name, int oflag, int mode) {
		return openSharedMemory(name, oflag, mode);
	}

	int CallUnlinkSharedMemory(const char *name) {
		return unlinkSharedMemory(name);
	}

	void CallFlushInstructionCache(void) {
		flushInstructionCache();
	}

namespace revwrapper {
	extern "C" bool InitRevtracerWrapper(void *configPage);

	DLL_WRAPPER_PUBLIC WrapperImports wrapperImports;
	
}; //namespace revwrapper
