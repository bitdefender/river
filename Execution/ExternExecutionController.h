#ifndef _EXTERN_EXECUTION_CONTROLLER_H
#define _EXTERN_EXECUTION_CONTROLLER_H

#include "CommonExecutionController.h"

#include "DualAllocator.h"
#include "../BinLoader/PE.Loader.h"

#include "../loader/loader.h"
#include "../ipclib/ipclib.h"
#include "../revtracer/revtracer.h"

#include "../CommonCrossPlatform/Common.h"

#ifdef _WIN32
#include <Windows.h>
#endif

class ExternExecutionController : public CommonExecutionController {
private:
	HANDLE hProcess, hMainThread, hControlThread;
	FILE_T hDbg;
	DWORD pid, mainTid;

	DualAllocator *shmAlloc;

	BYTE *pLoaderBase;
	BYTE *pLoaderConfig;
	BYTE *pLoaderPerform;

	BYTE *pIpcBase;
	BYTE *pRevtracerBase;

	BYTE *pLdrMapMemory;

	ldr::LoaderConfig loaderConfig;

	ipc::RingBuffer<(1 << 20)> *debugLog;
	ipc::ShmTokenRing *ipcToken;
	ipc::IpcData *ipcData;
	BYTE *pIPFPFunc;
	
	rev::RevtracerAPI tmpRevApi;
	rev::RevtracerConfig *revCfg;
	BYTE *revtracerPerform;

	char debugBuffer[4096];

	bool InitializeAllocator();
	bool MapLoader();
	bool MapTracer();
	bool WriteLoaderConfig();

	bool InitializeIpcLib(ldr::FloatingPE *fIpcLib);
	bool InitializeRevtracer(ldr::FloatingPE *fRevTracer);

	bool SwitchEntryPoint();
	bool PatchProcess();
public:
	ExternExecutionController();

	virtual bool SetEntryPoint();

	virtual bool Execute();
	virtual bool WaitForTermination();

	DWORD ControlThread();

	virtual THREAD_T GetProcessHandle();

	virtual bool ReadProcessMemory(unsigned int base, unsigned int size, unsigned char *buff);
	virtual bool WriteProcessMemory(unsigned int base, unsigned int size, unsigned char *buff);

	virtual unsigned int ExecutionBegin(void *address, void *cbCtx);
};


#endif
