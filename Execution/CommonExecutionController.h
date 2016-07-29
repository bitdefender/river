#ifndef _COMMON_EXECUTION_CONTROLLER_H
#define _COMMON_EXECUTION_CONTROLLER_H

#include "Execution.h"
#include "../revtracer/revtracer.h"

#include <vector>

using namespace std;

void DebugPrintf(const unsigned int printMask, const char *fmt, ...);
rev::DWORD BranchHandlerFunc(void *context, void *userContext, rev::ADDR_TYPE nextInstruction);

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

	::ExecutionBeginFunc eBegin;
	::ExecutionControlFunc eControl;
	::ExecutionEndFunc eEnd;
	TerminationNotifyFunc term;

	void *context;
	bool updated;
public :
	virtual int GetState() const;

	virtual bool SetPath(const wstring &p);
	virtual bool SetCmdLine(const wstring &c);
	virtual bool SetEntryPoint(void *ep);
	virtual bool SetExecutionFeatures(unsigned int feat);

	virtual void SetNotificationContext(void *ctx);

	virtual void SetTerminationNotification(TerminationNotifyFunc func);
	virtual void SetExecutionBeginNotification(::ExecutionBeginFunc func);
	virtual void SetExecutionControlNotification(::ExecutionControlFunc func);
	virtual void SetExecutionEndNotification(::ExecutionEndFunc func);

	virtual unsigned int ExecutionBegin(void *address, void *cbCtx);
	virtual unsigned int ExecutionControl(void *address, void *cbCtx);
	virtual unsigned int ExecutionEnd(void *cbCtx);

	virtual bool GetProcessVirtualMemory(VirtualMemorySection *&sections, int &sectionCount);
	virtual bool GetModules(ModuleInfo *&modules, int &moduleCount);

	virtual void DebugPrintf(const unsigned long printMask, const char *fmt, ...);
};

#endif // !_COMMON_EXECUTION_CONTROLLER_H
