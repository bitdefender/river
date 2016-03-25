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
	ExecutionBeginFunc eBegin;
	ExecutionControlFunc eControl;
	ExecutionEndFunc eEnd;
	TerminationNotifyFunc term;

	wstring path;
	wstring cmdLine;

	HANDLE hProcess, hMainThread, hControlThread;
	DWORD pid, mainTid;
	ULONG virtualSize, commitedSize;

	void *context;

	DualAllocator *shmAlloc;

	vector<VirtualMemorySection> sec;
	vector<ModuleInfo> mod;
	bool updated;

	enum {
		NEW = EXECUTION_NEW,
		INITIALIZED = EXECUTION_INITIALIZED,
		SUSPENDED_AT_START = EXECUTION_SUSPENDED_AT_START,
		RUNNING = EXECUTION_RUNNING,
		SUSPENDED = EXECUTION_SUSPENDED,
		SUSPENDED_AT_TERMINATION = EXECUTION_SUSPENDED_AT_TERMINATION,
		TERMINATED = EXECUTION_TERMINATED,
		ERR = EXECUTION_ERR
	} execState;

	BYTE *pLoaderBase;
	BYTE *pLoaderConfig;
	BYTE *pLoaderPerform;

	BYTE *pIpcBase;
	BYTE *pRevtracerBase;

	BYTE *pLdrMapMemory;

	DWORD featureFlags;

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

	bool InitializeIpcLib(FloatingPE *fIpcLib);
	bool InitializeRevtracer(FloatingPE *fRevTracer);

	bool SwitchEntryPoint();
	bool PatchProcess();

	bool UpdateLayout();
public:
	InternalExecutionController();

	virtual int GetState() const;

	virtual bool SetPath(const wstring &p);
	virtual bool SetCmdLine(const wstring &c);
	virtual bool SetExecutionFeatures(unsigned int feat);

	virtual bool Execute();
	virtual bool Terminate();

	DWORD ControlThread();

	virtual void *GetProcessHandle();

	virtual bool GetProcessVirtualMemory(VirtualMemorySection *&sections, int &sectionCount);
	virtual bool GetModules(ModuleInfo *&modules, int &moduleCount);
	virtual bool ReadProcessMemory(unsigned int base, unsigned int size, unsigned char *buff);

	virtual void SetNotificationContext(void *ctx);

	virtual void SetTerminationNotification(TerminationNotifyFunc func);
	virtual void SetExecutionBeginNotification(ExecutionBeginFunc func);
	virtual void SetExecutionControlNotification(ExecutionControlFunc func);
	virtual void SetExecutionEndNotification(ExecutionEndFunc func);

	virtual void GetCurrentRegisters(Registers &registers);
};


#endif
