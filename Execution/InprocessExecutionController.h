#ifndef _INPROCESS_EXECUTION_CONTROLLER_H_
#define _INPROCESS_EXECUTION_CONTROLLER_H_

#include "CommonExecutionController.h"
#include <Windows.h>

#include "../revtracer/revtracer.h"

class InprocessExecutionController : public CommonExecutionController {
private :
	HANDLE hThread;

	rev::RevtracerConfig *revCfg;
public :
	virtual bool SetPath();
	virtual bool SetCmdLine();

	virtual void *GetProcessHandle();

	virtual bool GetProcessVirtualMemory(VirtualMemorySection *&sections, int &sectionCount);
	virtual bool GetModules(ModuleInfo *&modules, int &moduleCount);
	virtual bool ReadProcessMemory(unsigned int base, unsigned int size, unsigned char *buff);

	DWORD ControlThread();

	virtual bool Execute();
	virtual bool WaitForTermination();

	virtual void GetCurrentRegisters(Registers &registers);
};

#endif
