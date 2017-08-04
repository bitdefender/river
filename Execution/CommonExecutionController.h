#ifndef _COMMON_EXECUTION_CONTROLLER_H
#define _COMMON_EXECUTION_CONTROLLER_H

#include "../CommonCrossPlatform/Common.h"
#include "Execution.h"
#include <vector>

using namespace std;

void DebugPrintf(const unsigned int printMask, const char *fmt, ...);
nodep::DWORD BranchHandlerFunc(void *context, void *userContext, rev::ADDR_TYPE nextInstruction);
nodep::DWORD ErrorHandlerFunc(void *context, void *userContext, rev::RevtracerError *rerror);
void InitSegments(void *hThread, nodep::DWORD *segments);

typedef void(*GetCurrentRegistersFunc)(void *ctx, rev::ExecutionRegs *regs);
typedef void *(*GetMemoryInfoFunc)(void *ctx, void *ptr);
typedef void (*SetSymbolicExecutorFunc)(rev::SymbolicExecutorConstructor);
typedef bool(*GetLastBasicBlockInfoFunc)(void *ctx, rev::BasicBlockInfo *);

typedef void (*MarkMemoryValueFunc)(void *ctx, rev::ADDR_TYPE addr, nodep::DWORD value);

class CommonExecutionController : public ExecutionController {
private :
	bool UpdateLayout();

	vector<VirtualMemorySection> sec;
	vector<ModuleInfo> mod;
	uint32_t virtualSize, commitedSize;

protected:

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

	CommonExecutionController();

	wstring path;
	wstring cmdLine;
	void *entryPoint;

	uint32_t featureFlags;

	void *context;
	bool updated;

	ExecutionObserver *observer;

	GetCurrentRegistersFunc gcr;
	GetMemoryInfoFunc gmi;
	MarkMemoryValueFunc mmv;
	GetLastBasicBlockInfoFunc glbbi;

	rev::TrackCallbackFunc trackCb;
	rev::MarkCallbackFunc markCb;
	rev::SymbolicHandlerFunc symbCb;

	static const rev::RevtracerVersion supportedVersion;
public :
	virtual int GetState() const;

	virtual bool SetPath(const wstring &p);
	virtual bool SetCmdLine(const wstring &c);
	virtual bool SetEntryPoint(void *ep);
	virtual bool SetExecutionFeatures(unsigned int feat);

	virtual void SetExecutionObserver(ExecutionObserver *obs);
	virtual void SetTrackingObserver(rev::TrackCallbackFunc track, rev::MarkCallbackFunc mark);
	virtual void SetSymbolicHandler(rev::SymbolicHandlerFunc symb);

	virtual unsigned int ExecutionBegin(void *address, void *cbCtx);
	virtual unsigned int ExecutionControl(void *address, void *cbCtx);
	virtual unsigned int ExecutionEnd(void *cbCtx);
	virtual unsigned int TranslationError(void *address, void *cbCtx);

	virtual void GetCurrentRegisters(void *ctx, rev::ExecutionRegs *registers);
	virtual void *GetMemoryInfo(void *ctx, void *ptr);
	virtual bool GetLastBasicBlockInfo(void *ctx, rev::BasicBlockInfo *bbInfo);

	virtual void DebugPrintf(const unsigned long printMask, const char *fmt, ...);

	virtual bool GetProcessVirtualMemory(VirtualMemorySection *&sections, int &sectionCount);
	virtual bool GetModules(ModuleInfo *&modules, int &moduleCount);
	virtual void MarkMemoryValue(void *ctx, rev::ADDR_TYPE addr, nodep::DWORD value);
};

#endif // !_COMMON_EXECUTION_CONTROLLER_H
