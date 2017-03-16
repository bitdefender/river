#ifndef _EXTERN_EXECUTION_CONTROLLER_H
#define _EXTERN_EXECUTION_CONTROLLER_H

#include "CommonExecutionController.h"

#include "../BinLoader/Abstract.Loader.h"
#include "../BinLoader/LoaderAPI.h"
#include "../BinLoader/PE.Loader.h"
#include "DualAllocator.h"

#include "../loader/loader.h"
#include "../revtracer-wrapper/RevtracerWrapper.h"
#include "../ipclib/ipclib.h"
#include "../revtracer/revtracer.h"


#include "../CommonCrossPlatform/Common.h"
#include "../CommonCrossPlatform/LibraryLayout.h"

#ifdef _WIN32
#include <Windows.h>
#include "../ipclib/ShmTokenRingWin.h"
#elif defined(__linux__)
#endif

class ExternExecutionController : public CommonExecutionController {
private:
	HANDLE hProcess /* child process */, hMainThread;
	THREAD_T hControlThread;
	FILE_T hDbg;
	DWORD pid, mainTid;

	DualAllocator* shmAlloc;

	ldr::AbstractBinary *hLoaderModule;
	BASE_PTR hLoaderBase;
	struct {
		ldr::LoaderConfig localConfig;
		ldr::LoaderConfig *pConfig;
		ldr::LoaderImports *pImports;
		ldr::LoaderExports *pExports;
	} loader;

	ldr::AbstractBinary *hRevWrapperModule;
	BASE_PTR hRevWrapperBase;
	struct {
		revwrapper::WrapperImports *pImports;
		revwrapper::WrapperExports *pExports;
	} wrapper;

	ldr::AbstractBinary *hIpcModule;
	BASE_PTR hIpcBase;

	ldr::AbstractBinary *hRevtracerModule;
	BASE_PTR hRevtracerBase;

	ext::LibraryLayout *libraryLayout;



	ipc::RingBuffer<(1 << 20)> *debugLog;
#ifdef __linux__
	ipc::ShmTokenRingLin *ipcToken;
#else
	ipc::ShmTokenRingWin *ipcToken;
#endif
	ipc::IpcData *ipcData;
	BYTE *pIPFPFunc;

	rev::RevtracerAPI tmpRevApi;
	rev::RevtracerConfig *revCfg;
	BYTE *revtracerPerform;

	char debugBuffer[4096];

	FILE_T hBlocksFd;

	bool InitializeAllocator();
	bool InitLibraryLayout();
	bool MapLoader();
	bool MapWrapper();
	bool MapTracer();
	bool WriteLoaderConfig();

	bool InitializeWrapper();
	bool InitializeIpcLib();
	bool InitializeRevtracer();

	bool SwitchEntryPoint();
	bool PatchProcess();

	void ConvertWideStringPath(char *result, size_t len);
	void MapSharedLibraries(unsigned long baseAddress);
public:
	ExternExecutionController();

	virtual bool SetEntryPoint();

	virtual bool Execute();
	virtual bool WaitForTermination();

	DWORD ControlThread();

	virtual THREAD_T GetProcessHandle();

	virtual bool ReadProcessMemory(unsigned int base, unsigned int size, unsigned char *buff);

	virtual unsigned int ExecutionBegin(void *address, void *cbCtx);
};


#endif
