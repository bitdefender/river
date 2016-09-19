#ifndef _INPROCESS_EXECUTION_CONTROLLER_H_
#define _INPROCESS_EXECUTION_CONTROLLER_H_

#include "CommonExecutionController.h"
#ifdef _WIN32
#include <Windows.h>
#endif

#include "../revtracer/revtracer.h"

#include "Common.h"

class InprocessExecutionController : public CommonExecutionController {
private :
	THREAD_T hThread;

	rev::RevtracerConfig *revCfg;
public :
	virtual bool SetPath();
	virtual bool SetCmdLine();

	virtual THREAD_T GetProcessHandle();

	virtual bool GetProcessVirtualMemory(VirtualMemorySection *&sections, int &sectionCount);
	virtual bool GetModules(ModuleInfo *&modules, int &moduleCount);
	virtual bool ReadProcessMemory(unsigned int base, unsigned int size, unsigned char *buff);

	DWORD ControlThread();

	virtual bool Execute();
	virtual bool WaitForTermination();
};

#endif
