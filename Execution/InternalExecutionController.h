#ifndef _INTERNAL_EXECUTION_CONTROLLER_H
#define _INTERNAL_EXECUTION_CONTROLLER_H

#include "Execution.h"

#include "DualAllocator.h"
#include "Loader/PE.ldr.h"

#include "../loader/loader.h"
#include "../ipclib/ipclib.h"
#include "../revtracer/revtracer.h"

#include <vector>
#include <Windows.h>

using namespace std;

class InternalExecutionController : public ExecutionController {
private:
	wstring path;
	wstring cmdLine;

	HANDLE hProcess, hMainThread;
	DWORD pid, mainTid;

	DualAllocator *shmAlloc;

	vector<VirtualMemorySection> sec;

	enum {
		NEW = EXECUTION_NEW,
		INITIALIZED = EXECUTION_INITIALIZED,
		RUNNING = EXECUTION_RUNNING,
		TERMINATED = EXECUTION_TERMINATED,
		ERR = EXECUTION_ERR
	} execState;

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
	
	rev::RevtracerAPI tmpRevApi;
	rev::RevtracerConfig *revCfg;
	BYTE *revtracerPerform;

	bool InitializeAllocator();
	bool MapLoader();
	bool MapTracer();
	bool WriteLoaderConfig();

	bool InitializeIpcLib(FloatingPE *fIpcLib);
	bool InitializeRevtracer(FloatingPE *fRevTracer);

	bool SwitchEntryPoint();
public:
	InternalExecutionController();

	virtual int GetState() const;

	virtual bool SetPath(const wstring &p);
	virtual bool SetCmdLine(const wstring &c);

	virtual bool Execute();
	virtual bool Terminate();

	virtual bool GetProcessVirtualMemory(VirtualMemorySection *&sections, int &sectionCount);
};


#endif
